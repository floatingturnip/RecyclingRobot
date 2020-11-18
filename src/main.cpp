//Time Trials Swvivel Michelle

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // This display does not have a reset pin accessible
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define SERVO_SCOOPER PinName::PB_0
#define SERVO_FRONT_DOOR PinName::PA_7
#define SERVO_BACK_DOOR PinName::PB_1
#define BACKDOORSWITCH PB3

#define START_BUTTON PB13
#define INNER_BUTTON PB12
#define COMPETITION_MODE PB15

#define LEFT_SENSOR PA6
#define RIGHT_SENSOR PA5
#define OUTER_SENSOR PA4

#define MOTOR_A_1 PB_9
#define MOTOR_A_2 PB_8
#define MOTOR_B_1 PA_0
#define MOTOR_B_2 PA_1

#define piezoPin PA3

// for the sonar sensors
#define TRIG_PIN PA11
#define ECHO_PIN PA12

#define STATE_ON 0
#define STATE_LEFT 1
#define STATE_RIGHT 2
#define STATE_OFF 4

#define SPEED_MAIN 700  //if old batteries in use use 700, if new batteries, use 600 and a smaller speed 2 value
#define SPEED_1 750 //750 is good
#define SPEED_2 850 //900 is good
//#define SPEED_2_LEFT 700
#define SWIVEL_SPEED 750

#define LEFT_THRESHOLD 60
#define RIGHT_THRESHOLD 60
#define OUTER_THRESHOLD 60

#define TURN_180 3000
#define TURN_90 800
#define MIDDLE_DRIVE 2500

//#define THRESHOLD 50
#define SPEED_MAX 1000
#define PWMFREQ 2000

int run_sonar();
void drive_to_middle_ish();

void swivel_left(int speed);
void swivel_right(int speed);
void stop_moving();
void go_forward(int speed);
void go_back(int speed);
void go_left(int);
void go_right(int);
void hard_left(int speed);
void hard_right(int speed);

void return_to_tape();
void go_to_line();
void check_for_tape();
void follow_tape();
void dump_cans();
void dump_check();

void turn_90_degrees();

void playMusic();

void setServo (PinName name, int degrees);
void scoopACan ();
void backdoorfunction();

void entertainment();

int scoop_counter = 0;
volatile bool backDoorSwitch = 0;

volatile float reflectanceL;
volatile float reflectanceR;

// Tailor these to your robot.
int backDoorUp = 170;
int backDoorDown = 80;

int scoopUp = 60;
int scoopDown = 168;
int scoopLittleUp = 157;

int frontArmOpen = 180;
int frontArmClose = 0;
int frontArmSemiOpen = 110;

int prev_state = STATE_ON;
int error = 0;
int last_error;

long time_limit = 15*1000;
long start_time;

long lastPress = 0;
long buttonTime = 1000; // hold button at least this long before releasing back gate.
//long debounceTime = 500; // do not register button presses within 0.5s of each other. can erase this if the state thing works
int counter = 0;
int state = 0; 
// state 0: butt

