#include <HX711_ADC.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
//#include <Wire.h> //dont think we need

#define DOUT 10
#define CLK 11
#define USERNAME_LENGTH 6
#define RS 9
#define E 8
#define D7 4
#define D6 5
#define D5 6
#define D4 7
#define S0 3
#define S1 2
#define S2 1
#define S3 0
#define MUXOUT A0
#define INPUTUP A1
#define INPUTDOWN A2
#define INPUTSELECT A3
#define NEWUSERENROLL A4


LiquidCrystal lcd(RS, E, D4, D5, D6, D7);
//LiquidCrystal lcd(12, 11, 5, 4, 3, 2); //need to change
HX711_ADC LoadCell(DOUT,CLK);

byte select_matrix [12] = { B0000, B1000, B0100, B1100, B0010, B1010, B0110, B1110, B0001, B1001, B0101, B1101 };
int select_pin [4] = { S3, S2, S1, S0 }; //pin3=S0, pin2=S1, pin1=S2, pin0=S3; pins are labelled backwards on MUX, S0 is actually S3, ...

int checkforinput();
void enroll_new_user();
char * enter_name();
int enter_weight_goal();
int checkforstep();
int takeweight();
void log_weight(int, unsigned short);
int printdata(int);
int * takePressure();
void logPressure(int, int *);

void setup() {
  float calibrationfactor = 22660; //originally 22680
  float poundfactor = 2.20662;
  lcd.begin(16, 2);  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Welcome to");
  lcd.setCursor(0,1);
  lcd.print("Project OTTER");
  LoadCell.begin();
  LoadCell.start(2000);
  LoadCell.setCalFactor(calibrationfactor/poundfactor);
  lcd.clear();
  // name and weight goal 
  pinMode(INPUTUP, INPUT);
  pinMode(INPUTDOWN, INPUT);
  pinMode(INPUTSELECT, INPUT);
  pinMode(ENROLLNEWUSER, INPUT);
  //Set MUX select signals pins as digital outputs
  for(int i=0; i < 4; i++) {
    pinMode(select_pin[i], OUTPUT);
  }
}

void loop() {
  checkforinput();
}

int checkforinput(){
  //**waits for either the new user button to be pushed or for a registered user to step on the scale**//
   if(digitalRead(NEWUSERENROLL) == LOW){
    enroll_new_user();
  }
  else {
    checkforstep();
  }
}

void enroll_new_user(){
  //**calls various functions to get the user's name, weight goal, weight measurement, and pressure measurement**/
  
  int user_address = 0; // need to find way to determine user_address
  char * personsName = enter_name(); //this is an address to the first element of the username array
  
  for(int i = 0 ; i < USERNAME_LENGTH - 1 ; i++){ //increment through each character of username and store in EEPROM
    EEPROM.write(user_address + i, *personsName);
    personsName++; //increment pointer
  }
  free(personsName);
  char name[6];
  for(int i = 0 ; i < USERNAME_LENGTH - 1 ; i++){
    name[i] = EEPROM.read(user_address + i);
  }
  
  lcd.clear(); //these three lines are for testing
  lcd.setCursor(0,0);
  lcd.print(name);
  delay(2000);
  
  int weightGoal = enter_weight_goal();
 
  int appliedWeight = 0;
  int weight = 0;
  while(appliedWeight<5){
     LoadCell.update();
     appliedWeight = LoadCell.getData();
     weight = takeweight();
  }
  log_weight(user_address, weight);
//run pressure sensing function AND PRESSURE LOGGING FUNCTION
}

char * enter_name() { //need to return name as string (look into pointers)
  //**instructs the user to input a name, returns said name**//
  char *username = (char*)malloc (sizeof (char) * USERNAME_LENGTH);
  char letter = 'A';
  char user[USERNAME_LENGTH];
  int name_index = 1;
  int name_cursor = 0;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("New User");
  lcd.setCursor(0,1);
  lcd.print("Registration");
  delay(1500);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Enter your name:");
  while((user[name_index-2] != '[') && (name_index < 11)){
    name_cursor = name_index-1;
    lcd.setCursor(name_cursor,1);
    lcd.print(letter);
    if(digitalRead(INPUTUP) == LOW){ //input == up
      if(letter != '['){
        letter++;
      }
      else{
        letter = 'A';
      }
      delay(200);
    }
    else if(digitalRead(INPUTDOWN) == LOW){ //input == down
      if(letter != 'A'){
        letter--;
      }
      else{
        letter = '[';
      }
      delay(200);
    }
    else if(digitalRead(INPUTSELECT) == LOW){ //input == select
      if(letter != ']'){
        user[name_index-1] = letter;
      }
      name_index++;
      letter = 'A';
      delay(200);
    }
  }
  for(int i = 0; i < name_index-2; i++){
   username[i] = user[i];    
  }
  for(int i = name_index-2; i < USERNAME_LENGTH; i++){
    username[i] = ' ';
  }
  return username;
}

