# SmartBlinds

 Smart module for pull string louver blinds.

Dependencies:  

- [arduino-libraries/Servo@^1.1.8](https://github.com/arduino-libraries/Servo)  
- [madhephaestus/ESP32Servo@^0.13.0](https://github.com/madhephaestus/ESP32Servo)  
- [tzapu/WifiManger@^2.0.15](https://github.com/tzapu/WiFiManager)  
- [bblanchon/ArduinoJson@^6.21.1](https://github.com/bblanchon/ArduinoJson)
- [khoih-prog/ESP_DoubleResetDetector@^1.3.2](https://github.com/khoih-prog/ESP_DoubleResetDetector)  
- [bblanchon/ArduinoJson@^6.21.2](https://github.com/bblanchon/ArduinoJson)
- [knolleary/PubSubClient@^2.8](https://github.com/knolleary/pubsubclient)

Stats:  

- Servo Pin: ESP32 GPIO Pin 13  
- Configuration Button Pin: 34  
- Serial Baud Rate: 115200  

Notes:  

- After every movement, plus a delay (servoDelay in milliseconds), the servo is detached to save power and avoid vibration.
- MQTT Password maximum length is 32 characters.
- Added 'configuration.yaml' snippet file for manual adoption into Home Assistant until MQTT auto-discovery is completed.

Roadmap:  

- Software:  
  - Complete:  
    - Servo Control w/ Ranged Input: Convert 0-100 dimmer percentage input to 0-160 servo range of movement.  
    - Serial IO: Allow for serial input of dimmer value as well as various debugging and status output.  
    - Wifi Connectivity: Allow for wifi configuration via temporary SSID.  
    - Reset Button: External button to reset all wifi configuration. (May be removed after development).  
    - Config Button: Double press ESP32 on-board reset button to enter enable configuration via temporary SSID.
    - Persistent Configuration Memory: Enable settings & configuration to persist through power loss. (Done via SPIFFS).  
    - MQTT Implementation: Enable IO via MQTT (enables Home Assistant integration).  \
    - Input sanitization
    - HomeAssistant/MQTT auto-discovery
  - To-Do:  
    - Battery voltage/percentage feedback
    - 2-way HomeAssistant feedback
- Hardware:
  - Complete:
    - Initial schematic design including solar charging with battery reporting.
  - To-Do:
    - Test force and travel distance for actuation. Choose appropriate servo
    - Integrate Config/reset button into schematic.  
    - Design enclosure.
    - Design armature & affixation method.
- Documentation Et. Al.:
  - Complete:
    - Readme
  - To-Do:
    - Add Hardware and assembly section to docs
    - Complete & upload schematic
    - CI/CD
    - Polish Readme (Badges, etc.)
