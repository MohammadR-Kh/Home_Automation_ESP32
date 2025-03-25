#include <WiFi.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <ESP32Servo.h>

const char* ssid = "KhMrMo";
const char* password = "M.r.k.h.1113.";

int lamp1Pin = 23;
int lamp2Pin = 14;
int lamp3Pin = 27;
int led1Pin = 26;
int led2Pin = 25;
int led3Pin = 33;
int servoPin = 32;
int pirPin = 35;
int buzzerPin = 18;
int fanPin = 16;
bool securityState = false;
bool fanState = false;

Adafruit_BMP280 bmp;
Servo servo;

WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); 

  if (!bmp.begin(0x76)) { 
    Serial.print("BMP280 Error!");
    while (1); 
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,    
                  Adafruit_BMP280::SAMPLING_X2,    
                  Adafruit_BMP280::SAMPLING_X16,    
                  Adafruit_BMP280::FILTER_X16,      
                  Adafruit_BMP280::STANDBY_MS_500);

  pinMode(lamp1Pin, OUTPUT);
  pinMode(lamp2Pin, OUTPUT);
  pinMode(lamp3Pin, OUTPUT);
  pinMode(led1Pin, OUTPUT);                
  pinMode(led2Pin, OUTPUT);
  pinMode(led3Pin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(fanPin, OUTPUT);
  pinMode(pirPin, INPUT);
  digitalWrite(lamp1Pin, HIGH);
  digitalWrite(lamp2Pin, HIGH);
  digitalWrite(lamp3Pin, LOW);
  digitalWrite(led1Pin, LOW);
  digitalWrite(led2Pin, LOW);
  digitalWrite(led3Pin, LOW);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(fanPin, LOW);
  servo.setPeriodHertz(50); 
  servo.attach(servoPin, 500, 2500); 
  servo.write(90);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("Connected to Wi-Fi!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());
  server.begin();

}

void loop() {
  WiFiClient client = server.available();
  if (!client) {
    if (bmp.readTemperature() >= 30) {
        digitalWrite(fanPin, HIGH);
        fanState = true;
    } else if (!fanState) {  
        digitalWrite(fanPin, LOW);
    }
    
    if (securityState) {
      int pirState = digitalRead(pirPin);
      if (pirState == HIGH) {
        digitalWrite(buzzerPin, HIGH);
      } else {
        digitalWrite(buzzerPin, LOW);
      }
    }
    return;
  }



  String request = readRequest(client);

  if (request.indexOf("OPTIONS /control") != -1) {
    sendCorsResponse(client);
  } else if (request.indexOf("GET /sensors") != -1) {
    sendSensorData(client);
  } else if (request.indexOf("POST /control") != -1) {
    handleControl(client, request);
  }

  client.stop();
}

String readRequest(WiFiClient client) {
  String request = "";
  unsigned long timeout = millis() + 2000;

  while (client.connected() && millis() < timeout) {
    while (client.available()) {
      char c = client.read();
      request += c;
      timeout = millis() + 200;
    }
  }

  return request;
}

String extractJsonBody(String request) {
  int jsonStart = request.indexOf("{");
  int jsonEnd = request.lastIndexOf("}");
  if (jsonStart == -1 || jsonEnd == -1) {
    return "";
  }
  return request.substring(jsonStart, jsonEnd + 1);
}

void handleControl(WiFiClient client, String request) {
  String requestBody = extractJsonBody(request);
  if (requestBody == "") {
    sendHttpResponse(client, "{\"status\":\"error\", \"message\":\"No JSON body found\"}");
    return;
  }

  StaticJsonDocument<200> jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, requestBody);


  if (error) {
    sendHttpResponse(client, "{\"status\":\"error\", \"message\":\"JSON Parsing Failed\"}");
    return;
  }

  String command = jsonDoc["command"];

  if (command == "lights_on") {
    digitalWrite(lamp1Pin, LOW);
    digitalWrite(lamp2Pin, LOW);
    digitalWrite(lamp3Pin, HIGH);
    digitalWrite(led1Pin, HIGH);
    digitalWrite(led2Pin, HIGH);
    digitalWrite(led3Pin, HIGH);
    servo.write(180); 
  } else if (command == "lights_off") {
    digitalWrite(lamp1Pin, HIGH);
    digitalWrite(lamp2Pin, HIGH);
    digitalWrite(lamp3Pin, LOW);
    digitalWrite(led1Pin, LOW);
    digitalWrite(led2Pin, LOW);
    digitalWrite(led3Pin, LOW);
    servo.write(0); 
  } else if (command == "night_mode") {
    digitalWrite(lamp1Pin, HIGH);
    digitalWrite(lamp2Pin, HIGH);
    digitalWrite(lamp3Pin, LOW);
    digitalWrite(led1Pin, HIGH);
    digitalWrite(led2Pin, LOW);
    digitalWrite(led3Pin, LOW);
  } else if (command == "lamp1_on") {
    digitalWrite(lamp1Pin, LOW);
  } else if (command == "lamp1_off") {
    digitalWrite(lamp1Pin, HIGH);
  } else if (command == "lamp2_on") {
    digitalWrite(lamp2Pin, LOW);
  } else if (command == "lamp2_off") {
    digitalWrite(lamp2Pin, HIGH);
  } else if (command == "lamp3_on") {
    digitalWrite(lamp3Pin, HIGH);
  } else if (command == "lamp3_off") {
    digitalWrite(lamp3Pin, LOW);
  } else if (command == "led1_on") {
    digitalWrite(led1Pin, HIGH);
  } else if (command == "led1_off") {
    digitalWrite(led1Pin, LOW);
  } else if (command == "led2_on") {
    digitalWrite(led2Pin, HIGH);
  } else if (command == "led2_off") {
    digitalWrite(led2Pin, LOW);
  } else if (command == "led3_on") {
    digitalWrite(led3Pin, HIGH);
  } else if (command == "led3_off") {
    digitalWrite(led3Pin, LOW);
  } else if (command == "servo0") {
    servo.write(0);
  } else if (command == "servo180") {
    servo.write(180);
  } else if (command == "security_mode_on") {
    securityState = true;
  } else if (command == "security_mode_off") {
    securityState = false;
    digitalWrite(buzzerPin, LOW);
  } else if(command == "fan_on"){
    digitalWrite(fanPin, HIGH);
    fanState = true;
  } else if(command == "fan_off"){
    digitalWrite(fanPin, LOW);
    fanState = false;
  }



  sendHttpResponse(client, "{\"status\":\"received\", \"command\":\"" + command + "\"}");
}

void sendHttpResponse(WiFiClient client, String content) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Access-Control-Allow-Methods: GET, POST, OPTIONS");
  client.println("Access-Control-Allow-Headers: Content-Type");
  client.println("Connection: close");
  client.println();
  client.println(content);
}

void sendSensorData(WiFiClient client) {
  StaticJsonDocument<200> jsonDoc;
  float temp = bmp.readTemperature();
  float press = bmp.readPressure() / 100.0;
  float alt = bmp.readAltitude(1013.25);
  int pirState = digitalRead(pirPin);

  jsonDoc["temperature"] = temp;
  jsonDoc["altitude"] = alt;
  jsonDoc["pressure"] = press;
  jsonDoc["pir"] = pirState;

  String response;
  serializeJson(jsonDoc, response);
  sendHttpResponse(client, response);
}

void sendCorsResponse(WiFiClient client) {
  client.println("HTTP/1.1 204 No Content");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Access-Control-Allow-Methods: GET, POST, OPTIONS");
  client.println("Access-Control-Allow-Headers: Content-Type");
  client.println("Connection: close");
  client.println();
}