int enter_weight_goal() {
  //**asks users for a 3 digit weight goal in lbs, returns weight goal**//
   float weight_goal = 0;
   int number = 0;
   int weight_index = 0;
   int weight_cursor;
   lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("Enter lb goal:");
   lcd.setCursor(1,1);
   lcd.print("00 lbs");
   while(weight_index <= 2){
    lcd.setCursor(weight_index,1);
    lcd.print(number);
    if(digitalRead(INPUTUP) == LOW){ //input == up
      if(number!= 9){
        number++;
      }
      else {
        number = 0;
      }
    delay(200);
    }
    else if (digitalRead(INPUTDOWN) == LOW) { //input == down
      if(number != 0){
        number--;
      }
      else {
        number = 9;
      }
    delay(200);
    }
    else if (digitalRead(INPUTSELECT) == LOW) { //input == select
      weight_goal += number * pow(10,2-weight_index);
      weight_index++;
      number = 0; 
      delay(200); 
    }
  }
  lcd.clear();
  return weight_goal;
}

int checkforstep(){
  //**Checks if user has stepped on scale**//
  LoadCell.update();
  float i = LoadCell.getData();
  if(i>5){ //checks if load cells have exceeded 5 lbs, min weight is 5
    takeweight();
  }
  else {
    lcd.setCursor(0,0);
    lcd.print("Ready to weigh");
  }
}

int takeweight(){
  //**uses loadcell functions to measure user's weight, returns weight**//
  float userWeight = 0;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Weighing...");
  for(float q = 0; q<20000; q++){ //cycles through 10000 readings
    LoadCell.update();
    userWeight = LoadCell.getData();
    if(userWeight<0){
      userWeight = 0;
    }
  }
  printdata(userWeight); //prints the 10000th reading
  while(userWeight>5){ //pauses while user is still on scale
    LoadCell.update();
    userWeight = LoadCell.getData();
  }
  lcd.clear();
  return userWeight;
}

void log_weight(int user_address, unsigned short weight) {
  //**logs weight entry into EEPROM location for specified user**//
  //Check which weight entry it is
  byte entry = EEPROM.read(user_address + 40);

  //If more than 10 entires exist, overwrite the oldest entry
  while (entry >= 10) {
    entry = entry - 10; 
  }
  int weight_address = user_address + 20 + (2*entry);
  EEPROM.update(weight_address, weight);
  EEPROM.write(user_address + 40, EEPROM.read(user_address + 40) + 1);

  //Calculate goal difference and weight difference ***MAKE THIS IT'S OWN FUNCTION
  unsigned short goal_weight, last_weight;
  EEPROM.get(user_address + 6, goal_weight);
  if (weight_address == (user_address + 20)) {
    EEPROM.get(user_address + 38, last_weight);
  }
  else {
    EEPROM.get(weight_address - 2, last_weight);
  }
  int goal_difference = weight - goal_weight;
  int weight_difference = weight - last_weight;

  //Print status
  lcd.clear();
  lcd.setCursor(0,0);
  if (weight_difference = 0){
    lcd.print("Your weight has not changed");
  }
  else if (weight_difference > 0) {
    lcd.print("You have gained");
    lcd.setCursor(0,1);
    lcd.print(weight_difference);
    lcd.setCursor(3,1);
    lcd.print("pounds");
  }
  else {
    lcd.print("You have lost");
    lcd.setCursor(0,1);
    lcd.print(weight_difference);
    lcd.setCursor(3,1);
    lcd.print("pounds");
  }
  
  delay(3000);

  lcd.clear();
  lcd.setCursor(0,0);
  if (goal_difference = 0){
    lcd.print("You reached your weight goal!");
  }
  else if (weight_difference > 0) {
    lcd.print("You are");
    lcd.setCursor(0,1);
    lcd.print(goal_difference);
    lcd.setCursor(3,1);
    lcd.print("pounds above goal");
  }
  else {
    lcd.print("You are");
    lcd.setCursor(0,1);
    lcd.print(goal_difference);
    lcd.setCursor(3,1);
    lcd.print("pounds below goal");
  }
}

int * takePressure() {
  delay(2000);// 2 second delay to allow user to stop moving

  int foot_map [12] = {0}; 
  
  for(byte select_signal=0; select_signal < 12; select_signal++){ 
    
    //Set the MUX select signal
    for(int i=0; i < 4; i++) {
      digitalWrite(select_pin[i], bitRead(select_matrix[select_signal],i));
    }

    //Read 10 samples from pressure sensor
    int intermediate_pressure = 0;
    for(int sample=0; sample < 10; sample++) {
      intermediate_pressure = intermediate_pressure + analogRead(A0);
      delay(500);
    }

    //Average the 10 samples and store in foot map
    foot_map[select_signal] = intermediate_pressure / 10;
    
    //Return a pointer to the first value of the foot map
    return foot_map; 
  }
}

void logPressure(int user_address, int foot_map[]) {
  //Check if it is the first input
  int sum = 0;
  for(int i = 0; i < 12; ++i) {
    sum = sum + EEPROM.read(user_address + 8 + i);
  }

  if(sum == 0) { //It is the first input, write directly to EEPROM
    for(int i = 0; i < 12; ++i) {
      EEPROM.update(user_address + 8 + i, foot_map[i]);
    }
  }
  else { //Not the first write, need to average the new values with the old ones (using exponential moving average)
    byte entry = EEPROM.read(user_address + 40);
    for(int i = 0; i < 12; ++i) {
      EEPROM.update(user_address + 8 + i, (2*foot_map[i]/(entry+1)+(entry-1)*EEPROM.read(user_address + 8 + i))/(entry+1));
    }
  }
}

int printdata(int i){ 
  //**prints weight data measured in take_weight()**//
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Weight [lbs]:");
    lcd.setCursor(0, 1);
    lcd.print(i, 1);
}
