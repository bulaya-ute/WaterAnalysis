#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// Network credentials
const char* ssid = "wifi_ssid";
const char* password = "wifi_password";

// Server IP and Port
const char* serverName = "http://192.168.43.77:5000/update";


int buzzerPin = 2;
int ThermistorPin = 0;
int Vo;
float minTemp = 20.0;
float maxTemp = 37.0;
float R1 = 10000;
float logR2, R2, T;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  pinMode(ThermistorPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  Vo = analogRead(ThermistorPin);

  Serial.print("RAW: ");
  Serial.println(Vo);
  
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T = T - 273.15; // Convert Kelvin to Celsius
  // T = (T * 9.0)/ 5.0 + 32.0; // Convert Celsius to Fahrenheit

  Serial.print("Temperature: ");
  Serial.print(T);
  Serial.println(" C");

  if(T>=minTemp && T<=maxTemp){
    beep();
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;

    // WiFiClient and Server URL
    http.begin(client, serverName);
    http.addHeader("Content-Type", "application/json");

    // JSON data
    StaticJsonDocument<200> jsonData;
    jsonData["turbidity"] = random(0, 100);
    jsonData["temperature"] = T;
    jsonData["voltage"] = random(110, 230);
    jsonData["analysis"] = random(200, 1000);

    // Serialize JSON to string
    String requestBody;
    serializeJson(jsonData, requestBody);

    // Send HTTP POST request
    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    // Free resources
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }

  // Send data every 10 seconds
  delay(10000);
}

void beep(){
  for(int i=0; i <= 3; i++){
    digitalWrite(buzzerPin, HIGH);
    delay(500);
    digitalWrite(buzzerPin, LOW);
    delay(500);
  }
}