void setup() {
  Serial.begin(9600);

  //servos
  pinMode(SERVO_SCOOPER,OUTPUT);
  pinMode(SERVO_BACK_DOOR,OUTPUT);
  pinMode(SERVO_FRONT_DOOR,OUTPUT);

  pinMode(BACKDOORSWITCH, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(BACKDOORSWITCH), backdoorfunction, RISING);

  //sonar sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  //tape sensors
  pinMode(LEFT_SENSOR, INPUT);
  pinMode(RIGHT_SENSOR, INPUT);
  pinMode(OUTER_SENSOR, INPUT);

  //for entertainment purposes
  pinMode(piezoPin, OUTPUT);

  pinMode(START_BUTTON, INPUT_PULLUP); 
  pinMode(INNER_BUTTON, INPUT_PULLDOWN);//maybe input pulldown 
  pinMode(BACKDOORSWITCH, INPUT_PULLUP);
  pinMode(COMPETITION_MODE, INPUT_PULLUP);

  pinMode(MOTOR_A_1, OUTPUT);
  pinMode(MOTOR_A_2, OUTPUT);
  pinMode(MOTOR_B_1, OUTPUT);
  pinMode(MOTOR_B_2, OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
 
  // Displays Adafruit logo by default. call clearDisplay immediately if you don't want this.
  display.display();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.println("Initial Message");
  display.display();

  setServo(SERVO_BACK_DOOR,backDoorUp);
  delay(500);
  setServo(SERVO_SCOOPER,scoopDown);
  delay(500);
  setServo(SERVO_FRONT_DOOR,frontArmOpen);
  delay(500);
  stop_moving();
}

void loop() {
  
    if(!digitalRead(START_BUTTON)){ //Note the start button is input pullup connected to ground
      //playMusic();
      
      start_time = millis();
      drive_to_middle_ish();
      while(millis()-start_time < time_limit){
        swivel_right(SWIVEL_SPEED);
        run_sonar();
      }
      /*
      go_back(SPEED_MAIN);
      delay(500);
      stop_moving();*/
      return_to_tape();
      follow_tape();
      dump_cans();
    }

    //use this bracket for debugging small parts of the code
    if(digitalRead(INNER_BUTTON)){
      
      display.clearDisplay();
      display.setCursor(0,0);
      display.println(analogRead(LEFT_SENSOR));
      display.println(analogRead(RIGHT_SENSOR));
      display.println(analogRead(OUTER_SENSOR));
      display.display();
      
      //return_to_tape();
      //follow_tape();
      //dump_cans();
      playMusic();
      entertainment();
    }
  /*
  else{
    if(!digitalRead(START_BUTTON)){
      entertainment();
    }
    if (digitalRead(INNER_BUTTON)){
      playMusic();
    }
  }
  */
}

/*
void setServo (PinName name, int degrees)
input: 
pinName name =name of the pin to set the pwm output to
int degrees = the angle in degrees between 0 and 180 to set the servo to
Output:
Sets the servo to a particular angle in degrees
*/
void setServo (PinName name, int degrees)
{
  float millisecs=(2000/180)*degrees+500;
  pwm_start(name, 50, millisecs, TimerCompareFormat_t::MICROSEC_COMPARE_FORMAT);
}

/*void scoopACan()
inputs:
None
Outputs:
Scoops a can into the holding area on the top of the robot. returns the door arm
to the straight forward position and the scooper to the down position
preconditions:
The front door must be straight forward, and the scooper must be down.
*/
void scoopACan ()
{
  setServo(SERVO_FRONT_DOOR,frontArmClose);
  delay(800);
  setServo(SERVO_SCOOPER,scoopLittleUp);
  delay(500);
  setServo(SERVO_FRONT_DOOR,frontArmOpen);
  delay(800);
  setServo(SERVO_SCOOPER,scoopUp);
  delay(1000);
  setServo(SERVO_SCOOPER,scoopDown);
  delay(1000);
}
/*
// Interrupt Service Routine
void backdoorfunction() {
  backDoorSwitch = 1;

}*/

int run_sonar(){
   //sonar code
  
  long duration;
  long distance = 1;
  //bool can_found = true;
  while(distance <= 60 && distance > 0){
  //send 10us pulse
    digitalWrite(TRIG_PIN, LOW);  
    delayMicroseconds(2); 
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10); 
    digitalWrite(TRIG_PIN, LOW);

    //read time value, convert to distance in cm using speed of sound
    duration = pulseIn(ECHO_PIN, HIGH);
    distance = (duration/2) / 29.1; //convert time to distance

    Serial.println(distance);
    display.clearDisplay();
    display.setCursor(0,0);
    display.print(distance);
    display.print(" cm");
    display.display();
    check_for_tape();
    
    if(distance <= 60 && distance > 22){//TODO: Change to while loop to see if it helps?
      delay(50);
      //stop_moving();
      go_forward(SPEED_1);
      check_for_tape();
    } else if(distance <=22 && distance >= 1){
      delay(50);
      stop_moving();
      scoopACan();
      //scoopACan();
      //return_to_tape();
    }

    if(millis()-start_time < time_limit){
      return distance;
    }
  }
  //delay(200);
  return distance;
}

