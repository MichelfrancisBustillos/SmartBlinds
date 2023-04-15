#include <Arduino.h>
#include <ESP32Servo.h>
#include <WiFiManager.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#define ESP_DRD_USE_SPIFFS true  //define storage type for DoubleResetDetector. Must be done before header inclusion

#include <ESP_DoubleResetDetector.h>

//define constants and pinouts
#define SERVOPIN 13 //GPIO pin for servo data
#define LED_BUILTIN 2 //Pin for onboard LED
#define CONFIGBUTTON 34 //GPIO pin for config button
#define DRD_ADDRESS 0 //RTC Memory Address for the DoubleResetDetector to use
#define JSON_CONFIG "/config.json" //Location for config file

//define globel variables
int servoDelay = 700; //Delay between servo movements
int configTimeout = 300; //Timeout for wifi portal mode
int drdTimeout = 10; //Timeout for DoubleResetDetector
bool saveConfig = false; //Tag if configuration should be saved
Servo servo1; //Create servo object
WiFiManager wifi; //Create wifimanager object
DoubleResetDetector* drd; //Create DoubleResetDetector object
WiFiClient espClient;
PubSubClient client(espClient);

//WiFiManager Custom Parameters
char deviceName_String[15] = "SmartBlinds";
WiFiManagerParameter deviceName("device_name", "Device Name:", deviceName_String, 15); //hostname
char mqttServer_String[15] = "0.0.0.0";
WiFiManagerParameter mqttServer("mqttServer", "MQTT Server IP:", mqttServer_String, 15); //MQTT Server IP
char mqttUsername_String[23] = "";
WiFiManagerParameter mqttUsername("mqttUsername", "MQTT Username:", mqttUsername_String, 23); //MQTT Username
char mqttPassword_String[32] = "";
WiFiManagerParameter mqttPassword("mqttPassword", "MQTT Password:", mqttPassword_String, 32); //MQTT Password

//define methods
int rangeConversion(int);
void serialDimmerIO();
void checkConfigButton();
void saveParamsCallback();
void saveConfigFile();
bool loadConfigFile();
void callback(char*, byte*, unsigned int);
void reconnect();

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

  bool spiffsSetup = loadConfigFile();
  if (!spiffsSetup){
    Serial.println("Forcing config mode as there is no saved config!");
    forceConfig = true;
  }

  drd = new DoubleResetDetector(drdTimeout, DRD_ADDRESS);
  if (drd -> detectDoubleReset()){
    Serial.println("Double reset detected...");
    forceConfig = true;
  }
  WiFi.mode(WIFI_STA); //Set ESP32 to 'station mode (wifi client) for desired end state
  wifi.addParameter(&deviceName);
  wifi.addParameter(&mqttServer);
  wifi.addParameter(&mqttUsername);
  wifi.addParameter(&mqttPassword);
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

  //Initialize MQTT
  Serial.println(mqttServer_String);
  client.setServer(mqttServer_String, 1883);
  client.setCallback(callback);

  Serial.println("Setup complete...");
}

void loop() {
  wifi.process();
  digitalWrite(LED_BUILTIN, WiFi.status() == WL_CONNECTED); //Turn on onboard LED as wifi status indicator
  checkConfigButton();
  serialDimmerIO();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
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

    if (dimmer >= 0 && dimmer <= 100){
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
    } else {
      Serial.println("Invalid dimmer value!");
    }
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

    //Pull, print and save deivceName
    Serial.print(deviceName.getID());
    Serial.print(": ");
    Serial.println(deviceName.getValue());
    WiFi.setHostname(deviceName.getValue());
    strncpy(deviceName_String, deviceName.getValue(), sizeof(deviceName_String));

    //Pull, print and save mqttServer
    Serial.print(mqttServer.getID());
    Serial.print(": ");
    Serial.println(mqttServer.getValue());
    strncpy(mqttServer_String, mqttServer.getValue(), sizeof(mqttServer_String));

    //Pull, print and save mqttUsername
    Serial.print(mqttUsername.getID());
    Serial.print(": ");
    Serial.println(mqttUsername.getValue());
    strncpy(mqttUsername_String, mqttUsername.getValue(), sizeof(mqttUsername_String));

    //Pull, print and save mqttPassword
    Serial.print(mqttPassword.getID());
    Serial.print(": ");
    Serial.println(mqttPassword.getValue());
    strncpy(mqttPassword_String, mqttPassword.getValue(), sizeof(mqttPassword_String));

    saveConfigFile();
}

void saveConfigFile(){
  Serial.println("Saving configuration...");
  StaticJsonDocument<512> json;
  json["deviceName"] = deviceName_String;
  json["mqttServer"] = mqttServer_String;
  json["mqttUsername"] = mqttUsername_String;
  json["mqttPassword"] = mqttPassword_String;

  File configFile = SPIFFS.open(JSON_CONFIG, "w"); //Open config file
  if (!configFile){ //Check if file opened
    Serial.println("Failed to open config file for writing!"); //Error: Config file did not open
  }

  serializeJsonPretty(json, Serial); //Serialize JSON data
  if (serializeJson(json, configFile) == 0){ //Check if file serialized
    Serial.println("Failed to write to file!"); //Error: File unable to serialize for writing
  }

  configFile.close();
}

bool loadConfigFile(){
  Serial.println("Mounting file system...");

  if (SPIFFS.begin(false) || SPIFFS.begin(true)){ //Check if FS mounted
    Serial.println("Mounted file system."); //FS mounted successfully
    if (SPIFFS.exists(JSON_CONFIG)){ //Check if config file exists
      Serial.println("Reading config file...");
      File configFile = SPIFFS.open(JSON_CONFIG, "r"); //Open config file for reading
      if (configFile){
        Serial.println("Opened config file.");
        StaticJsonDocument<512> json;
        DeserializationError error = deserializeJson(json, configFile);
        serializeJsonPretty(json, Serial);
        if (!error){ //If no error...
          Serial.println("Parsing JSON...");
          strcpy(deviceName_String, json["deviceName"]);
          strcpy(mqttServer_String, json["mqttServer"]);
          strcpy(mqttUsername_String, json["mqttUsername"]);
          strcpy(mqttPassword_String, json["mqttPassword"]);

          return true;
        } else {
          Serial.println("Failed to load JSON config!"); //Error: Unable to load JSON config
        }
      }
    }
  } else {
    Serial.println("Failed to mount file system!");
  }

  return false;
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(deviceName_String, mqttUsername_String, mqttPassword_String)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic","hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}