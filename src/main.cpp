#include <Arduino.h>
#include <ESP32Servo.h>

const int servoPin = 13; //GPIO pin for servo data
int servoDelay = 700; //Delay between servo movements
Servo servo1;

//define methods
int rangeConversion(int);
void serialIO();

void setup() {
  Serial.begin(115200);

  servo1.attach(servoPin);
  servo1.write(0);
  delay(servoDelay);
  servo1.detach();

  Serial.println("Ready...");
}

void loop() {
  serialIO();
}

int rangeConversion(int dimmer){
  int servoPosition = 0;
  int servoMin = 0;
  int servoMax = 160;
  int dimmerMin = 0;
  int dimmerMax = 100;
  int servoRange = (servoMax - servoMin);
  int dimmerRange = (dimmerMax - dimmerMin);

  servoPosition = (((dimmer - dimmerMin) * servoRange) / dimmerRange) + servoMin;

  return servoPosition;
}

void serialIO(){
    String userInput = "";
    if(Serial.available()){
    userInput = Serial.readStringUntil('\n');
    int dimmer = userInput.toInt();
    int servoPosition = rangeConversion(dimmer); //Convert dimmer percentage input into servo steps
    //int servoPosition = dimmer; //Bypass rangeConversion for debugging
    Serial.println("Servo to ");
    Serial.print(servoPosition);
    servo1.attach(servoPin);
    servo1.write(servoPosition);
    delay(servoDelay);
    servo1.detach();
  }
  return;
}