/*
 * Treasure Hunter game 
 *
 * sketch for Arduino Yun board1
 * Developed by Ilaria Baggio
 * Master of Advance Studies in Interaction Design.
 * Scuola universitaria professionale della Svizzera italiana (SUPSI)
 * 2013
 */

#include <Bridge.h>
#include <YunClient.h>
#include <PubSubClient.h>
#include <Messenger.h>

/*
 * Constant Values
 */
byte const BOARD_SIZE=4; //Slots in board
byte const TREASURE_NUM=1; //Number of treasures allowed
byte const GUESS_NUM=1; //Number of guesses per turn

//Input/Output pins
byte SENSOR_MATRIX[]={8,9,10,11};
byte POINTS_MATRIX[]={3};
byte RGB_MATRIX[]={5,6,7};
byte CONTROL_BUTTON=2;

//States of slots
byte const TREASURE_FOUND=2;
byte const TREASURE=1;
byte const EMPTY=0;

/*
 * Variables
 */
bool playing;
bool waitingForStatus;
bool waitingForResults;
char result;
byte pointsP1, pointsP2;

byte treasure_board[BOARD_SIZE]={0};
byte board_status[BOARD_SIZE]={0};
byte remote_board_status[BOARD_SIZE]={0};

//Server of MQTT to connect to
byte server[] = { 192,168,0,25};

//Instantiate Messenger object with the message function and the "," separator
Messenger message = Messenger(','); 

//Instantiate the MQTT client on the server using the default port (1883)
YunClient yunClient;
PubSubClient client(server, 1883, pubSubCallback, yunClient);

void setup() {
  
  Serial.begin(9600);
  
  Serial.println("Initializing Bridge");
  Bridge.begin();
  Serial.println("Done initializing the Bridge");
  message.attach(messageRecieved);
  
  /*
   * Connect with label board01
   * Publish initial message on board02 channel
   * Subscribe to owned channel to recieved messages
   */
  if (client.connect("board02")) {
    Serial.println("Connecting as board02");
    client.publish("/devices/board01/in","begin");
    client.subscribe("/devices/board02/in");
  } 
 
  //Set the mode for each pin of the Arduino used
  for (int i=0; i < sizeof(SENSOR_MATRIX); i++){
    pinMode(SENSOR_MATRIX[i],INPUT);
  }
  
  for (int i=0; i < sizeof(RGB_MATRIX); i++){
    pinMode(RGB_MATRIX[i],OUTPUT);
  }
  for (int i=0; i < sizeof(POINTS_MATRIX); i++){
    pinMode(POINTS_MATRIX[i],OUTPUT);
  }
  pinMode(CONTROL_BUTTON,INPUT);
  
}

/*
 * Message Recieved
 * Method to be called each time a complete message is recieved,
 * and is ready to be processed. According to the message set
 * flags and read necesary values.
 */
void messageRecieved(){
  Serial.println("Message recieved");
  if ( message.checkString("status") ) {
    for (int i=0;i<BOARD_SIZE;i++){
      remote_board_status[i]=message.readInt();
    }
    waitingForStatus=false;
  }
  if ( message.checkString("result") ) {
    if ( message.checkString("F") ) {
      result='F';
    } 
    else if ( message.checkString("N") ) {
      result='N';
    }
    waitingForResults=false;
  }
}

/*
 * Publish Subscriber callback
 * Method to be called each time message is recieved in the MQTT queue,
 * read all message and add it to the Messenger object.
 */
void pubSubCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = 13; 
  // The following line is the most effective way of 
  // feeding the serial data to Messenger
  for (int i = 0; i <= length; i++){
    message.process(payload[i]);
  }
}

/*
 * Setup Game
 * Method to that prepares the system for starting a new game
 * it cleans all variables and set them to the initial values,
 * also is in charge of reading the position of the treasures.
 */
