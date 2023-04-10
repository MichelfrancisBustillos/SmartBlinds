#include <Arduino.h>
#include <ESP32Servo.h>
#include <WiFiManager.h>

#define ESP_DRD_USE_SPIFFS true  //define storage type for DoubleResetDetector. Must be done before header inclusion

#include <ESP_DoubleResetDetector.h>

//define constants and pinouts
#define SERVOPIN 13 //GPIO pin for servo data
#define LED_BUILTIN 2 //Pin for onboard LED
#define CONFIGBUTTON 34 //GPIO pin for config button
#define DRD_ADDRESS 0 //RTC Memory Address for the DoubleResetDetector to use

//define globel variables
int servoDelay = 700; //Delay between servo movements
int configTimeout = 300; //Timeout for wifi portal mode
int drdTimeout = 10; //Timeout for DoubleResetDetector
Servo servo1; //Create servo object
WiFiManager wifi; //Create wifimanager object
DoubleResetDetector* drd; //Create DoubleResetDetector object

//WiFiManager Custom Parameters
WiFiManagerParameter deviceName("device_name", "Device Name:", "Smart Blinds", 15); //hostname

//define methods
int rangeConversion(int);
void serialDimmerIO();
void checkConfigButton();
void saveParamsCallback();

void setup() {
  Serial.begin(115200); //open serial coms
  pinMode(LED_BUILTIN, OUTPUT); //define onboard ESP32 LED for use as status indicator
  pinMode(CONFIGBUTTON, INPUT_PULLDOWN); //define pin for user reste button input

  //Initiale Servo
  servo1.attach(SERVOPIN);
  servo1.write(0); //set servo to '0' starting position
  delay(servoDelay);
  servo1.detach();
  Serial.println("Servo initilized.");

  //Initialize Wifi
  bool forceConfig = false;
  drd = new DoubleResetDetector(drdTimeout, DRD_ADDRESS);
  if (drd -> detectDoubleReset()){
    Serial.println("Double reset detected...");
    forceConfig = true;
  }
  WiFi.mode(WIFI_STA); //Set ESP32 to 'station mode (wifi client) for desired end state
  wifi.addParameter(&deviceName);
  //wifi.setConfigPortalBlocking(false);
  wifi.setConfigPortalTimeout(configTimeout);
  wifi.setSaveParamsCallback(saveParamsCallback);

  if(forceConfig) {  //If reset button doulbe pressed
    if(!wifi.startConfigPortal("ESP32_Setup")) {  //Start config portal and wait for connection or timeout
      Serial.println("failed to connect and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.restart();
        delay(5000);
    }
  } else {
    if(!wifi.autoConnect("ESP32_Setup")) {  //Attempt autoConnect, on fail open portal and wait for connection or timeout
      Serial.println("failed to connect and hit timeout");
        delay(3000);
        // if we still have not connected restart and try all over again
        ESP.restart();
        delay(5000);
    } else {
      Serial.println("Connected to wifi.");
    }
  }
    Serial.println("Setup complete...");
}

void loop() {
  wifi.process();
  digitalWrite(LED_BUILTIN, WiFi.status() == WL_CONNECTED); //Turn on onboard LED as wifi status indicator
  checkConfigButton();
  serialDimmerIO();
  drd->loop();
}

int rangeConversion(int dimmer){
  int servoPosition = 0;
  int servoMin = 0; //minimum value for servo movement
  int servoMax = 160; //max value for servo movement
  int dimmerMin = 0;
  int dimmerMax = 100;
  int servoRange = (servoMax - servoMin); //servo range of motion
  int dimmerRange = (dimmerMax - dimmerMin);

  servoPosition = (((dimmer - dimmerMin) * servoRange) / dimmerRange) + servoMin; //convert dimmer percentage to servo position within min/max range

  if (servoPosition < servoMin){ //If requested position is less than minimum, set to minimum.
    servoPosition = servoMin;
  } else if (servoPosition > servoMax){ //If requested position is greater than maximum, set to maximum.
    servoPosition = servoMax;
  }

  return servoPosition;
}

void serialDimmerIO(){
  String userInput = "";
  if(Serial.available()){
    userInput = Serial.readStringUntil('\n');
    int dimmer = userInput.toInt();
    int servoPosition = rangeConversion(dimmer); //Convert dimmer percentage input into servo steps
    //int servoPosition = dimmer; //Bypass rangeConversion for debugging
    Serial.print("Dimmer: ");
    Serial.print(dimmer);
    Serial.print(" Servo: ");
    Serial.println(servoPosition);
    servo1.attach(SERVOPIN);
    servo1.write(servoPosition);
    delay(servoDelay);
    servo1.detach();
  }
  return;
}

void checkConfigButton(){
  if (digitalRead(CONFIGBUTTON) == HIGH){
    Serial.println("Config button pressed.");
    wifi.resetSettings(); //Reset all wifi settings
    ESP.restart();
  }
}

void saveParamsCallback(){
    Serial.println("Get Params: ");
    Serial.print(deviceName.getID());
    Serial.print(": ");
    Serial.println(deviceName.getValue());
    WiFi.setHostname(deviceName.getValue());
}