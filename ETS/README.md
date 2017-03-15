# ETS

Electronic Toggle Switch based on ESP8266

## Install

Make necessary changes for PIN assignment, compile, and flash onto your device using Arduino IDE.

## Usage

Once flashed on the device, your device will boot on AP mode. Connect to this AP using your smart phone or laptop.
You will be prompted to connect your device to your WiFi router. Select your WiFi SSID, enter your WiFi password, 
enter a name for your device and save to connect to WiFi.

Once connected to your WiFi network, the switch can be controlled (toggled) over HTTP using following endpoint.

    GET http://<device-ip>/switch/on
    
Note: you can find the device IP address from the Arduino serial monitor while connecting to WiFi. Or you can always perform a 
SSDP m-search and discover this device in your network.

Manual operation is also supported using a button.

- Short single press toggles the relay.
- 3 seconds long press will restart the device.
- 10 seconds long press will reset and restart the device. You will loose your wifi connection and will have to re-connect.