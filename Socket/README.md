# Socket

Power Adapter Socket based on ESP8266

## Install

Create a <code>Credential.h</code> file in your Arduino IDE setup at Arduino/libraries/Credential directory and define the following in it.
    
    #define OTA_PASS "secret"
    #define AP_PASS "secret"

Or alternatively, comment out line 13: <code>#include <Credential.h></code> and define the two passwords above in the code.
The <code>OTA_PASS</code> is the password for performing Over The Air (WiFi) firmware update. And <code>AP_PASS</code> is the 
password for connecting to the device in AP mode and configure it to connect to your WiFi SSID.

Make necessary changes for PIN assignment, compile, and flash onto your device using Arduino IDE.

## Usage

Once flashed on the device, your device will boot on AP mode. Connect to this AP using your smart phone or laptop.
You will be prompted to connect your device to your WiFi router. Select your WiFi SSID, enter your WiFi password, 
enter a name for your device and save to connect to WiFi.

Once connected to your WiFi network, it can be controlled over HTTP using following endpoints.

    GET http://<device-ip>/switch/on
    GET http://<device-ip>/switch/off
    GET http://<device-ip>/switch/state
    
Note: you can find the device IP address from the Arduino serial monitor while connecting to WiFi. Or you can always perform a 
SSDP m-search and discover this device in your network.

Manual operation is also supported using a button.

- Short single press toggles the relay.
- 3 seconds long press will restart the device.
- 10 seconds long press will reset and restart the device. You will loose your wifi connection and will have to re-connect.