void setUpGame(){
  /*
   * Let user know we are setting up the game by turning on one of the
   * RGB leds.
   */
  Serial.println("Setting up board.");
  digitalWrite(RGB_MATRIX[2],HIGH);
  
  //Clean all varaibles to have a fresh start
  memset(treasure_board,0,sizeof(treasure_board));
  memset(board_status,0,sizeof(board_status));
  memset(remote_board_status,0,sizeof(remote_board_status));
  pointsP1=0;
  pointsP2=0;
  playing=true;
  
  Serial.println("Get treasure locations.");
  readBoard(treasure_board, TREASURE_NUM);
  
  for (int i=0; i < BOARD_SIZE; i++){
    Serial.print(treasure_board[i]);
  }
  Serial.println("");

  digitalWrite(RGB_MATRIX[2],LOW);
}

/*
 * Check Board Status
 * Method to that checks that the user is setting the correct number of
 * treasures per game, or using the maximun number of guesses per turn.
 */
bool checkBoard(byte board[],byte limit){
  byte cont=0;
  Serial.println("Checking that board status is correct.");
  //Count number of treasures or tokens in the board, they must match the limit.
  for (byte i=0; i < BOARD_SIZE; i++){
      if (board[i]==TREASURE){
        cont++;
      }
   }
   if (cont==limit) return true;
   else return false;
}

/*
 * Read Board Status
 * Method for reading the board sensors and obtaining the
 * location of the tokens.
 */
void readBoard(byte board[], byte limit){
  byte value=0;
  while (true){
    Serial.println("Reading board status!!");
    memset(board,0,BOARD_SIZE);

    delay(3000);//Give time to player to put token

    for (byte i=0; i < BOARD_SIZE; i++){
      value=digitalRead(SENSOR_MATRIX[i]);
      if (value==TREASURE){
        board[i]=TREASURE;
      }
      else{
        board[i]=EMPTY;
      }
    }
    if (checkBoard(board,limit)) break;
    else Serial.println("Wrong input try again!!");
  }
}

/*
 * Turn Play for local player
 * Read the play, send it to the other board and
 * act according to the results obtained.
 */
void turnPlay(){
  /* Let user know is his turn by turning on one of the
   * RGB leds.
   */
  digitalWrite(RGB_MATRIX[0],HIGH);
  
  Serial.println("P1 TURN");
  readBoard(board_status,GUESS_NUM);
  sendStatus();
  getResults();
  digitalWrite(RGB_MATRIX[0],LOW);
}

/*
 * Turn Play for remote player
 * Wait for the other player's play and compare with treasure position,
 * assign points if necesary and send results to other player.
 */
void turnWaitForPlay(){
  /* Let user know is the other player's turn by turning on one of the
   * RGB leds.
   */
  digitalWrite(RGB_MATRIX[1],HIGH);
  
  bool success=false;
  Serial.println("P2 TURN");
  
  getStatus();
  
  Serial.println("Comparing boards!!");
  
  for (byte i=0; i < BOARD_SIZE; i++){
    if ((treasure_board[i]==TREASURE) && (remote_board_status[i]==TREASURE)){
      Serial.println("One of your Treasure's has been found!!");
      treasure_board[i]=TREASURE_FOUND;
      pointsP2++;
      success=true; 
    }
  }
  sendResults(success);
  
  digitalWrite(RGB_MATRIX[1],LOW);
}

/*
 * Send Status as MQTT message
 * Create the message packet with the proper information of 
 * the board and send it to the other player.
 */
void sendStatus(){
  
  char bstatus[sizeof('status') + 2*BOARD_SIZE];
  
  Serial.print("Sending play: ");
  for (int i=0; i < BOARD_SIZE; i++){
    Serial.print(board_status[i]);
  }
  Serial.println("");
  
  sprintf(bstatus,"%s","status");
  
  for (byte i = 0; i < BOARD_SIZE; i++){
    sprintf(bstatus,"%s,%d",bstatus,board_status[i]);
  }
  client.publish("/devices/board01/in",bstatus);
  
}

