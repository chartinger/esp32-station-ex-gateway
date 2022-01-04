#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "./config.h"

#define RX 16
#define TX 17

#define COMMAND_BUFFER_SIZE 150
#define COMMAND_OUT_BUFFER_SIZE 150

#define CsExSerial Serial2

const char* ssid = WLAN_SSID;
const char* password = WLAN_PASSWORD;
const char* mqtt_server = MQTT_BROKER_URL;

uint16_t serialInputBufferIndex = 0;
char serialInputBuffer[COMMAND_BUFFER_SIZE];
char serialOutputBuffer[COMMAND_OUT_BUFFER_SIZE];

WiFiClient espClient;
PubSubClient mqttClient(espClient);

AsyncWebServer webserver(80);
AsyncWebSocket ws("/ws");

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    Serial.print("WEBSOCKET:");
    Serial.println((char*)data);
    CsExSerial.println((char*)data);
  }
}


void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}


void setupWebsocket() {
  ws.onEvent(onWebSocketEvent);
  webserver.addHandler(&ws);
  webserver.begin();
}

void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());
}

void handleMqttMessage(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    CsExSerial.print((char)payload[i]);
  }
  CsExSerial.println();
}

void setupMqtt() {
  mqttClient.setServer(mqtt_server, MQTT_BROKER_PORT);
  mqttClient.setCallback(handleMqttMessage);
}

void setup() {
  Serial.begin(115200);
  CsExSerial.begin(115200, SERIAL_8N1, RX, TX);
  Serial.println("ESP32S Online");
  pinMode(LED_BUILTIN, OUTPUT);
  setupWifi();
  setupMqtt();
  setupWebsocket();
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // client.publish("outTopic", "hello world");
      // ... and resubscribe
      mqttClient.subscribe(MQTT_TOPIC_IN);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off by making the voltage LOW
  while (CsExSerial.available()) {
    char character = CsExSerial.read() & 0xFF;
    Serial.print(character);
    if(character == '\r') {
      continue;
    }
    if(character == '\n' || serialInputBufferIndex > COMMAND_BUFFER_SIZE - 1) {
      if(serialInputBufferIndex == 0) {
        continue;
      }
      serialInputBuffer[serialInputBufferIndex] = '\0';
      Serial.println();
      Serial.print("--");
      Serial.print(serialInputBuffer);
      Serial.println("--");
      mqttClient.publish(MQTT_TOPIC_OUT, serialInputBuffer);
      ws.textAll(serialInputBuffer);
      serialInputBufferIndex = 0;
      continue;
    }
    serialInputBuffer[serialInputBufferIndex] = character;
    serialInputBufferIndex++;
  }
  ws.cleanupClients();
  // while (Serial.available()) {
  //   CsExSerial.print(char(Serial.read()));
  // }
}
