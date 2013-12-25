
#include <Bridge.h>
#include <YunClient.h>
#include <PubSubClient.h>
#include <Messenger.h>

/*
 * Constant Values
 */
byte const BOARD_SIZE=4; //Slots in board
byte const TREASURE_NUM=1;
byte const GUESS_NUM=1;
byte SENSOR_MATRIX[]={8,9,10,11};
byte POINTS_MATRIX[]={3};
byte RGB_MATRIX[]={5,6,7};
byte CONTROL_BUTTON=2;

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

byte server[] = { 192,168,0,25};

// Instantiate Messenger object with the message function and the default separator (the space character)
Messenger message = Messenger(','); 

YunClient yunClient;
PubSubClient client(server, 1883, pubSubCallback, yunClient);

void setup() {
  
  Serial.begin(9600);
  
  Serial.println("Initializing Bridge");
  Bridge.begin();
  Serial.println("Done with Bridge");
  message.attach(messageRecieved);
  
  if (client.connect("board01")) {
    Serial.println("Connecting as board01");
    client.publish("/devices/board02/in","begin");
    client.subscribe("/devices/board01/in");
  } 
 
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

// Define messenger function
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

// Callback function
void pubSubCallback(char* topic, byte* payload, unsigned int length) {

  payload[length] = 13; 
  // The following line is the most effective way of 
  // feeding the serial data to Messenger
  for (int i = 0; i <= length; i++){
    message.process(payload[i]);
  }
}

void setUpGame(){
  //Let user know we are setting up the game
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

bool checkBoard(byte board[],byte limit){
  byte cont=0;
  Serial.println("Checking that board status is correct.");
  //Count number of treasures in board, they must match the limit.
  for (byte i=0; i < BOARD_SIZE; i++){
      if (board[i]==TREASURE){
        cont++;
      }
   }
   if (cont==limit) return true;
   else return false;
}

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

void turnPlay(){
  //Let user know is his turn
  digitalWrite(RGB_MATRIX[0],HIGH);
  
  Serial.println("P1 TURN");
  readBoard(board_status,GUESS_NUM);
  sendStatus();
  getResults();
  digitalWrite(RGB_MATRIX[0],LOW);
}


void turnWaitForPlay(){
  //Let user know is the other player's turn
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
  client.publish("/devices/board02/in",bstatus);
  
}

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
    else Serial.println("Wrong input try again!!");
  }
}

void sendResults(bool success){
  
  Serial.println("Sending results of comparison");
  if (success){
    client.publish("/devices/board02/in","result,F");
    Serial.println("F");
  }
  else{
    Serial.println("N");
    client.publish("/devices/board02/in","result,N");
  }
}

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

void paintPoints(){
  if (pointsP1>0){
    digitalWrite(POINTS_MATRIX[pointsP1-1],HIGH);
  }
  if (pointsP2>0){
    digitalWrite(POINTS_MATRIX[pointsP2-1],HIGH);
  }
  delay(1000);
}

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

void startGame(){
  Serial.println("Start the game!!");
  byte cont=0;
  while(playing){
    cont++;
    Serial.print("------------- Turn ");
    Serial.print(cont);
    Serial.println(" -------------");
    turnPlay();
    paintPoints();
    if (checkWinner() != 0) break;
    turnWaitForPlay();
    paintPoints();
    if (checkWinner() != 0) break;
  }
}

void cleanGame(){
  for (byte i=0; i < sizeof(RGB_MATRIX); i++){
    digitalWrite(RGB_MATRIX[i],LOW);
  }
  for (byte i=0; i < sizeof(POINTS_MATRIX); i++){
    digitalWrite(POINTS_MATRIX[i],LOW);
  }
}

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

void loop() {
  Serial.println("Press button to begin");
  if (digitalRead(CONTROL_BUTTON)==HIGH){
    setUpGame();
    startGame();
    cleanGame();
  }
  blinkRGB();
}
