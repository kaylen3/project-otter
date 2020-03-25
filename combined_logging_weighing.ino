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
#define S2 12 //not in tx/rx
#define S3 13 //not in tx/rx
#define MUXOUT A0
#define INPUTUP A1
#define INPUTDOWN A2
#define INPUTSELECT A3
#define NEWUSERENROLL A4
#define CALIBRATIONFACTOR 23000  //originally 22680, did more testing 23000 was more accurate
#define POUNDFACTOR 2.20662
#define NUMBEROFUSERSADDRESS 0
#define TIMEOUT 5000
#define PRESSUREWEIGHTING 0.7 //how heavily we weight pressure compared to weight when identifying users

LiquidCrystal lcd(RS, E, D4, D5, D6, D7);
HX711_ADC LoadCell(DOUT,CLK);

byte select_matrix [12] = { B0000, B1000, B0100, B1100, B0010, B1010, B0110, B1110, B0001, B1001, B0101, B1101 };
int select_pin [4] = { S3, S2, S1, S0 }; //pin3=S0, pin2=S1, pin1=S2, pin0=S3; pins are labelled backwards on MUX, S0 is actually S3, ...

void setup();
int checkForInput();
void enrollNewUser();
char * enterName();
unsigned short enterWeightGoal();
int checkForStep();
unsigned short takeWeight();
void logWeight(int, unsigned short);
void goalStatus(int, int, unsigned short);
int printData(int);
int * takePressure();
void logPressure(int, int *);
void identifyUser();
void waitForStep(int);


void setup() {
  //**Initializes pins, initializes load cells, and welcomes user**//
  lcd.begin(16, 2);  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Welcome to");
  lcd.setCursor(0,1);
  lcd.print("Project OTTER");
  LoadCell.begin();
  LoadCell.start(2000);
  LoadCell.setCalFactor(CALIBRATIONFACTOR/POUNDFACTOR);
  lcd.clear(); 
  pinMode(INPUTUP, INPUT);
  pinMode(INPUTDOWN, INPUT);
  pinMode(INPUTSELECT, INPUT);
  pinMode(NEWUSERENROLL, INPUT);
  //Set MUX select signals pins as digital outputs
  for(int i=0; i < 4; i++) {
    pinMode(select_pin[i], OUTPUT);
  }
}

void loop() {
  checkForInput();
}

int checkForInput(){
  //**waits for either the new user button to be pushed or for a registered user to step on the scale**//
   if(digitalRead(NEWUSERENROLL) == LOW){
    enrollNewUser();
  }
  else {
    checkForStep();
  }
}

void enrollNewUser(){
  //**calls various functions to get the user's name, weight goal, weight measurement, and pressure measurement**/
  
  byte existing_users = EEPROM.read(NUMBEROFUSERSADDRESS);
  int user_address = 1 + (existing_users*37); 
  
  //clear the number of writes value in memory
  EEPROM.update(user_address + 36, 0);
  
  //clear the name cells in memory in case the length of the user's name is less than USERNAME_LENGTH
  for(int i = 0; i < USERNAME_LENGTH; i++){
    EEPROM.update(user_address + i, ' ');
  }
  
  //get user's name
  char * personsName = enterName(); 
  
   //store user's name in EEPROM
  for(int j = 0 ; j < USERNAME_LENGTH; j++){
    EEPROM.write(user_address + j, *personsName);
    personsName++;
  }
  free(personsName);
  
  //get user's weight goal
  unsigned short weightGoal = enterWeightGoal();
  
  //store user's weight goal in EEPROM
  EEPROM.put(user_address + 6, weightGoal);
 
  //have the user step on and off the scale 5 times
  for(int i = 0; i<5; i++) { 
    
    //instruct user to step on the scale
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Please step on");
    
    //waitForStep(1);
  
    //get user's weight
    unsigned short appliedWeight = 0;
    unsigned short weight = 0;
    while(appliedWeight<5){ //wait until user steps on scale
      LoadCell.update();
      appliedWeight = LoadCell.getData();
      weight = takeWeight();
    }
  
    //store user's weight in EEPROM
    logWeight(user_address, weight);
  
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Scanning..."); //might just wanna get rid of this, scanning is probably too fast
    
    //get user's foot map
    int* foot_map = takePressure();
  
    //store foot map in EEPROM
    logPressure(user_address, foot_map);
    free(foot_map);
    
    //instruct user to step off the scale
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Please step off");
    
    waitForStep(0);
    
  }
  //increment the number of users stored in the scale 
  EEPROM.write(NUMBEROFUSERSADDRESS, existing_users+1);
  
}

