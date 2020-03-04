#include <HX711_ADC.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <Wire.h>

#define DOUT 7
#define CLK 6
#define USERNAME_LENGTH 6

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
HX711_ADC LoadCell(DOUT,CLK);


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
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
}


int printdata(int i){ 
  //**prints weight data measured in take_weight()**//
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Weight [lbs]:");
    lcd.setCursor(0, 1);
    lcd.print(i, 1);
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
  float i = 0;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Weighing...");
  for(float q = 0; q<20000; q++){ //cycles through 10000 readings
    LoadCell.update();
    i = LoadCell.getData();
    if(i<0){
      i = 0;
    }
  }
  printdata(i); //prints the 10000th reading
  while(i>5){ //pauses while user is still on scale
    LoadCell.update();
    i = LoadCell.getData();
  }
  lcd.clear();
  //return i;
}

char * enter_name() { //need to return name as string (look into pointers)
  //**instructs the user to input a name, returns said name**//
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
    if(digitalRead(A0) == LOW){ //input == up
      if(letter != '['){
        letter++;
      }
      else{
        letter = 'A';
      }
      delay(200);
    }
    else if(digitalRead(A1) == LOW){ //input == down
      if(letter != 'A'){
        letter--;
      }
      else{
        letter = '[';
      }
      delay(200);
    }
    else if(digitalRead(A2) == LOW){ //input == select
      if(letter != ']'){
        user[name_index-1] = letter;
      }
      name_index++;
      letter = 'A';
      delay(200);
    }
  }
  char username[USERNAME_LENGTH];
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
    if(digitalRead(A0) == LOW){ //input == up
      if(number!= 9){
        number++;
      }
      else {
        number = 0;
      }
    delay(200);
    }
    else if (digitalRead(A1) == LOW) { //input == down
      if(number != 0){
        number--;
      }
      else {
        number = 9;
      }
    delay(200);
    }
    else if (digitalRead(A2) == LOW) { //input == select
      weight_goal += number * pow(10,2-weight_index);
      weight_index++;
      number = 0; 
      delay(200); 
    }
  }
  lcd.clear();
  return weight_goal;
}

void enroll_new_user(){
  //**calls various functions to get the user's name, weight goal, weight measurement, and pressure measurement**/
  char * personsName = enter_name(); //this is an address to the first element of the username array
  
  for(int i = 0 ; i < USERNAME_LENGTH - 1 ; i++){ //increment through each character of username and store in EEPROM
    EEPROM.write(user_address + i, *personName);
    personsName++; //increment pointer
  }
  char name[6];
  for(int i = 0 ; i < USERNAME_LENGTH - 1 ; i++){
    name[i] = EEPROM.read(user_address + i);
  }
  
  lcd.clear(); //these three lines are for testing
  lcd.setCursor(0,0);
  lcd.print(name);
  delay(2000);
  
  int weightGoal = enter_weight_goal();
 // checkforstep(); //need to change somehow, if no one is stepping on the scale at the moment this is called it will not take a reading. Maybe add a message telling the user to get on the scale and add a while loop that ends only when someone steps on. After that we could just call the take_weight function
//run pressure sensing function\
//return(name, weight_goal, weight, pressure_array)
}

int checkforinput(){
  /**waits for either the new user button to be pushed or for a registered user to step on the scale**//
   if(digitalRead(A3) == LOW){
    enroll_new_user();
  }
  else {
    checkforstep();
  }
}

void loop() {
  checkforinput();
}



void log_weight(int user_address, unsigned short weight) {
  //Check which weight entry it is
  byte entry = EEPROM.read(user_address + 40);

  //If more than 10 entires exist, overwrite the oldest entry
  while (entry >= 10) {
    entry = entry - 10; 
  }
  int weight_address = user_address + 20 + (2*entry);
  EEPROM.update(weight_address, weight);
  EEPROM.write(user_address + 40, EEPROM.read(user_address + 40) + 1);

  //Calculate goal difference and weight difference
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
