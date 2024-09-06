/**
 * @file main.cpp
 * @brief Firmware for ESP32 responsible for communication with cloud and sensor ESP.
 * 
 * This file includes functionalities for WiFi connectivity, MQTT communication, HTTP requests,
 * and handling sensor data and control parameters. It also integrates with the AnimatedEye library
 * to display animations.    
 * 
 * @note Next: Temporary ESP-NOW connection with ESP1 on setup to receive WiFi credentials.
 *             Receive data based on each of the device's unique ID(device ID/IP Address).
 *             Work on emotions portrayed on the display.
 *             Offline Wake Word Recognition
 */
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <CRC32.h>

#include "AnimatedEye.h" ///< Include the graphics library (this includes the sprite functions)

/**
 * @class AnimatedEye
 * @brief Class for handling eye animations on the display.
 */
AnimatedEye isaac_eyes;  ///< Create an object of class AnimatedEye

// LED Control Parameters
/**
 * @struct ledParameters
 * @brief Structure to hold the parameters for LED control.
 */
struct ledParameters {
  uint8_t red; ///< Red component of the LED color.
  uint8_t green; ///< Green component of the LED color.
  uint8_t blue; ///< Blue component of the LED color.
};
ledParameters ledcolor; ///< LED color parameters

// Motor Control Parameters
uint16_t dutycycle; ///< Duty cycle for motor control

WebServer server(80); ///< WebServer instance for HTTP server

// WiFi Credentials
const char* ssid = "PiggyRide"; ///< SSID for WiFi connection
const char* password = "9599301716"; ///< Password for WiFi connection

// MQTT Broker details
const char* mqtt_server = "driver.cloudmqtt.com"; ///< Cloud MQTT Server
const int mqtt_port = 18989; ///< Port for Cloud MQTT
const char* mqttUser = "wgreqkue";
const char* mqttPassword = "Xfm3vi1pwbk_";

// MQTT Client
WiFiClient espClient; ///< WiFi client instance
PubSubClient client(espClient); ///< MQTT client instance

// MongoDB Data API settings
const char* apiKeySEND = "5iIsSuTUvxucmbwwFGKaMeTpVdJd0lRD8scEMaNK4ri9kXuyObpcNdjeCimPHQuX"; ///< API key for sending sensor data to MongoDB
const char* endpointSEND = "https://ap-south-1.aws.data.mongodb-api.com/app/data-rcdrhhi/endpoint/data/v1/action/insertOne"; ///< Endpoint for sending data to MongoDB
const char* endpoint = "https://ap-south-1.aws.data.mongodb-api.com/app/data-rcdrhhi/endpoint/data/v1"; ///< Endpoint for sending data to MongoDB


// Task Handles
TaskHandle_t DisplayEyesHandle; ///< Handle for the DisplayEyes task
TaskHandle_t TaskReceiveFromESPHandle; ///< Handle for the ReceiveFromServer task
TaskHandle_t ReceiveFromCloudHandle; ///< Handle for the ReceiveFromCloud task


/**
 * @brief Connects to WiFi network.
 */
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

/**
 * @brief Sends sensor data to the MongoDB database.
 * 
 * @param sensor_message JSON string containing sensor data.
 */
void SendSensorDataToMongoDB (String sensor_message){
  Serial.println("Hello");
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    http.begin(endpointSEND);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("api-key", apiKeySEND);

    int httpResponseCode = http.POST(sensor_message);
    if (httpResponseCode > 0) {
      Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    }
    else {
      if (httpResponseCode == HTTPC_ERROR_CONNECTION_REFUSED) {
        Serial.println("Connection refused");
      } else if (httpResponseCode == HTTPC_ERROR_SEND_PAYLOAD_FAILED) {
          Serial.println("Failed to send payload");
      } else if (httpResponseCode == HTTPC_ERROR_SEND_HEADER_FAILED) {
          Serial.println("Failed to send headers");
      } else {
          Serial.printf("Error code: %d\n", httpResponseCode);
        }
    }
    http.end();
    } 
    else {
      Serial.println("WiFi Disconnected");
    }
}

void performApiRequest(const char* action, const String& payload) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(endpoint) + action;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("api-key", apiKeySEND);

    // Send HTTP POST request
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error in sending request");
      Serial.println("HTTP Response code: " + String(httpResponseCode));
    }

    // Free resources
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}


