#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Network credentials
const char* ssid = "WaterAnalysisSystem";
const char* password = "wifi_password";

// List of high priority fourth octets
const int highPriorityIPs[] = { 68, 212, 77, 78, 79 };
const int numHighPriorityIPs = sizeof(highPriorityIPs) / sizeof(highPriorityIPs[0]);

int buzzerPin = 2;
int ThermistorPin = 0;
int Vo;
float minTemp = 20.0;
float maxTemp = 37.0;
float R1 = 10000;
float logR2, R2, T;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

String serverName = "";
int timeout = 500;

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

  // Find the correct server IP
  serverName = "http://" + findServerIP() + ":5000";

  Serial.println("Server name: " + serverName);
}

void loop() {
  Vo = analogRead(ThermistorPin);

  Serial.print("RAW: ");
  Serial.println(Vo);

  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2));
  T = T - 273.15;  // Convert Kelvin to Celsius
  // T = (T * 9.0)/ 5.0 + 32.0; // Convert Celsius to Fahrenheit

  Serial.print("Temperature: ");
  Serial.print(T);
  Serial.println(" C");

  if (T >= minTemp && T <= maxTemp) {
    beep();
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;

    String updateEndpoint = serverName + "/update";
    Serial.println("Update URL: " + updateEndpoint);

    // WiFiClient and Server URL
    http.begin(client, updateEndpoint);
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

  // Send data every 5 seconds
  delay(5000);
}

void beep() {
  for(int i=0; i <= 3; i++){
    digitalWrite(buzzerPin, HIGH);
    delay(500);
    digitalWrite(buzzerPin, LOW);
    delay(500);
  }
}


String findServerIP() {
  IPAddress localIP = WiFi.localIP();
  String baseIP = String(localIP[0]) + "." + String(localIP[1]) + "." + String(localIP[2]) + ".";

  Serial.print("Local IP: ");
  Serial.println(localIP.toString());
  Serial.print("Base IP range: ");
  Serial.println(baseIP);

  // Try high priority IPs first
  for (int i = 0; i < numHighPriorityIPs; i++) {
    String currentIP = baseIP + String(highPriorityIPs[i]);
    Serial.print("Trying high priority IP: ");
    Serial.println(currentIP);
    if (tryIP(currentIP)) 
      return currentIP;
  }

  // Brute force the IP range
  int bruteForceCount = 0;
  for (int i = 1; i <= 255; i++) {
    // Skip high priority IPs during brute force
    bool isHighPriority = false;
    for (int j = 0; j < numHighPriorityIPs; j++) {
      if (i == highPriorityIPs[j]) {
        isHighPriority = true;
        break;
      }
    }
    if (isHighPriority) continue;

    String currentIP = baseIP + String(i);
    Serial.print("Trying IP: ");
    Serial.println(currentIP);

    if (tryIP(currentIP)) return currentIP;

    bruteForceCount++;
    if (bruteForceCount % 10 == 0) {
      // Retry high priority IPs every 10 brute force attempts
      for (int j = 0; j < numHighPriorityIPs; j++) {
        String highPriorityIP = baseIP + String(highPriorityIPs[j]);
        Serial.print("Retrying high priority IP: ");
        Serial.println(highPriorityIP);
        if (tryIP(highPriorityIP)) return highPriorityIP;
      }
    }

    delay(10);  // Reduced delay to 10ms between scans
  }

  Serial.println("No working server found.");
  return "";
}



bool tryIP(const String& fullIP) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;

    String fullUrl = "http://" + fullIP + ":5000/ping";
    http.begin(client, fullUrl);
    http.setTimeout(500);  // 500 ms timeout

    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      String response = http.getString();
      if (response.indexOf("alive") >= 0) {
        // serverName = "http://" + fullIP + ":5000";
        Serial.print("Found working server at: ");
        Serial.println(serverName);
        http.end();
        return true;
      }
    }
    http.end();
  }
  return false;
}
