  #include <Arduino.h>
  #include <BLEDevice.h>
  #include <BLEServer.h>
  #include <BLEUtils.h>
  #include <BLE2902.h>
  #include "DHT.h"
  #include <Adafruit_Sensor.h>

  #define temperatureCelsius
  #define BLE_server "ESP32_Server_22"

  #define DHTTYPE DHT22 

  #define uS_TO_S_FACTOR 1000000  // biến chuyển từ micro giây sang giây
  #define TIME_TO_SLEEP  1830        //Thời gian thức dậy là 30 phút + 30 giây

  const int DHTPin = 4;
  DHT dht(DHTPin, DHTTYPE);
  
  #define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"

  BLECharacteristic dhtTemperatureCelsiusCharacteristics("cba1d466-344c-4be3-ab3f-189f80dd7518", BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor dhtTemperatureCelsiusDescriptor(BLEUUID((uint16_t)0x2902));

  BLECharacteristic dhtHumidityCharacteristics("ca73b3ba-39f6-4ab3-91ae-186dc9577d99", BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor dhtHumidityDescriptor(BLEUUID((uint16_t)0x2903));
  
  
  bool device_connected = false;
  
  class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
  device_connected = true;
  };
  
  void onDisconnect(BLEServer* pServer) {
  device_connected = false;
  Serial.println("Client disconnected. Going to sleep mode now...");
  Serial.flush(); 
  esp_deep_sleep_start();
  }
  };

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : 
      Serial.println("Wakeup caused by external signal using RTC_IO"); 
      break;
    case ESP_SLEEP_WAKEUP_EXT1 : 
      Serial.println("Wakeup caused by external signal using RTC_CNTL"); 
      break;
    case ESP_SLEEP_WAKEUP_TIMER : 
      Serial.println("Wakeup caused by timer"); 
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : 
      Serial.println("Wakeup caused by touchpad"); 
      break;
    case ESP_SLEEP_WAKEUP_ULP : 
      Serial.println("Wakeup caused by ULP program"); 
      break;
    default : 
      Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); 
      break;
  }
}
  
  void setup() {
  // khởi tạo các hàm đọc nhiệt độ, độ ẩm 
  // và hàm in ra màn hình serial với tốc độ 115200 bit/s
  dht.begin();
  Serial.begin(115200);
  
  // Create a new BLE device with the BLE server name you’ve defined earlier
  BLEDevice::init("ESP32_Server_22");
  
  // Set the BLE device as a server and assign a callback function
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // Start a BLE service with the service UUID defined earlier
  BLEService *dhtService = pServer->createService(SERVICE_UUID);

  // create the temperature BLE characteristic
  #ifdef temperatureCelsius
  dhtService->addCharacteristic(&dhtTemperatureCelsiusCharacteristics);
  dhtTemperatureCelsiusDescriptor.setValue("DHT Temperature (Celsius)");
  dhtTemperatureCelsiusCharacteristics.addDescriptor(new BLE2902());
  // Fahrenheit characteristic
  #else
  dhtService->addCharacteristic(&dhtTemperatureFahrenheitCharacteristics);
  dhtTemperatureFahrenheitDescriptor.setValue("DHT Temperature in (Fahrenheit)");
  dhtTemperatureFahrenheitCharacteristics.addDescriptor(new BLE2902());
  #endif

  // After that, it sets the humidity characteristic
  dhtService->addCharacteristic(&dhtHumidityCharacteristics);
  dhtHumidityDescriptor.setValue("DHT humidity");
  dhtHumidityCharacteristics.addDescriptor(new BLE2902());

  //gọi hàm thức dậy mỗi 30 phút
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
  //in ra nguồn đánh thức esp32
  print_wakeup_reason();

  // Finally, you start the service, and the server starts the advertising
  // so other devices can find it.
  // Start the service
  dhtService->start();
  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
  }
  
  void loop() {

// nếu client connect thì đọc cảm biến và notify giá trị cho client:
  if (device_connected) 
  {
  // đọc cảm biến
  Serial.println("Client found...");
  float temp = dht.readTemperature();
  float f = dht.readTemperature(true);
  float hum = dht.readHumidity();
  
  if (isnan(hum) || isnan(temp) || isnan(f)) {
  Serial.println("Failed to read from DHT sensor! Check connections");
  return;
  }

  // đặt giá trị đọc đc vào characteristics rồi notify cho client
  #ifdef temperatureCelsius
  static char temperature_celsius[7];
  dtostrf(temp, 6, 2, temperature_celsius);
  dhtTemperatureCelsiusCharacteristics.setValue(temperature_celsius);
  dhtTemperatureCelsiusCharacteristics.notify();
  Serial.print("Temperature Celsius: ");
  Serial.print(temp);
  Serial.print(" *C");
  #else
  static char temperature_Fahrenheit[7];
  dtostrf(f, 6, 2, temperature_Fahrenheit);
  dhtTemperatureFahrenheitCharacteristics.setValue(temperature_Fahrenheit);
  dhtTemperatureFahrenheitCharacteristics.notify();
  Serial.print("Temperature Fahrenheit: ");
  Serial.print(f);
  Serial.print(" *F");
  #endif
  
  static char humidity[7];
  dtostrf(hum, 6, 2, humidity);
  dhtHumidityCharacteristics.setValue(humidity);
  dhtHumidityCharacteristics.notify();
  Serial.print("  Humidity: ");
  Serial.print(hum);
  Serial.println(" %");

  delay(5000); // delay 5s giữa các lần đọc
  }
  }