bool checkDeviceExists(const char* deviceId) {
  String payload = String(R"(
  {
    "dataSource": "IsaacTest",
    "database": "isaac_v1",
    "collection": "device_id",
    "filter": {
      "ISAAC ID": ")") + deviceId + String(R"("
    }
  }
  )");

  HTTPClient http;
  String url = String(endpoint) + "/action/findOne";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("api-key", apiKeySEND);

  // Send HTTP POST request
  int httpResponseCode = http.POST(payload);
  bool exists = false;

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Find One Response: " + response);
    exists = response.indexOf("\"document\":null") == -1;
  } else {
    Serial.println("Error in sending request");
    Serial.println("HTTP Response code: " + String(httpResponseCode));
  }

  // Free resources
  http.end();
  return exists;
}

void insertDeviceData(const char* deviceId) {
  String payload = String(R"(
  {
    "dataSource": "IsaacTest",
    "database": "isaac_v1",
    "collection": "device_id",
    "document": {
      "ISAAC ID": ")") + deviceId + String(R"(",
      "IP Address": )") + String(WiFi.localIP()) + 
      String(R"(}})");

  performApiRequest("/action/insertOne", payload);
}

void updateDeviceData(const char* deviceId) {
  String payload = String(R"(
  {
    "dataSource": "IsaacTest",
    "database": "isaac_v1",
    "collection": "device_id",
    "filter": {
      "ISAAC ID": ")") + deviceId + String(R"("
    },
    "update": {
      "$set": {
        "IP Address": )") + String(WiFi.localIP()) + 
        String(R"(
      }
    }
  })");

  performApiRequest("/action/updateOne", payload);
}

void SendDeviceDataToMongoDB(String payload){
  
  const char* DeviceID;
  String jsonpayload;

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if(!error){
    DeviceID = doc["document"]["ISAAC ID"];

    if(checkDeviceExists(DeviceID)){
      updateDeviceData(DeviceID);
    }
    else{
      insertDeviceData(DeviceID);
    }

    } 
  else {
    Serial.println("Failed to parse JSON");
    server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Failed to parse JSON\"}");
  }
}

void SendFanDataToMongoDB(String payload){
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if(!error){
    const char* DeviceID = doc["ISAAC ID"];
    const char* FanSpeed = doc["DutyCycle"];

    String jsonpayload = String(R"(
    {
      "dataSource": "IsaacTest",
      "database": "isaac_v1",
      "collection": "fan",
      "document": {
        "ISAAC ID": ")") + DeviceID + String(R"(",
        "Fan Speed": ")") + FanSpeed + String(R"("
      }
    })");
    if(checkDeviceExists(DeviceID)){
      performApiRequest("/action/updateOne", jsonpayload);
    }
    else{
      performApiRequest("/action/insertOne", jsonpayload);
    }
  }
  else {
    Serial.println("Failed to parse JSON");
    server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Failed to parse JSON\"}");
  }
}


/**
 * @brief Task to receive data from ESP module.
 * 
 * This task continuously checks for available data from the ESP module using Serial1.
 * If data is available, it reads the data until a newline character is encountered.
 * The received data is then printed to the Serial monitor and sent to MongoDB using the functions SendSensorDataToMongoDB() and SendDeviceDataToMongoDB().
 * 
 * Serial.write() sends the string as a series of bytes to sensor-ESP32.
 * 
 * @param pvParameters Pointer to task parameters (not used in this task).
 */
