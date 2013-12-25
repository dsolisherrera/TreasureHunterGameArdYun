/*
 * Constant Values
 */
int const BOARD_SIZE=4; //Slots in board
int const TREASURE_NUM=1;
int SENSOR_MATRIX[]={8,9,10,11};
int POINTS_MATRIX[]={3};
int RGB_MATRIX[]={5,6,7};
int CONTROL_BUTTON=2;

int const TREASURE_FOUND=2;
int const TREASURE=1;
int const EMPTY=0;

/*
 * Variables
 */
bool playing;
int pointsP1, pointsP2;

int treasure_board[BOARD_SIZE]={0};
int board_status[BOARD_SIZE]={0};
int remote_board_status[BOARD_SIZE]={0};


void setup() {
  
  Serial.begin(9600);
  
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

void flushSerial(){
  while(Serial.read() >=0){
    ;//do nothing
  }
}

void setUpGame(){
  digitalWrite(RGB_MATRIX[2],HIGH);
  memset(treasure_board,0,sizeof(treasure_board));
  memset(board_status,0,sizeof(board_status));
  memset(remote_board_status,0,sizeof(remote_board_status));
  
  pointsP1=0;
  pointsP2=0;
  playing=true;
  
  Serial.println("Setting up board.");
  
  Serial.println("Get treasure locations.");
  readBoard(treasure_board, TREASURE_NUM);
  for (int i=0; i < BOARD_SIZE; i++){
    Serial.print(treasure_board[i]);
  }
  Serial.println("");

  digitalWrite(RGB_MATRIX[2],LOW);
}

bool checkBoard(int board[],int limit){
  int cont=0;
  Serial.println("Checking that board is correct.");
  for (int i=0; i < BOARD_SIZE; i++){
      if (board[i]==TREASURE){
        cont++;
      }
   }
   if (cont==limit) return true;
   else return false;
}

void readBoard(int board[], int limit){
  int value=0;
  while (true){

    Serial.println("Reading board status!!");
    memset(board,0,BOARD_SIZE);
    flushSerial();
    delay(2000);
    //while(Serial.available()<=0){
      //delay(500);
    //}
    for (int i=0; i < BOARD_SIZE; i++){
      //value = Serial.parseInt();
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
  digitalWrite(RGB_MATRIX[0],HIGH);
  Serial.println("P1 TURN");
  readBoard(board_status,1);
  sendStatus();
  getResults();
  digitalWrite(RGB_MATRIX[0],LOW);
}


void turnWaitForPlay(){
  digitalWrite(RGB_MATRIX[1],HIGH);
  bool success=false;
  Serial.println("P2 TURN");
  readBoard(remote_board_status,1);
  Serial.println("Comparing boards!!");
  for (int i=0; i < BOARD_SIZE; i++){
    if ((treasure_board[i]==TREASURE) && (remote_board_status[i]==TREASURE)){
      Serial.println("Your Treasure has been found!!");
      treasure_board[i]=TREASURE_FOUND;
      pointsP2++;
      success=true; 
    }
  }
  sendResults(success);
  digitalWrite(RGB_MATRIX[1],LOW);
}

void sendStatus(){
  Serial.print("Sending play: ");
  for (int i=0; i < BOARD_SIZE; i++){
    Serial.print(board_status[i]);
  }
  Serial.println("");
}

void sendResults(bool success){
  
  Serial.println("Sending results of comparison");
  if (success){
    Serial.println("F");
  }
  else{
    Serial.println("N");
  }
}

void getResults(){
  char result;
  flushSerial();
  Serial.println("Waiting for results");
  while(Serial.available()<=0){
    delay(1000);
  }

  result=Serial.read();
  Serial.println("Results from last play is: ");
  Serial.println(result);
  
  if (result=='F'){
    pointsP1++;
  }
}

void paintPoints(){
  if (pointsP1>0){
    //digitalWrite(POINTS_MATRIX[pointsP1-1],HIGH);
    digitalWrite(3,HIGH);
  }
  if (pointsP2>0){
    //digitalWrite(POINTS_MATRIX[pointsP2-1],HIGH);
    digitalWrite(3,HIGH);
  }
  delay(1000);
}

int checkWinner(){
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
  int cont=0;
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
  for (int i=0; i < sizeof(RGB_MATRIX); i++){
    digitalWrite(RGB_MATRIX[i],LOW);
  }
  for (int i=0; i < sizeof(POINTS_MATRIX); i++){
    digitalWrite(POINTS_MATRIX[i],LOW);
  }
}

void loop() {
  Serial.println("Press button to begin");
  if (digitalRead(CONTROL_BUTTON)==HIGH){
    setUpGame();
    startGame();
    cleanGame();
  }
  delay(1000);
}
