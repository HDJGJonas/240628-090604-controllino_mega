#include <Arduino.h>

#include <AccelStepper.h>
#include <MultiStepper.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <SPI.h>
#include <Controllino.h>

const int stepsPerRevolution = 400;

const int enaStirrPin = 2; 
const int dirStirrPin = 3;
const int stepStirrPin = 4;

const int enaPumpPin = 5;
const int dirPumpPin = 6;
const int stepPumpPin = 7;
int pumpStepperTarget = stepsPerRevolution;

AccelStepper stirrStepper = AccelStepper(1, stepStirrPin, dirStirrPin);
AccelStepper pumpStepper = AccelStepper(1, stepPumpPin, dirPumpPin);

void setup()
{  
    pinMode(enaStirrPin, OUTPUT);
    pinMode(dirStirrPin, OUTPUT); 
    pinMode(stepStirrPin, OUTPUT);
    digitalWrite(enaStirrPin, LOW);

    pinMode(enaPumpPin, OUTPUT);
    pinMode(dirPumpPin, OUTPUT);
    pinMode(stepPumpPin, OUTPUT);
    digitalWrite(enaPumpPin, LOW);

    stirrStepper.setMaxSpeed(800);
    stirrStepper.setAcceleration(200);
    stirrStepper.move(1000000000);

    pumpStepper.setMaxSpeed(4000);
    pumpStepper.setAcceleration(2000);
    pumpStepper.move(pumpStepperTarget);
}

void loop()
{
    stirrStepper.run();
    pumpStepper.run();

    if (digitalRead(A9) == HIGH && pumpStepper.distanceToGo() == 0)
    {   
        pumpStepper.move(pumpStepperTarget);
    }
}
