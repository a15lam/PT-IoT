//================================================================================================
// Power Adapter Socket based on ESP8266
//================================================================================================

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266SSDP.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <DNSServer.h>    
#include <Secrets.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <Bounce2.h>

//================================================================================================
// Defined constants
//================================================================================================
#define SERIAL_NUM "00001"
#define DEVICE_PREFIX "AI"
#define DEVICE_TYPE "SOCKET"
#define DEVICE_MODEL "Socket"
#define DEVICE_MODEL_NUM "SO001"
//================================================================================================

//================================================================================================
// Global variables, callbacks and functions
//================================================================================================
char OTA_HOST_NAME[100];
char DEVICE_NAME[51];
bool saveConfig = false;
const int relay = 12;
const int led = 13;
const int button = 0;
int ledReversed = 1;          // If LED is reversed use 1 or else use 0
int relayState = LOW;         // the current state of the relay

//========================[ MQTT Begin ]========================
WiFiClientSecure espClient;
PubSubClient client(espClient);

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    relayOn();
  } else {
    relayOff();
  }
}

void reconnect(String clientName) 
{
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Client ID:");
    Serial.println(clientName.c_str());
    Serial.print("Attempting MQTT connection...");
   
    // Attempt to connect
    if (client.connect(clientName.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
      blinkLed(3, true);

      String controlTopic = getControlTopic();
      int controlTopicLength = controlTopic.length() + 1;
      char controlTopicChar[controlTopicLength];
      controlTopic.toCharArray(controlTopicChar, controlTopicLength); 
      
      String payload = getRegisterPayload(controlTopic);
      // Once connected, publish an announcement...
      Serial.print("Payload:");
      Serial.println(payload);
      
      int payloadLength = payload.length() + 1;
      char payloadChar[payloadLength];
      payload.toCharArray(payloadChar, payloadLength);
     
      String registerTopic = getRegisterTopic();
      int registerTopicLength = registerTopic.length() + 1;
      char registerTopicChar[registerTopicLength];
      registerTopic.toCharArray(registerTopicChar, registerTopicLength);
      
      // NOTE: Change MQTT_MAX_PACKET_SIZE in PubSubClient.h to 256 (from 128). Otherwise publish won't work. 
      client.publish(registerTopicChar, payloadChar);
      
      Serial.print("Subscribing to topic:");
      Serial.println(controlTopicChar);
      client.subscribe(controlTopicChar);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}
String getMacAddress()
{
  uint8_t mac[6];
  WiFi.macAddress(mac);
  return macToStr(mac);
}

String getControlTopic() 
{
  String mac = getMacAddress();
  String topic = String(DEVICE_PREFIX) + "/" + String(DEVICE_TYPE) + "/" + String(DEVICE_MODEL_NUM) + "/" + mac;
  
  return topic;
}

String getRegisterTopic()
{
  String topic = String(DEVICE_PREFIX) + "/Register";

  return topic;
}

String getRegisterPayload(String controlTopic) 
{
  String mac = getMacAddress();
  IPAddress localIp = WiFi.localIP();
  char bufIp[16];
  sprintf(bufIp, "%d.%d.%d.%d", localIp[0], localIp[1], localIp[2], localIp[3] );
  String payload = "{\"name\":\"" + String(DEVICE_NAME) + 
                    "\",\"prefix\":\"" + String(DEVICE_PREFIX) + 
                    "\",\"type\":\"" + String(DEVICE_TYPE) + 
                    "\",\"model\":\"" + String(DEVICE_MODEL_NUM) + 
                    "\",\"mac\":\"" + mac + 
                    "\",\"lan_ip\":\"" + String(bufIp) + 
                    "\",\"sub_topic\":\"" + controlTopic + "\"}";

  return payload;
}

String mqttClientName;
//========================[ MQTT End ]========================

Bounce toggle = Bounce();
Bounce restart = Bounce();
Bounce reset = Bounce();

void saveConfigCallback () 
{
  Serial.println("should save config");
  saveConfig = true;
}

void configModeCallback (WiFiManager *myWiFiManager) 
{
  writeLed(HIGH);
}

void blinkLed(int times, bool fast)
{
  while(times > 0){
    writeLed(HIGH);
    if(fast == true){
      delay(100);
    } else {
      delay(300);
    }
    writeLed(LOW);
    if(fast == true){
      delay(100);
    } else {
      delay(300);
    }
    times--;  
  }
}

void writeLed(int flag) 
{
  if(ledReversed == 1) {
    digitalWrite(led, !flag);
  } else {
    digitalWrite(led, flag);
  }
}

void relayOn() 
{
  writeLed(HIGH);
  digitalWrite(relay, HIGH);
  relayState = HIGH;
}

void relayOff() 
{
  writeLed(LOW);
  digitalWrite(relay, LOW);
  relayState = LOW;
}

ESP8266WebServer HTTP(80);
WiFiManager wifiManager;
//================================================================================================


//================================================================================================
// Setup function
//================================================================================================
void setup() 
{
  //----------------------------------------------------------------------------------------------
  // Initialize serial communication
  //
  Serial.begin(115200);
  Serial.println();
  //----------------------------------------------------------------------------------------------
  
  //----------------------------------------------------------------------------------------------
  // Initialize LED pin
  //
  pinMode(led, OUTPUT);
  writeLed(LOW);
  //----------------------------------------------------------------------------------------------

  //----------------------------------------------------------------------------------------------
  // Read config from File System (FS) json
  //
  Serial.println("mounting FS...");
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          strcpy(DEVICE_NAME, json["device_name"]);
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //----------------------------------------------------------------------------------------------

  //----------------------------------------------------------------------------------------------
  // Initialize WiFi Manager
  //
  Serial.println("starting WiFi Manager...");
  WiFiManagerParameter device_name_param("device_name", "Device Name", DEVICE_NAME, 50);
  wifiManager.addParameter(&device_name_param);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setAPCallback(configModeCallback);

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // and goes into a blocking loop awaiting configuration
  char AutoAPName[30];
  sprintf(AutoAPName, "%s-%s-%s", DEVICE_PREFIX, DEVICE_TYPE, SERIAL_NUM);
  wifiManager.autoConnect(AutoAPName, AP_PASS);

  Serial.println("Connected! Local IP");
  Serial.println(WiFi.localIP());
  writeLed(LOW);

  //========================[ MQTT Begin ]========================
  if (!espClient.connect(MQTT_BROKER, MQTT_PORT)) {
    Serial.println("connection failed");
    //return;
  }
  if (espClient.verify(MQTT_X509_FINGERPRINT, MQTT_BROKER)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(mqttCallback);
  
  mqttClientName += "esp8266-";
  mqttClientName += getMacAddress();
  mqttClientName += "-";
  mqttClientName += String(micros() & 0xff, 16);
  //=========================[ MQTT End ]=========================

  strcpy(DEVICE_NAME, device_name_param.getValue());
  // save the custom parameters to FS
  if (saveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["device_name"] = DEVICE_NAME;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
  }
  //----------------------------------------------------------------------------------------------

  //----------------------------------------------------------------------------------------------
  // Initialize Over The Air (OTA) update feature
  //
  sprintf(OTA_HOST_NAME, "%s-%s-%s-%s", DEVICE_PREFIX, DEVICE_TYPE, SERIAL_NUM, DEVICE_NAME);
  ArduinoOTA.setHostname(OTA_HOST_NAME);
  ArduinoOTA.setPassword(OTA_PASS);
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  //----------------------------------------------------------------------------------------------

  //----------------------------------------------------------------------------------------------
  // Initialize relay, button, and bouncer.
  //
  pinMode(relay, OUTPUT);  //Relay pin
  pinMode(button, INPUT);  //Button
 
  toggle.interval(100);
  toggle.attach(button);
  restart.interval(3000);
  restart.attach(button);
  reset.interval(9000);
  reset.attach(button);
  //----------------------------------------------------------------------------------------------

  //----------------------------------------------------------------------------------------------
  // Setup HTTP operation APIs
  //
  
  Serial.println("starting HTTP...");
  HTTP.on("/switch/on", HTTP_GET, [](){
    relayOn();
    HTTP.send(200, "application/json", "{\"switch\":1}");
  });
  HTTP.on("/switch/off", HTTP_GET, [](){
    relayOff();
    HTTP.send(200, "application/json", "{\"switch\":0}");
  });
  HTTP.on("/switch/state", HTTP_GET, [](){
    int val = digitalRead(relay);
    if(val == 1){
      HTTP.send(200, "application/json", "{\"switch\":1}");
    } else {
      HTTP.send(200, "application/json", "{\"switch\":0}");
    }
  });
  HTTP.on("/description.xml", HTTP_GET, [](){
    SSDP.schema(HTTP.client());
  });
  HTTP.begin();
  //----------------------------------------------------------------------------------------------

  //----------------------------------------------------------------------------------------------
  // Setup SSDP feature
  //
  Serial.println("starting SSDP...");
  SSDP.setSchemaURL("description.xml");
  SSDP.setHTTPPort(80);
  SSDP.setDeviceType("urn:peach:device:Basic:1");
  SSDP.setName(DEVICE_NAME);
  SSDP.setSerialNumber(SERIAL_NUM);
  SSDP.setURL("index.html");
  SSDP.setModelName(DEVICE_MODEL); 
  SSDP.setModelNumber(DEVICE_MODEL_NUM);
  SSDP.setModelURL("https://www.peach-tech.com");
  SSDP.setManufacturer("PeachTech");
  SSDP.setManufacturerURL("https://www.peach-tech.com");
  SSDP.begin();
  //----------------------------------------------------------------------------------------------

  Serial.println("Ready!");
  blinkLed(3, false);
}
//================================================================================================


//================================================================================================
// Loop function
//================================================================================================
void loop() 
{
  //----------------------------------------------------------------------------------------------
  // MQTT connection handling
  
  if (!client.connected()) {
    reconnect(mqttClientName);
  } else {
    client.loop();
  }
  //----------------------------------------------------------------------------------------------
  
  //----------------------------------------------------------------------------------------------
  // OTA handler
  ArduinoOTA.handle();
  //----------------------------------------------------------------------------------------------

  //----------------------------------------------------------------------------------------------
  // HTTP handler
  HTTP.handleClient();
  //----------------------------------------------------------------------------------------------

  //----------------------------------------------------------------------------------------------
  // Manual button operation
  //

  toggle.update();
  restart.update();
  reset.update();

  if(reset.fell()){
    Serial.println("resetting device...");
    blinkLed(10, true);
    //clean FS, for testing
    SPIFFS.format();
    wifiManager.resetSettings();
    ESP.restart();
  }

  if(restart.fell()){
    // restart triggered.
    writeLed(HIGH);
  }
  if(restart.rose()){
    Serial.println("restarting device...");
    blinkLed(5, true);
    ESP.restart();
  }
  
  if(toggle.rose()){
    relayState = !relayState;
    if(relayState == LOW){
      Serial.println("toggling device Off");
      relayOff();
    } else {
      Serial.println("toggling device On");
      relayOn();
    }
  }
  //----------------------------------------------------------------------------------------------
}
//================================================================================================

