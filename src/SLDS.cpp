#include <Arduino.h>

#include <AccelStepper.h>
#include <MultiStepper.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <TimerOne.h>

#include <SPI.h>
#include <Controllino.h>

// Stepper parameters
const int stepsPerRevolution = 400;

const int enaStirrPin = 2; 
const int dirStirrPin = 3; 
const int stepStirrPin = 4;

const int enaPumpPin = 5;
const int dirPumpPin = 6;
const int stepPumpPin = 7;

AccelStepper stirrStepper = AccelStepper(1, stepStirrPin, dirStirrPin);
AccelStepper pumpStepper = AccelStepper(1, stepPumpPin, dirPumpPin);

// Input parameters
const int potStirrPin = A0;
const int potPumpPin = A1;
int potValueStirr = 0;
int potValuePump = 0;

const int btnStartPin = A5;
const int btnStopPin = A6;

// const int btnEnablePumpPin = A7;
// const int btnEnableStirrPin = A8;

const int switchModePin = A9;

// Program parameters
int stirrerSpeed = 10;
unsigned long pumpStepperTarget = stepsPerRevolution;
int pumpSpeed = 1;

int prevStirrerSpeed = -1;
int prevPumpSpeed = -1;
bool prevSwitchState = false;

int lastPotValueStirr = -1;
int lastPotValuePump = -1;

unsigned long lastDebounceTimeStart = 0;  // Last debounce time for start button 
unsigned long lastDebounceTimeStop = 0; // Last debounce time for stop button
unsigned long lastExecutionTime = 0; // Last execution time for update functions

bool isDosing = false;
bool switchState = false; // False equals single dose mode, true equals continuous mode

LiquidCrystal_I2C lcd(0x3F, 16, 2); // Add I2C address of display

void setup();
void loop();
void updateStirrerSpeed();
void updatePumpSpeed();
void startDosing(bool mode);
void runSteppers();
bool debounceButton(int buttonPin, unsigned long& lastDebounceTime);

void setup()
{
  //Stepper config
  pinMode(enaStirrPin, OUTPUT);
  pinMode(dirStirrPin, OUTPUT); 
  pinMode(stepStirrPin, OUTPUT);
  digitalWrite(enaStirrPin, LOW);

  pinMode(enaPumpPin, OUTPUT);
  pinMode(dirPumpPin, OUTPUT);
  pinMode(stepPumpPin, OUTPUT);
  digitalWrite(enaPumpPin, LOW);

  stirrStepper.setMaxSpeed((2*stepsPerRevolution));
  stirrStepper.setAcceleration(30000);
  stirrStepper.move(1000000);

  pumpStepper.setMaxSpeed(pumpSpeed*stepsPerRevolution);
  pumpStepper.setAcceleration(30000);
  pumpStepper.move(pumpStepperTarget);


  //Input config
  pinMode(potStirrPin, INPUT);
  pinMode(potPumpPin, INPUT);
  pinMode(btnStartPin, INPUT);
  pinMode(btnStopPin, INPUT);
  pinMode(switchModePin, INPUT);

  //Display config
  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Stirrer:");
  lcd.setCursor(12, 0);
  lcd.print("RPM");

  lcd.setCursor(0, 1);
  lcd.print("Pump:");
  lcd.setCursor(12, 1);
  lcd.print("RPS");

  //Timer config
  Timer1.initialize(100); // Timer for stepper motor updates
  Timer1.attachInterrupt(runSteppers);
}

void loop()
{
  // Non-blocking debounce check for buttons
  if (debounceButton(btnStartPin, lastDebounceTimeStart))
  {
    isDosing = true;
    pumpStepper.move(stepsPerRevolution);
    // startDosing(switchState);
  }

  if (debounceButton(btnStopPin, lastDebounceTimeStop))
  {
    isDosing = false;
  }

  //Read and update only if necessary to minimize delay
  if (millis() - lastExecutionTime > 300)
  {
    // Change Switch State
    switchState = digitalRead(switchModePin);

    // Update Stepper Speeds
    updateStirrerSpeed();
    updatePumpSpeed();

    // Update Display
    if (prevStirrerSpeed != stirrerSpeed)
    {
      lcd.setCursor(8, 0);
      if (stirrerSpeed < 100) {
        lcd.print(F("0")); // Print a leading zero
      }
      lcd.print(stirrerSpeed);
      prevStirrerSpeed = stirrerSpeed;
    }
    if (prevPumpSpeed != pumpSpeed)
    {
      lcd.setCursor(8, 1);
      if (pumpSpeed < 10) {
        lcd.print(F("0")); // Print a leading zero
      }
      lcd.print(pumpSpeed);
      prevPumpSpeed = pumpSpeed;
    }
    lastExecutionTime = millis();
  }
}

// Update stirrer speed based on potentiometer value
void updateStirrerSpeed()
{
  potValueStirr = analogRead(potStirrPin);
  if (abs(potValueStirr - lastPotValueStirr) > 2) { // Only update if change is significant
    stirrerSpeed = map(potValueStirr, 5, 815, 10, 120);
    stirrStepper.setMaxSpeed(ceil(stirrerSpeed*stepsPerRevolution/60));
    lastPotValueStirr = potValueStirr;
  }
}

// Update pump speed based on potentiometer value
void updatePumpSpeed()
{ 
  potValuePump = analogRead(potPumpPin); 
  // Map potentiometer value (0-820) to pump speed (1-10 RPS) 
  if (abs(potValuePump - lastPotValuePump) > 2) { // Only update if change is significant
    pumpSpeed = map(potValuePump, 5, 815, 1, 10);
    pumpStepper.setMaxSpeed(pumpSpeed*stepsPerRevolution);
    lastPotValuePump = potValuePump;
  }
}

// void startDosing(bool mode)
// {
//   if (!mode)
//   {
//     pumpStepper.setAcceleration(30000);
//     pumpStepper.move(stepsPerRevolution);
//   }else{
//     pumpStepper.setAcceleration(200);
//     pumpStepper.move(100000);
//   }
// }

// Run both Steppers
inline void runSteppers()
{
  stirrStepper.run();
  if (isDosing || (pumpStepper.distanceToGo() != 0) && (switchState == false)){
    if (switchState){
      pumpStepper.runSpeed();
    }else{
      if (pumpStepper.distanceToGo() == 0){
        isDosing = false;
      }else{
        pumpStepper.run();
      }
    }
  }
}

// Function for debouncing button presses
inline bool debounceButton(int buttonPin, unsigned long& lastDebounceTime)
{ 
  unsigned long currentTime = millis();
  const long debounceDelay = 50;  // Debounce delay in milliseconds

  if (digitalRead(buttonPin) == LOW && currentTime - lastDebounceTime > debounceDelay) 
  {
    lastDebounceTime = currentTime;
    return true;
  }
return false;
}