void swivel_right(int speed){
  if(speed>SPEED_MAX){
    speed = SPEED_MAX;
  }
  pwm_start(MOTOR_A_1, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_A_2, PWMFREQ, speed, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_2, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_1, PWMFREQ, speed, RESOLUTION_10B_COMPARE_FORMAT);

}

void swivel_left(int speed){
  if(speed>SPEED_MAX){
    speed = SPEED_MAX;
  }
  pwm_start(MOTOR_A_2, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_A_1, PWMFREQ, speed, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_1, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_2, PWMFREQ, speed, RESOLUTION_10B_COMPARE_FORMAT);
}

void stop_moving(){
  pwm_start(MOTOR_A_1, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_A_2, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_1, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_2, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
}

void go_forward(int speed){
  //run_motor(0,1000,0,1000);
  pwm_start(MOTOR_A_1, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_A_2, PWMFREQ, speed, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_1, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_2, PWMFREQ, speed, RESOLUTION_10B_COMPARE_FORMAT);
}

void go_back(int speed){
  pwm_start(MOTOR_A_2, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_A_1, PWMFREQ, speed, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_2, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_1, PWMFREQ, speed, RESOLUTION_10B_COMPARE_FORMAT);
}

void go_left(int speed){
  pwm_start(MOTOR_A_1, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_A_2, PWMFREQ, speed, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_1, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_2, PWMFREQ, SPEED_MAIN, RESOLUTION_10B_COMPARE_FORMAT);
}

void hard_left(int speed){
  pwm_start(MOTOR_A_1, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_A_2, PWMFREQ, speed, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_1, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_2, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
}

void hard_right(int speed){
  pwm_start(MOTOR_A_1, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_A_2, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_1, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_2, PWMFREQ, speed, RESOLUTION_10B_COMPARE_FORMAT);
}
void go_right(int speed){
  pwm_start(MOTOR_A_1, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_A_2, PWMFREQ, SPEED_MAIN, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_1, PWMFREQ, 0, RESOLUTION_10B_COMPARE_FORMAT);
  pwm_start(MOTOR_B_2, PWMFREQ, speed, RESOLUTION_10B_COMPARE_FORMAT);
}

void return_to_tape(){
  scoopACan();
  setServo(SERVO_FRONT_DOOR, frontArmSemiOpen);
  go_forward(SPEED_MAIN);
  int ref_left = analogRead(LEFT_SENSOR);
  int ref_right = analogRead(RIGHT_SENSOR);
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(ref_left);
    display.println(ref_right);
    display.println(analogRead(OUTER_SENSOR));
    display.display();
  while(true){
    ref_left = analogRead(LEFT_SENSOR);
    ref_right = analogRead(RIGHT_SENSOR);
    if (ref_left > LEFT_THRESHOLD || ref_right > RIGHT_THRESHOLD){
      break;
    }
  }
  stop_moving();
  turn_90_degrees();
}

void turn_90_degrees(){
  swivel_right(SWIVEL_SPEED);
  delay(TURN_90);
  stop_moving();
}

void check_for_tape(){
  int ref_left = analogRead(LEFT_SENSOR);
  int ref_right = analogRead(RIGHT_SENSOR);
  if (ref_left>LEFT_THRESHOLD || ref_right > RIGHT_THRESHOLD){
    //stop_moving()
    go_back(SPEED_1);
    delay(500);
    swivel_right(SWIVEL_SPEED);
    delay(100);
  }
}

void follow_tape(){
  int outer_count = 0;
  long current_time = millis();
  long last_time = current_time;

  while(true){
    reflectanceL = analogRead(LEFT_SENSOR);
    reflectanceR = analogRead(RIGHT_SENSOR);
    last_error = error;
    //threshold = analogRead(DETECT_THRESHOLD);
    current_time = millis();

    //if(reflectance > threshold){
    if(reflectanceL > LEFT_THRESHOLD && reflectanceR > RIGHT_THRESHOLD){
      if(analogRead(OUTER_SENSOR) > OUTER_THRESHOLD){
        if(outer_count == 0){
          outer_count = 1;
          last_time = current_time;
        }
        if(outer_count == 1 && millis()-last_time>300){
          stop_moving();
          display.clearDisplay();
          display.setCursor(0,0);
          display.println("Enter Dump Sequence");
          display.println(analogRead(OUTER_SENSOR));
          display.display();
          return;
        }
      }
      //display.println("Nothing here"); //on tape
      go_forward(SPEED_MAIN);
      prev_state = STATE_ON;
    }
    else if(reflectanceL <= LEFT_THRESHOLD && reflectanceR > RIGHT_THRESHOLD){
      //display.println("Left of Tape"); //left of tape
      go_right(SPEED_1);
      prev_state = STATE_LEFT;
      outer_count = 0;
      last_time = current_time;
    }
    else if(reflectanceL > LEFT_THRESHOLD && reflectanceR <= RIGHT_THRESHOLD){
      //display.println("Right of Tape"); //right of tape
      go_left(SPEED_1);
      prev_state = STATE_RIGHT;
      outer_count = 0;
      last_time = current_time;
    }
    else
    {
      if(prev_state == STATE_LEFT){
        hard_right(SPEED_1);
      } else if(prev_state == STATE_RIGHT){
        //go_left(SPEED_2);
        hard_left(SPEED_1);
      }
      else{
        //stop_moving();
        go_right(SPEED_1);
      }
      outer_count = 0;
      last_time = current_time;
    }

    while(digitalRead(INNER_BUTTON)){
      stop_moving();
      display.clearDisplay();
      display.setCursor(0,0);
      
      display.println(reflectanceL);
      display.println(reflectanceR);
      display.println(analogRead(OUTER_SENSOR));
      //display.println(motor_speed);
      display.display();
      delay(100);
    }
  }
}

void drive_to_middle_ish(){
  go_right(SPEED_1+100);
  delay(MIDDLE_DRIVE);
  stop_moving();
}

void dump_cans(){
  //swivel_right(SWIVEL_SPEED);
  //delay(TURN_180);
  //stop_moving();
  setServo(SERVO_FRONT_DOOR, frontArmClose);
  delay(800);
  setServo(SERVO_SCOOPER, scoopLittleUp);
  delay(500);
  go_back(SPEED_1);
  dump_check();
}

void dump_check(){
  while(true){
    bool backDoorSwitch2 = digitalRead(BACKDOORSWITCH);
    long currentMillis = millis();
    //backDoorServo.write(backDoorUp);
    
  // start counting as soon as button pressed 
    if (backDoorSwitch2 == 1 && state == 0) {
      lastPress = currentMillis;
      state = 1;
  // if button not pressed anymore, revert to state 0
    } else if (backDoorSwitch2 == 0 && state == 1) {
      state = 0;
    }

    if ((state == 1 && (currentMillis - lastPress > buttonTime )) || millis()-start_time>59000) {
      stop_moving();
      state = 2;
      counter++;
      // lets door down, waits 1000, brings door back up
      display.clearDisplay();
      display.setCursor(0,0);
      display.print("opening. counter: ");
      display.println(counter);

      display.display();
      //backDoorServo.write(backDoorDown);
      setServo(SERVO_BACK_DOOR, backDoorDown);
      delay(500);
      setServo(SERVO_FRONT_DOOR, frontArmOpen);
      delay(800);
      setServo(SERVO_SCOOPER,scoopUp);
      delay(1000);
      setServo(SERVO_SCOOPER, scoopDown);
      delay(1000);
      scoopACan();

      display.clearDisplay();
      display.setCursor(0,0);
      display.print("Done");
      display.display();
      //backDoorServo.write(backDoorUp);
      /*setServo(SERVO_BACK_DOOR, backDoorUp);
      delay(500);
      
      display.clearDisplay();
      */
      backDoorSwitch2 = 0;
      state = 0;
      return;
    }
  }
}

void entertainment(){
  while(true){
    swivel_right(SWIVEL_SPEED);
    
    long duration;
    long distance = 1;
    //bool can_found = true;
    while (distance <=60 && distance >0)
    {
      //send 10us pulse
      digitalWrite(TRIG_PIN, LOW);  
      delayMicroseconds(2); 
      digitalWrite(TRIG_PIN, HIGH);
      delayMicroseconds(10); 
      digitalWrite(TRIG_PIN, LOW);

      //read time value, convert to distance in cm using speed of sound
      duration = pulseIn(ECHO_PIN, HIGH);
      distance = (duration/2) / 29.1; //convert time to distance

      Serial.println(distance);
      display.clearDisplay();
      display.setCursor(0,0);
      display.print(distance);
      display.print(" cm");
      display.display();

      if(distance <= 50 && distance > 22){
        delay(100);
        //stop_moving();
        //check_for_tape();
        go_forward(SPEED_MAIN);
      }
      if(distance <=22 && distance >= 1){
        delay(50);
        stop_moving();
        //scoopACan();
        //scoopACan();
        //return_to_tape();
      }
      //return distance;
    }
  }
}
//For entertainment purposes, play Mary Had a Little Lamb

void playMusic(){
  int note_length = 500;
  int short_note = 250;
  int pause_1 = 500;
  int pause_2 = 1000;
  int note_e = 329;
  int note_d = 293;
  int note_c = 261;
  int note_g = 392;
  tone(piezoPin, note_e, note_length);
  delay(pause_1);
  tone(piezoPin, note_d, note_length);
  delay(pause_1);
  tone(piezoPin, note_c, note_length);
  delay(pause_1);
  tone(piezoPin, note_d, note_length);
  delay(pause_1);
  tone(piezoPin, note_e, short_note);
  delay(pause_1);
  tone(piezoPin, note_e, short_note);
  delay(pause_1);
  tone(piezoPin, note_e, short_note);
  delay(pause_2);
  tone(piezoPin, note_d, short_note);
  delay(pause_1);
  tone(piezoPin, note_d, short_note);
  delay(pause_1);
  tone(piezoPin, note_d, short_note);
  delay(pause_2);
  tone(piezoPin, note_e, short_note);
  delay(pause_1);
  tone(piezoPin, note_g, short_note);
  delay(pause_1);
  tone(piezoPin, note_g, short_note);
  delay(pause_2);
  tone(piezoPin, note_e, note_length);
  delay(pause_1);
  tone(piezoPin, note_d, note_length);
  delay(pause_1);
  tone(piezoPin, note_c, note_length);
  delay(pause_1);
  tone(piezoPin, note_d, note_length);
  delay(pause_1);
  tone(piezoPin, note_e, short_note);
  delay(pause_1);
  tone(piezoPin, note_e, short_note);
  delay(pause_1);
  tone(piezoPin, note_e, short_note);
  delay(pause_2);
  tone(piezoPin, note_d, short_note);
  delay(pause_1);
  tone(piezoPin, note_d, short_note);
  delay(pause_1);
  tone(piezoPin, note_e, note_length);
  delay(pause_1);
  tone(piezoPin, note_d, note_length);
  delay(pause_1);
  tone(piezoPin, note_c, note_length);
  delay(pause_2);
}
