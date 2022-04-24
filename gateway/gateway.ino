  #include "BLEDevice.h"
  #include <BLEServer.h>
  #include <BLEUtils.h>
  #include <Wire.h>

/*** WiFI INCLUDE BEGIN ***/
#include <WiFi.h>
extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
}
#include <AsyncMqttClient.h>  
/*** WiFI INCLUDE BEGIN END ***/

/*** BLE DEFINE BEGIN ***/
  #define temperatureCelsius
  #define bleServerName11 "ESP32_Server_11"
  #define bleServerName22 "ESP32_Server_22"

  /* UUID's of the service, characteristic that we want to read*/
  // BLE Service
  static BLEUUID dhtServiceUUID("91bad492-b950-4226-aa2b-4ede9fa42f59");
  
  // BLE Characteristics
  #ifdef temperatureCelsius
  //Temperature Celsius Characteristic
  static BLEUUID temperatureCharacteristicUUID("cba1d466-344c-4be3-ab3f-189f80dd7518");
  #else
  //Temperature Fahrenheit Characteristic
  static BLEUUID temperatureCharacteristicUUID("f78ebbff-c8b7-4107-93de-889a6a06d408");
  #endif
  // Humidity Characteristic
  static BLEUUID humidityCharacteristicUUID("ca73b3ba-39f6-4ab3-91ae-186dc9577d99");
  
  // cờ kiểm tra xem client có đang kết nối với server hay không
  static boolean doConnect = false;
  static boolean connected = false;
  
  // biến kiểu BLEAddress trỏ đến địa chỉ máy chủ mà ta muốn kết nối. Địa chỉ này sẽ đc tìm thấy trong quá trình quét
  static BLEAddress *pServerAddress;

  // Đặt các characteristic muốn đọc
  static BLERemoteCharacteristic* temperatureCharacteristic;
  static BLERemoteCharacteristic* humidityCharacteristic;

  // mảng thông báo
  const uint8_t notificationOn[] = {0x1, 0x0};
  const uint8_t notificationOff[] = {0x0, 0x0};
  
  //Variables to store temperature and humidity
  char* temperatureChar;
  char* humidityChar;

  //Flags to check whether new temperature and humidity readings are available
  boolean newTemperature = false;
  boolean newHumidity = false;

  // tao ra client
  BLEClient* pClient = BLEDevice::createClient();

  // biến đếm số lần đọc cảm biến
  static uint8_t count;
/*** BLE DEFINE END ***/

 /*** WiFi DEFINE BEGIN ***/
#define WIFI_SSID "HIEN MANH"
#define WIFI_PASSWORD "tuanmanh123"

// Raspberry Pi Mosquitto MQTT Broker
//#define MQTT_HOST IPAddress(192, 168, 1, 11)
// For a cloud MQTT broker, type the domain name
#define MQTT_HOST "broker.hivemq.com"
#define MQTT_PORT 1883

// Temperature MQTT Topics
#define MQTT_PUB_TEMP_22 "esp32/node2/temperature"
#define MQTT_PUB_HUM_22  "esp32/node2/humidity"
#define MQTT_PUB_TEMP_11 "esp32/node1/temperature"
#define MQTT_PUB_HUM_11  "esp32/node1/humidity"


AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 10000;        // Interval at which to publish sensor readings
/*** WIFI DEFINE END ***/

/*** BLE FUNCTION BEGIN ***/
  //Connect to the BLE Server that has the name, Service, and Characteristics
  bool connectToServer(BLEAddress pAddress) {
  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(dhtServiceUUID);
  // Obtain a reference to the characteristics in the service of the remote BLE server.
  temperatureCharacteristic = pRemoteService->getCharacteristic(temperatureCharacteristicUUID);
  humidityCharacteristic = pRemoteService->getCharacteristic(humidityCharacteristicUUID);
  //Assign callback functions for the Characteristics  
  temperatureCharacteristic->registerForNotify(temperatureNotifyCallback);
  humidityCharacteristic->registerForNotify(humidityNotifyCallback);
  return true;
  }

//Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == "ESP32_Server_11") //Check if the name of the advertiser matches
    { 
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
      doConnect = true; //Set indicator, stating that we are ready to connect
      Serial.println("Device DHT11 found. Connecting!");
    }
    else if (advertisedDevice.getName() == "ESP32_Server_22")
    {
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
      doConnect = true; //Set indicator, stating that we are ready to connect
      Serial.println("Device DHT22 found. Connecting!");      
    }
    else
    {
      Serial.println("Cant found device...");
    }
  }
};

//When the BLE Server sends a new temperature reading with the notify property
static void temperatureNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                        uint8_t* pData, size_t length, bool isNotify) {
  //store temperature value
  temperatureChar = (char*)pData;
  newTemperature = true;
  Serial.print(temperatureChar);
  // publishing on MQTT topic:
  uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP_22, 1, true, String(temperatureChar).c_str());                            
  Serial.printf("\nPublishing on topic %s at QoS 1, packetId: %i", MQTT_PUB_TEMP_22, packetIdPub1);
  Serial.printf("Message: %s \n", temperatureChar);  
}
  
//When the BLE Server sends a new humidity reading with the notify property
static void humidityNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                    uint8_t* pData, size_t length, bool isNotify) {
  //store humidity value
  humidityChar = (char*)pData;
  newHumidity = true;
  Serial.println(humidityChar);
  // Publish an MQTT message on topic esp32/dht/humidity
  uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_HUM_22, 1, true, String(humidityChar).c_str());                            
  Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_HUM_22, packetIdPub2);
  Serial.printf("Message: %s \n", humidityChar);
}
/*** BLE FUNCTION END ***/

/*** WiFi FUNCTION BEGIN ***/
void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;
  }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

/*void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}
void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}*/
/*
void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}
*/
/*** WiFi FUNCTION END ***/


/***************** SETUP *****************/
  void setup() 
  {
  //Start serial communication
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");

/*** SET UP WIFI FUNCTION BEGIN ***/
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  //mqttClient.onSubscribe(onMqttSubscribe);
  //mqttClient.onUnsubscribe(onMqttUnsubscribe);
  /* mqttClient.onPublish(onMqttPublish); */
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");
  connectToWifi();
/*** SET UP WIFI FUNCTION END ***/

/*** SET UP BLE FUNCTION BEGIN ***/
  //Init BLE device
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(30);
/*** SET UP BLE FUNCTION END ***/
  }
  
  void loop() 
  {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) 
  {
    if (connectToServer(*pServerAddress)) 
    {
      Serial.println("We are now connected to the BLE Server.");
       
      temperatureCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      humidityCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      connected = true;
      Serial.println("Tuan Manh");
    } 
    else 
    {
      Serial.println("Failed to connect to server!");
    }
  doConnect = false; // set về false để kết nối lần lượt với các nút khác nhau
  }
  //if new temperature readings are available
  if (newTemperature && newHumidity)
  {
    count++;
    if (count = 5)  // nhận đủ 5 giá trị cảm biến mới thì ngắt kết nối.
    {
    count = 0;
    newTemperature = false;
    newHumidity = false;
    pClient->disconnect();
    Serial.println("Disconnect to server...");
    delay(1000);
    Serial.println("Reconnect to server...");
    BLEDevice::getScan()->start(30);
    }
  }
  delay(1000); // Delay a second between loops.
  }