/*
 * Get Status as MQTT message
 * Wait until the proper message has been recieved, blink that leds,
 * to let the user knows he must wait for the other player.
 */
void getStatus(){
  while(true){
    waitingForStatus=true;
    Serial.println("Waiting for status");
    while(waitingForStatus){
      digitalWrite(RGB_MATRIX[1],LOW);
      client.loop();
      delay(250);
      digitalWrite(RGB_MATRIX[1],HIGH);
      delay(250);
    }
    if (checkBoard(remote_board_status,GUESS_NUM)) break;
    else Serial.println("Board status recieved is wrong, wait for proper input!!");
  }
}

/*
 * Send Result as MQTT message
 * Create the message packet with the proper information of 
 * the board and send it to the other player.
 */
void sendResults(bool success){
  
  Serial.println("Sending results of comparison");
  if (success){
    client.publish("/devices/board01/in","result,F");
    Serial.println("F");
  }
  else{
    Serial.println("N");
    client.publish("/devices/board01/in","result,N");
  }
}

/*
 * Get Result of play as MQTT message
 * Wait until the proper message has been recieved, blink that leds,
 * to let the user knows he must wait for the other player.
 */
void getResults(){
  waitingForResults=true;
  Serial.println("Waiting for results");
  while(waitingForResults){
    digitalWrite(RGB_MATRIX[0],LOW);
    client.loop();
    delay(250);
    digitalWrite(RGB_MATRIX[0],HIGH);
    delay(250);
  }
  Serial.println(result);
  
  if (result=='F'){
    pointsP1++;
  }
}

/*
 * Paint the points
 * Function for displaying the current score of the game.
 */
void paintPoints(){
  if (pointsP1>0){
    digitalWrite(POINTS_MATRIX[pointsP1-1],HIGH);
  }
  if (pointsP2>0){
    digitalWrite(POINTS_MATRIX[pointsP2-1],HIGH);
  }
  delay(1000);
}


/*
 * Check if there is a Winner
 * Compare points obtained with maximum.
 */
byte checkWinner(){
    if (pointsP1==TREASURE_NUM){
      Serial.println("Found all the tokens, you win :)");
      return 1;
    }
    else if (pointsP2==TREASURE_NUM){
      Serial.println("All your tokens have been found, you loose :(");
      return 2;
    }
    else return 0;
}

/*
 * Game
 * Main loop for the game.
 */
void startGame(){
  Serial.println("Start the game!!");
  byte cont=0;
  while(playing){
    cont++;
    Serial.print("------------- Turn ");
    Serial.print(cont);
    Serial.println(" -------------");
    turnWaitForPlay();
    paintPoints();
    if (checkWinner() != 0) break;
    turnPlay();
    paintPoints();
    if (checkWinner() != 0) break;
  }
}

/*
 * Clean Game
 * Make sure to power off all the status leds before
 * ending the game.
 */
void cleanGame(){
  for (byte i=0; i < sizeof(RGB_MATRIX); i++){
    digitalWrite(RGB_MATRIX[i],LOW);
  }
  for (byte i=0; i < sizeof(POINTS_MATRIX); i++){
    digitalWrite(POINTS_MATRIX[i],LOW);
  }
}

/*
 * Blink RBG led
 * Function for making the RGB led blink.
 */
void blinkRGB(){
  for (byte i=0; i < sizeof(RGB_MATRIX); i++){
    digitalWrite(RGB_MATRIX[i],HIGH);
  }
  delay(250);
  for (byte i=0; i < sizeof(RGB_MATRIX); i++){
    digitalWrite(RGB_MATRIX[i],LOW);
  }
  delay(250);
}

/*
 * Main Loop
 * Blink the leds telling the user to press the button to start a new game.
 */
void loop() {
  Serial.println("Press button to begin");
  if (digitalRead(CONTROL_BUTTON)==HIGH){
    setUpGame();
    startGame();
    cleanGame();
  }
  blinkRGB();
}
