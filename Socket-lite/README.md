# Socket

Power Adapter Socket based on ESP8266

## Install

Create a <code>Credential.h</code> file in your Arduino IDE setup at Arduino/libraries/Credential directory and define the following in it.
    
    #define OTA_PASS "secret"
    #define AP_PASS "secret"
    #define MQTT_BROKER "mqtt.example.com"
    #define MQTT_PORT 1883
    #define MQTT_USER "username"
    #define MQTT_PASS "password"
    #define MQTT_X509_FINGERPRINT "XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX"


Or alternatively, comment out line 14: <code>#include <Credential.h></code> and define the configuration above in the code.
The <code>OTA_PASS</code> is the password for performing Over The Air (WiFi) firmware update. And <code>AP_PASS</code> is the 
password for connecting to the device in AP mode and configure it to connect to your WiFi SSID. Rest of the configurations are self-explanatory.

Make necessary changes for PIN assignment, compile, and flash onto your device using Arduino IDE.

## Usage

Once flashed on the device, your device will boot on AP mode. Connect to this AP using your smart phone or laptop.
You will be prompted to connect your device to your WiFi router. Select your WiFi SSID, enter your WiFi password, 
enter a name for your device and save to connect to WiFi.

Once connected to your WiFi network, it can be controlled over HTTP using following endpoints.

    GET http://<device-ip>/switch/on
    GET http://<device-ip>/switch/off
    GET http://<device-ip>/switch/state
    
Note: you can find the device IP address from the Arduino serial monitor while connecting to WiFi. 
If MQTT is configured then it will subscribe to topic "<DEVICE_PREFIX>/<DEVICE_TYPE>/<DEVICE_MODEL_NUM>/<mac_address>". Example: /PT/SOCKET/SO001/aa.bb.cc.dd.ee.ff

Manual operation is also supported using a button.

- Short single press toggles the relay.
- 3 seconds long press will restart the device.
- 10 seconds long press will reset and restart the device. You will loose your wifi connection and will have to re-connect.
