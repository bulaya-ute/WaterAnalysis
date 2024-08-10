#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = "wifi_ssid";
const char* password = "wifi_password";

// Replace with your server IP and port
const char* serverName = "http://192.168.43.212:5000/update";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Specify server URL
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    // Create JSON data
    StaticJsonDocument<200> jsonData;
    jsonData["turbidity"] = random(0, 100);
    jsonData["temperature"] = random(20, 30);
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