void TaskReceiveFromESP(void *pvParameters){
  while(1){
    if(Serial1.available() > 0){
      int incomingByte = Serial1.peek();  // Peek the next incoming byte w/o removing it from serial buffer
      if(incomingByte != -1){

        String data_document = Serial1.readStringUntil('\n');
        // Check for start and end markers
        if(data_document.startsWith("{")){
          Serial.println("Received valid data: " + data_document);
          
          // Extract relevant data from the received JSON string
          Serial.println("Extracting relevant data");
          String receivedJsonPaylaod = data_document.substring(0, data_document.lastIndexOf('}') + 1);
          Serial.println(receivedJsonPaylaod);

          // Extract CRC from the received string
          String crcString = data_document.substring(data_document.lastIndexOf('}') + 1);
          Serial.println(crcString);

          uint32_t crcValue = strtoul(crcString.c_str(), NULL, 10);

          // Calculate CRC32 of the received JSON string
          CRC32 crc;
          crc.update((uint8_t*)receivedJsonPaylaod.c_str(), receivedJsonPaylaod.length());
          uint32_t calculatedCRC = crc.finalize();

          // Check if the received CRC matches the calculated CRC
          if (crcValue == calculatedCRC) {
            SendDeviceDataToMongoDB(data_document);
            SendSensorDataToMongoDB(data_document);
            //SendFanDataToMongoDB(data_document);
          }
          else{
            Serial.println(String(crcValue) + " " + String(calculatedCRC));
            Serial.println("CRC32 mismatch");
            Serial.println("Discarding data");
            continue;
          }
        }
        else{
          Serial.println("Not the required data");
        }
      }
    }
    vTaskDelay(100/portTICK_PERIOD_MS); // Delay of 100 ms for polling mechanism
  }
}

void SendDataToESP(String action_params){
  // Calculate CRC32
  CRC32 crc;
  crc.update((uint8_t*)action_params.c_str(), action_params.length());
  uint32_t crcValue = crc.finalize();

  // Append CRC32 to the JSON payload
  action_params +=String(crcValue) + "\n";

  Serial.println(action_params);

  // Send data to the sensor-ESP
  Serial1.write(action_params.c_str());
}

/**
 * @brief Task for displaying eye animations.
 * 
 * This task updates the display with eye animations, including blinking.
 * 
 * No further improvements to be done yet.
 * 
 * @param pvParameters Pointer to task parameters (not used).
 */
void DisplayEyes(void *pvParameters){
  while(1){
    isaac_eyes.drawEyes();
    vTaskDelay(1500/ portTICK_PERIOD_MS);
    isaac_eyes.blink();
    vTaskDelay(50/ portTICK_PERIOD_MS);
  }
}


void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client", mqttUser, mqttPassword)) {
      if(client.subscribe("devices/ec03f332a7b0400000/action_params")) {
        Serial.println("Subscribed successfully");
      } else {
        Serial.println("Subscription failed");
      }
    } else {
      delay(5000);
    }
  }
}

/**
 * @brief Callback function for handling incoming MQTT messages.
 * 
 * This function is called whenever a message is received from the MQTT broker.
 * It prints the topic and payload of the message to the Serial monitor.
 * 
 * @param topic Topic of the message.
 * @param payload Payload of the message.
 * @param length Length of the payload.
 */
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    Serial1.print((char)payload[i]);
  }
/*
  char action_params[(sizeof(payload) + 1)];
  memcpy(action_params, payload, sizeof(payload));
  Serial.println();
  Serial.println(action_params);
*/
  // Handle message arrived
  //SendDataToESP(action_params);
}

void TaskReceiveFromMQTT(void *pvParameters){
  while(1){
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
}
/**
 * @brief Initialization function called once on boot-up.
 * 
 * Sets up serial communication, initializes the display, connects to WiFi, sets up the HTTP server,
 * connects to the MQTT broker, and creates tasks for handling various functions.
 */
void setup(){
  Serial.begin(9600);
  Serial1.begin(9600, SERIAL_8E1, 12,13);
  Serial1.flush();
  isaac_eyes.init();
 
  setup_wifi(); // Connect to WiFi network
  // MQTT Server connection
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); // Register the callback function
  reconnect();  // Connect to the MQTT broker

  // Create Core 0 Task(s)
  xTaskCreatePinnedToCore(TaskReceiveFromESP, "TaskReceiveFromESP", 4096, NULL, 1, &TaskReceiveFromESPHandle, 0);
  xTaskCreatePinnedToCore(TaskReceiveFromMQTT, "TaskReceiveFromMQTT", 4096, NULL, 1, &ReceiveFromCloudHandle, 0);
  // Create Core 1 Task(s)
  xTaskCreatePinnedToCore(DisplayEyes, "DisplayEyes", 4096, NULL, 1, &DisplayEyesHandle, 1);
}

/**
 * @brief Main loop function.
 * 
 * The loop function is intentionally left empty as tasks are handled by FreeRTOS tasks.
 */
void loop() {
  // Do Nothing
}
