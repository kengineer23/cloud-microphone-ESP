/*
1. Filename: ESP.cpp
2. Description: Firmware for ESP32 responsible for all the communication with the cloud and the sensor ESP
3. Author: Kanishk Gulati
4. Date: 
5. Version: V0.1
*/

/**
 * @file main.cpp
 * @brief Firmware for ESP32 responsible for communication with cloud and sensor ESP.
 * 
 * This file includes functionalities for WiFi connectivity, MQTT communication, HTTP requests,
 * and handling sensor data and control parameters. It also integrates with the AnimatedEye library
 * to display animations.
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WebServer.h>

#include "AnimatedEye.h" // Include the graphics library (this includes the sprite functions)

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
const char* mqtt_server = "192.168.1.249"; ///< IP address of MQTT broker
const int mqtt_port = 1883; ///< Port for MQTT broker

WiFiClient espClient; ///< WiFi client instance
PubSubClient client(espClient); ///< MQTT client instance

// MongoDB Data API settings
const char* apiKeySEND = "eN2shi0T52prrIrWwmK4NxnLCVM4wefv1Gx5hHLAiLohz510h1vaS84p9AVjKYWI"; ///< API key for sending data to MongoDB
const char* apikeyRECEIVE = "PW0Z7GH5INqp6lKXzlByp3f21ewtxMUXoaNhw12ksm7QKPyBZEcsJpAO5MMpSDgk"; ///< API key for receiving data from MongoDB
const char* endpointSEND = "https://ap-south-1.aws.data.mongodb-api.com/app/data-giddkzt/endpoint/data/v1/action/insertOne"; ///< Endpoint for sending data to MongoDB
const char* endpointRECEIVE = "https://ap-south-1.aws.data.mongodb-api.com/app/data-giddkzt/endpoint/data/v1/action/aggregate"; ///< Endpoint for receiving data from MongoDB

// Task Handles
TaskHandle_t DisplayEyesHandle; ///< Handle for the DisplayEyes task
TaskHandle_t ReceiveFromServerHandle; ///< Handle for the ReceiveFromServer task
TaskHandle_t ReceiveFromCloudHandle; ///< Handle for the ReceiveFromCloud task

// Semaphore Handles
SemaphoreHandle_t xSendMutex; ///< Mutex for managing concurrent access to shared resources

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
void SendToCloud(String sensor_message){
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

/**
 * @brief Reconnects to the MQTT broker if the connection is lost.
 */
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP2")) {
      Serial.println("connected");
      if (client.subscribe("isaac/sensor_data")) {
        Serial.println("Subscription successful");
      } else {
        Serial.println("Subscription failed");
      }
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/**
 * @brief Callback function for handling incoming MQTT messages.
 * 
 * @param topic Topic of the incoming message.
 * @param message Pointer to the message payload.
 * @param length Length of the message payload.
 */
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  Serial.println(messageTemp);

  SendToCloud(messageTemp);
}

/**
 * @brief Task for handling incoming MQTT messages.
 * 
 * This task ensures that the MQTT client is connected and processes incoming messages.
 * 
 * @param pvParameters Pointer to task parameters (not used).
 */
void TaskReceiveFromServer(void *pvParameters){
  while(1){
    if(client.connected()){
      client.loop();
    }
    else{
      reconnect();
    }

    vTaskDelay(10000/portTICK_PERIOD_MS);
  }
}

/**
 * @brief Task for sending action parameters to the MQTT server.
 * 
 * This task constructs a payload from LED color and duty cycle parameters and publishes it
 * to the MQTT broker.
 */
void TaskSendToServer(){ 
    if(client.connected()){
      String payload = "{\"RED\": " + String(ledcolor.red) + ",";
      payload += "\"GREEN\" : " + String(ledcolor.green) + ",";
      payload += "\"BLUE\" : " + String(ledcolor.blue) + ",";
      payload += "\"DUTY CYCLE\" : " + String(dutycycle);
      payload += "}";

      // Print the received 'action' collection document
      Serial.println(payload);
  
      if(client.publish("isaac/action", payload.c_str())){
        Serial.println("Message published");
      }  
      else{
        Serial.println("Message didn't publish");
      }
    }
    else{
      reconnect();
    }
  }

/**
 * @brief Handles HTTP POST requests for updating LED and motor parameters.
 * 
 * @details Parses the incoming JSON data and updates the LED color and duty cycle parameters.
 * The updated parameters are then sent to the MQTT server.
 */
void handleUpdate() {
  if (server.hasArg("plain")) {
    String json = server.arg("plain");
    Serial.println("Received data: " + json);

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, json);

    if (!error) {
      ledcolor.red = doc["RED"]["$numberInt"].as<uint8_t>();
      ledcolor.green = doc["GREEN"]["$numberInt"].as<uint8_t>();
      ledcolor.blue = doc["BLUE"]["$numberInt"].as<uint8_t>();
      dutycycle = doc["DutyCycle"]["$numberDouble"].as<uint16_t>();

      Serial.println("RED: " + String(ledcolor.red) + ", GREEN: " + String(ledcolor.green) + ", BLUE: " + String(ledcolor.blue));
      server.send(200, "application/json", "{\"status\":\"success\"}");

      TaskSendToServer();

    } else {
      Serial.println("Failed to parse JSON");
      server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Failed to parse JSON\"}");
    }
  } else {
    Serial.println("No data received");
    server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"No data received\"}");
  }
}

/**
 * @brief Task for handling incoming HTTP requests from the cloud.
 * 
 * This task processes HTTP client requests to update parameters.
 * 
 * @param pvParameters Pointer to task parameters (not used).
 */
void TaskReceiveFromCloud(void* pvParameters){
  while(1){
    server.handleClient();
    vTaskDelay(10000 / portTICK_PERIOD_MS); //Receive data every 10 seconds
  }
}

/**
 * @brief Task for displaying eye animations.
 * 
 * This task updates the display with eye animations, including blinking.
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

/**
 * @brief Initialization function called once on boot-up.
 * 
 * Sets up serial communication, initializes the display, connects to WiFi, sets up the HTTP server,
 * connects to the MQTT broker, and creates tasks for handling various functions.
 */
void setup(){
  Serial.begin(9600);
  isaac_eyes.init();

  setup_wifi();
  server.on("/update", HTTP_POST, handleUpdate);
  server.begin();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); // Register the callback function

  // Create Core 0 Task(s)
  xTaskCreatePinnedToCore(TaskReceiveFromServer, "TaskReceiveFromServer", 4096, NULL, 1, &ReceiveFromServerHandle, 0);
  xTaskCreatePinnedToCore(TaskReceiveFromCloud, "TaskReceiveFromCloud", 4096, NULL, 1, &ReceiveFromCloudHandle, 0);
  
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