char * enterName() { 
  //**instructs the user to input a name, returns said name via a pointer**//
  char *username = (char*)malloc (sizeof (char) * USERNAME_LENGTH);
  char letter = 'A';
  char user[USERNAME_LENGTH];
  int name_index = 1;
  int name_cursor = 0;
  int name_done_flag = 0;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("New User");
  lcd.setCursor(0,1);
  lcd.print("Registration");
  delay(1500);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Enter your name:");
  while((name_index < USERNAME_LENGTH) && (name_done_flag == 0)){
    name_cursor = name_index-1;
    lcd.setCursor(name_cursor,1);
    lcd.print(letter);
    if(digitalRead(INPUTUP) == LOW){ //input == up
      if(letter != '['){
        letter++;
      }
      else{
        letter = 'Z';
      }
      delay(200);
    }
    else if(digitalRead(INPUTDOWN) == LOW){ //input == down
      if(letter != 'A'){
        letter--;
      }
      else{
        letter = 'Z';
      }
      delay(200);
    }
    else if(digitalRead(INPUTSELECT) == LOW){ //input == select
      user[name_index-1] = letter;
      name_index++;
      letter = 'A';
      delay(200);
    }
    else if(digitalRead(NEWUSERENROLL) == LOW){// input == finish name (user enroll button)
      name_done_flag = 1;
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

unsigned short enterWeightGoal() {
  //**asks users for a 3 digit weight goal in lbs, returns weight goal**//
   unsigned short weight_goal = 0;
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

int checkForStep(){
  //**Checks if user has stepped on scale**//
  LoadCell.update();
  float i = LoadCell.getData();
  if(i>5){ //checks if load cells have exceeded 5 lbs, min weight is 5
    identifyUser();
  }
  else {
    lcd.setCursor(0,0);
    lcd.print("Ready to weigh");
  }
}

unsigned short takeWeight(){
  //**uses loadcell functions to measure user's weight, returns weight**//
  unsigned short userWeight = 0;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Weighing...");
  delay(3000); //give time for user to put full weight
  for(float q = 0; q<20000; q++){ //cycles through 10000 readings
    LoadCell.update();
    userWeight = LoadCell.getData();
    if(userWeight<0){
      userWeight = 0;
    }
  }
  printData(userWeight); //prints the 10000th reading
  
  return userWeight;
}

void logWeight(int user_address, unsigned short weight) { 
  //**logs weight entry into EEPROM location for specified user**//
  //Check which weight entry it is
  byte entry = EEPROM.read(user_address + 36) + 1; //added plus one so that (for example) on the second read, entry will be equal to 2

  //store most recent weight in EEPROM
  EEPROM.put(user_address + 34, weight);
  
  //update number of entries
  EEPROM.write(user_address + 36, EEPROM.read(user_address + 36) + 1);
  
  //check if it is the first entry, if not, do EMA. 
  if(entry != 1){
    
    //compute exponential moving average and store in EEPROM
    unsigned short oldEMA;
    EEPROM.get(user_address + 32, oldEMA);
    EEPROM.put(user_address + 32, 2*weight/(entry+1)+(entry-1)*oldEMA/(entry+1));
  }
  else{
    //if it is the first entry, store the weight directly
    EEPROM.put(user_address + 32, weight);
  }
}

void goalStatus(int user_address, unsigned short weight) {
  //**Calculates and prints user's progress relative to their goals**//
  
  //Calculate goal difference and weight difference 
  unsigned short goal_weight, last_weight;
  EEPROM.get(user_address + 6, goal_weight);
  EEPROM.get(user_address + 34, last_weight);
  
  int goal_difference = weight - goal_weight;
  int weight_difference = weight - last_weight;

  //Print status
  lcd.clear();
  lcd.setCursor(0,0);
  if (weight_difference == 0){
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
    lcd.print(abs(weight_difference));
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
    lcd.print("You are ");
    lcd.setCursor(8,0);
    lcd.print(goal_difference);
    lcd.setCursor(11,0);
    lcd.print("lbs");
    lcd.setCursor(0,1);
    lcd.print("above your goal");
  }
  else {
    lcd.print("You are ");
    lcd.setCursor(8,0);
    lcd.print(abs(goal_difference));
    lcd.setCursor(11,0);
    lcd.print("lbs");
    lcd.setCursor(0,1);
    lcd.print("below your goal");
  }

  delay(3000);
}

int printData(int i){ 
  //**prints weight data measured in take_weight()**//
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Weight [lbs]:");
    lcd.setCursor(0, 1);
    lcd.print(i, 1);
}

int * takePressure() {
  //** Cycles through 12 pressure sensors and returns pointer to array with their measurements in it**//
  delay(2000);// 2 second delay to allow user to stop moving
  int *foot_map = (int*)malloc (sizeof (int) * 12);
  //int foot_map [12] = {0}; I don't think we need to do this anymore with the line above 
  
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

void logPressure(int user_address, int *foot_map) {
  //** Takes pointer to foot map created by takePressure() and stores it in the user's profile in EEPROM**//
  //Check if it is the first input
  byte entry = EEPROM.read(user_address + 36);

  if(entry == 1) { //It is the first input, write directly to EEPROM
    for(int i = 0; i < 12; ++i) {
      EEPROM.put(user_address + 8 + (2*i), foot_map[i]);
    }
  }
  else { //Not the first write, need to average the new values with the old ones (using exponential moving average)
    int oldEMA;
    for(int i = 0; i < 12; ++i) {
      EEPROM.get(user_address + 8 + (2*i), oldEMA);
      EEPROM.put(user_address + 8 + (2*i), 2*foot_map[i]/(entry+1)+(entry-1)*oldEMA/(entry+1));
    }
  }
}

void identifyUser(){
  //** Compares weight/pressure measurements of the current user to all stored profiles and returns the memory location of the user that most closely matches the current user.**//
  
  unsigned short totalDifference;
  int singlePressureDifference[12];
  unsigned short weightDifference;
  unsigned short bestTotalDifference = 30000;
  int chosenUser = NUMBEROFUSERSADDRESS + 1;
  unsigned short userWeight;
  int userPressure; 

  //check how many users are registered
  byte numberOfUsers = EEPROM.read(NUMBEROFUSERSADDRESS);
  
  //make sure at least one profile is stored in the scale
  if(numberOfUsers == 0){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("No profiles,");
    lcd.setCursor(0,1);      
    lcd.print("please enroll");
    waitForStep(0);
    lcd.clear();
    return;
  }
  
  //take weight reading
  unsigned short newWeight = takeWeight();
  
  //take pressure reading
  int* newFootMap = takePressure(); 
  
  //initialize variable to the first memory cell of the first stored user
  int storedUserAddress = NUMBEROFUSERSADDRESS + 1;
  
  //for loop that iterates through all stored user profiles 
  for(int i = 0; i < numberOfUsers; i++){
    
    int pressureDifference = 0;
    
    //nested for loop computing the percent difference between all cells in the pressure measurements
    for(int j = 0; j < 12; j++){
      EEPROM.get(storedUserAddress + 8 + (2*j), userPressure);
      singlePressureDifference[j] = fabs((*newFootMap - userPressure)/userPressure);
      
      pressureDifference += singlePressureDifference[j];
      
      newFootMap += 2;
    }
    pressureDifference /= 12;
    
    newFootMap -= 24; 
   
    //compute difference in EMA weight in profile compared to measured weight
    EEPROM.get(storedUserAddress + 32, userWeight);
    weightDifference = fabs((newWeight - userWeight)/userWeight); 
  
    //compute total difference value 
    totalDifference = PRESSUREWEIGHTING*pressureDifference + (1 - PRESSUREWEIGHTING)*weightDifference;
  
    //if new total difference value < old total difference value --> replace old total difference value and record user in chosenUser variable
    if(totalDifference < bestTotalDifference) {
      bestTotalDifference = totalDifference;
      chosenUser = storedUserAddress;
    }
    
    //cycle to next stored user
    storedUserAddress += 37;
  }
  
  //Get chosen user's name
  char name[USERNAME_LENGTH];
  for(int i = 0; i < USERNAME_LENGTH; i++){
    name[i] = EEPROM.read(chosenUser + i);
  }  
  
  //display, "Hello ___" to the chosen user
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Hello ");
  lcd.setCursor(0,1);
  lcd.print(name); 
  
  float wrong_user_flag = 10;
  //run a while loop for a given amount of time to allow the user to step off the scale to signal that they are the wrong user
  for(int t = 0; t*250 < TIMEOUT; t++){
  
    //if user steps off the scale, and ask them to try again and abort the function
    LoadCell.update();
    wrong_user_flag = LoadCell.getData();
    if(wrong_user_flag < 5){

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Please Try Again");
      delay(3000);
      lcd.clear();
      return;
    }   
    
    delay(250);
  }
  
  goalStatus(chosenUser, newWeight);
  //MAKE SURE TO RUN GOALSTATUS BEFORE LOGWEIGHT, this is to ensure that it can calculate difference from last weight before overwriting that value
  
  logWeight(chosenUser, newWeight);
  
  logPressure(chosenUser, newFootMap);
  
  //Free newFootMap pointer
  free(newFootMap);
  
  //display, "Bye ___" to chosen user
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Goodbye ");
  lcd.setCursor(0,1);
  lcd.print(name); 
  delay(3000);

  waitForStep(0);
  lcd.clear();
  
  return;
}

void waitForStep(int off_on){
  float step_flag;
  
  if(off_on == 0){ //if called with a '0', wait for user to step off
    step_flag = 10;
    while(step_flag > 5){
      LoadCell.update();
      step_flag = LoadCell.getData();
    }
  }
    
  else if(off_on == 1){ //if called with a '1', wait for user to step on
    step_flag = 0;
    while(step_flag < 5){
      LoadCell.update();
      step_flag = LoadCell.getData();
    }
  }
    
  delay(2000); //try to prevent it from weighing after majority of weight is shifted off / minority of weight is shifted on
}
    
    
