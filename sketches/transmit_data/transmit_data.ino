#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Network credentials
const char* ssid = "WaterAnalysisSystem";
const char* password = "wifi_password";

// List of high priority fourth octets
const int highPriorityIPs[] = { 212, 69, 88 };
const int numHighPriorityIPs = sizeof(highPriorityIPs) / sizeof(highPriorityIPs[0]);

#define sensorPin 33  // 33 for ESP32
#define SAMPLES 10
int readings[SAMPLES];
int readIndex = 0;
long total = 0;
int buzzerPin = 32;
int ThermistorPin = 34;
int Vo;
float minTemp = 20.0;
float maxTemp = 37.0;
float R1 = 10000;
float logR2, R2, T;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

struct ADCTemperaturePair {
  int adcValue;
  float temperature;
};

// Define the pairs of ADC values and corresponding temperatures
ADCTemperaturePair adcTempPairs[] = {
  // { 2187, 24.0}, // anomaly
  { 2200, 35.0 }, // from body temperature
  { 2353, 28.0 },  // from ambient
  { 2614, 27.0},  // Calibrated from ambient
  { 2777, 23.0},  // estimated indoor temp. Actual outdoor temp: 18
  { 3600, 20.0}
};

// Number of pairs
const int numPairs = sizeof(adcTempPairs) / sizeof(adcTempPairs[0]);


String serverName = "127.0.01:5000";
int timeout = 500;


int getAverageSensorValue() {
  total = total - readings[readIndex];
  readings[readIndex] = analogRead(sensorPin);
  total = total + readings[readIndex];
  readIndex = (readIndex + 1) % SAMPLES;
  return total / SAMPLES;
}


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
  // Set the attenuation level for the ADC
  analogSetAttenuation(ADC_11db);  // Configure to handle 0-3.3V range
}


void loop() {
  Vo = analogRead(ThermistorPin);
  int sensorValue = getAverageSensorValue();
  float voltage = sensorValue * (5.0 / 4095.0);          // Convert to voltage (assuming 3.3V reference)
  double turbidity = map(sensorValue, 0, 4095, 100, 0);  // Adjusted for ESP32's 12-bit ADC

  Serial.print("RAW: ");
  Serial.println(Vo);

  // if (Vo != 0) {
  //   R2 = R1 * (4095.0 / (float) Vo - 1.0); // 4095 because of 12-bit ADC
  //   if (R2 > 0) {
  //     logR2 = log(R2);
  //     T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2));
  //     T = T - 273.15; // Convert Kelvin to Celsius

  //     Serial.print("Temperature: ");
  //     Serial.print(T);
  //     Serial.println(" C");
  //   } else {
  //     Serial.println("Error: R2 is non-positive.");
  //   }
  // } else {
  //   Serial.println("Error: Vo is zero.");
  // }

  T = getTemperatureFromADC(Vo);


  Serial.print("Raw Sensor Value: ");
  Serial.print(sensorValue);
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 2);  // Print voltage with 2 decimal places
  Serial.print("V | Turbidity: ");
  Serial.println(turbidity);

  // Put the values here!!! Random values have been put in as placeholders
  // for testing purposes!

  int temp_adc = Vo;                    // raw ADC for the temperature sensor
  double temp_val = T;                  // Calculated temperature in degrees C
  double turb_adc = sensorValue;        // raw ADC for the turbidity sensor
  double turb_val = (turbidity / 100);  // Caluculated value for turbidity. It should be a ratio, 0.0 to 1.0

  // Calibrate the turbidity value
  if (turb_val < 0.35)
    turb_val = 0.35;
  turb_val = (turb_val - 0.35) * (1 / (1 - 0.35));

  // No need to touch these, unless you're just changing the other input parameters
  double temp_volt = calculateVoltage(temp_adc, 4095, 5.0);
  double turb_volt = calculateVoltage(turb_adc, 4095, 5.0);

  // if (T >= minTemp && T <= maxTemp) {
  //   beep();
  // }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;

    String updateEndpoint = serverName + "/update";
    Serial.println("Update URL: " + updateEndpoint);

    // WiFiClient and Server URL
    http.begin(client, updateEndpoint);
    http.addHeader("Content-Type", "application/json");

    // JSON data containing sensor readings
    StaticJsonDocument<256> jsonData;
    jsonData["temperature_sensor"]["rawADC"] = temp_adc;
    jsonData["temperature_sensor"]["voltage"] = temp_volt;
    jsonData["temperature_sensor"]["temperature"] = temp_val;

    jsonData["turbidity_sensor"]["rawADC"] = turb_adc;
    jsonData["turbidity_sensor"]["voltage"] = turb_volt;
    jsonData["turbidity_sensor"]["turbidity"] = turb_val;

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
      // http.get
    }

    // StaticJsonDocument<200> responseJson;
    // const char* safetyRating = responseJson["safety_rating"];
    // double sr = String(safetyRating).toDouble();
    // Serial.println("Safety rating: " + String(safetyRating) + " --- " + String(sr));

    // if (sr < 0.5)
    //   beep();



    // Free resources
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }

  // Send data every 0.5 seconds
  delay(500);
}


void beep() {
  for (int i = 0; i <= 3; i++) {
    digitalWrite(buzzerPin, HIGH);
    delay(500);
    digitalWrite(buzzerPin, LOW);
    delay(500);
  }
}


double calculateVoltage(int rawADC, int maxADC, double maxVoltage) {
  // Ensure rawADC is within the valid range
  if (rawADC < 0) rawADC = 0;
  if (rawADC > maxADC) rawADC = maxADC;

  // Calculate the voltage
  double voltage = (double(rawADC) / maxADC) * maxVoltage;
  return voltage;
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


float getTemperatureFromADC(int adcValue) {
  // If the ADC value is below the first known value
  if (adcValue <= adcTempPairs[0].adcValue) {
    int adcDiff = adcTempPairs[1].adcValue - adcTempPairs[0].adcValue;
    float tempDiff = adcTempPairs[1].temperature - adcTempPairs[0].temperature;
    float rateOfChange = tempDiff / adcDiff;
    return adcTempPairs[0].temperature + (adcValue - adcTempPairs[0].adcValue) * rateOfChange;
  }

  // If the ADC value is above the last known value
  if (adcValue >= adcTempPairs[numPairs - 1].adcValue) {
    int adcDiff = adcTempPairs[numPairs - 1].adcValue - adcTempPairs[numPairs - 2].adcValue;
    float tempDiff = adcTempPairs[numPairs - 1].temperature - adcTempPairs[numPairs - 2].temperature;
    float rateOfChange = tempDiff / adcDiff;
    return adcTempPairs[numPairs - 1].temperature + (adcValue - adcTempPairs[numPairs - 1].adcValue) * rateOfChange;
  }

  // For ADC values within the range of known pairs, interpolate
  for (int i = 0; i < numPairs - 1; i++) {
    if (adcValue >= adcTempPairs[i].adcValue && adcValue <= adcTempPairs[i + 1].adcValue) {
      int adcDiff = adcTempPairs[i + 1].adcValue - adcTempPairs[i].adcValue;
      float tempDiff = adcTempPairs[i + 1].temperature - adcTempPairs[i].temperature;
      float rateOfChange = tempDiff / adcDiff;
      return adcTempPairs[i].temperature + (adcValue - adcTempPairs[i].adcValue) * rateOfChange;
    }
  }

  // Default return value, though this should never be reached
  return 0.0;
}
