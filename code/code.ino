/*
 * Project: GPT4o Image Question-Answering on ESP32
 * Description: This code demonstrates how to run the GPT4o model on an ESP32, allowing users to input an image URL 
 *              and a question via the serial monitor and receive AI-generated responses directly in the serial monitor.
 * 
 * Tested Environment:
 * - Arduino IDE version: 2.3.2
 * - ESP32 boards package version: 3.0.0
 * - ArduinoJson library version: 7.1.0
 * 
 * Important Notes:
 * 1. Before uploading this code, ensure you have entered the following details:
 *    - Wi-Fi credentials (SSID and Password) for internet connectivity.
 *    - GPT4o API or related setup information required for the model to work.
 *    - Any other project-specific configuration parameters as mentioned in the code.
 * 2. Install all necessary libraries and verify compatibility with the mentioned versions.
 * 3. Ensure the ESP32 is connected to a reliable power source and your hardware setup is correctly configured.
 * 
 * For a complete step-by-step tutorial, check out our YouTube video: 
 * https://youtu.be/Mp0GPfIBWMs
 * 
 * Happy Making!
 */

#include <WiFi.h>               // Include WiFi library for ESP32
#include <HTTPClient.h>         // Include HTTPClient library for HTTP requests
#include <ArduinoJson.h>         // Include ArduinoJson library for JSON parsing

// Replace with your WiFi credentials
const char* WIFI_SSID = "SSID";
const char* WIFI_PASSWORD = "PASSWORD";

// Replace the key below with your actual API key
const String API_KEY = "OPEN AI API KEY";

// API host and endpoint
const char* ENDPOINT = "https://api.openai.com/v1/chat/completions";

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Connect to WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("\nEnter an image URL for analysis:");
}

void loop() {
  static String imageUrl = "";
  static bool awaitingQuestion = false;

  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (!awaitingQuestion) {
      imageUrl = input;
      Serial.println("Image URL received. Now enter the question to ask:");
      awaitingQuestion = true;
    } else {
      String question = input;
      awaitingQuestion = false;

      if (imageUrl.length() > 0 && question.length() > 0) {
        String result;
        Serial.println("\n[ChatGPT] - Analyzing the provided image URL and question");

        if (Image_Query("gpt-4o", "user", question.c_str(), imageUrl.c_str(), "auto", 400, result)) {
          Serial.print("[ChatGPT] Response: ");
          Serial.println(result);
        } else {
          Serial.print("[ChatGPT] Error: ");
          Serial.println(result);
        }

        Serial.println("\nEnter another image URL for analysis:");
      } else {
        Serial.println("Invalid input. Please try again.");
      }
    }
  }
}

bool Image_Query(const char* model, const char* role, const char* text, const char* imageUrl, const char* detail, int max_tokens, String& result) {
  String postBody = String("{") +
                    "\"model\": \"" + model + "\", " +
                    "\"max_tokens\": " + String(max_tokens) + ", " +
                    "\"messages\": [{\"role\": \"" + role + "\", \"content\": " +
                    "[{\"type\": \"text\", \"text\": \"" + text + "\"}, " +
                    "{\"type\": \"image_url\", \"image_url\": {\"url\": \"" + imageUrl + "\", \"detail\": \"" + detail + "\"}}]}]}";

  return sendRequest(postBody, result);
}

bool sendRequest(const String& postBody, String& result) {
  HTTPClient http;

  http.begin(ENDPOINT);  // Specify the endpoint
  http.addHeader("Authorization", "Bearer " + API_KEY);  // Add Authorization header
  http.addHeader("Content-Type", "application/json");    // Add Content-Type header

  int httpResponseCode = http.POST(postBody);

  if (httpResponseCode > 0) {
    String response = http.getString();

    if (httpResponseCode == 200) {
      int start = response.indexOf("{");
      int end = response.lastIndexOf("}");

      if (start == -1 || end == -1) {
        result = "[ERR] Invalid JSON response";
        return false;
      }

      String jsonBody = response.substring(start, end + 1);

      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, jsonBody);
      if (error) {
        result = "[ERR] JSON Parsing Failed: ";
        result += error.c_str();
        return false;
      }

      if (doc.containsKey("choices") && doc["choices"].size() > 0) {
        result = doc["choices"][0]["message"]["content"].as<String>();
        return true;
      } else {
        result = "[ERR] No valid response in JSON";
        return false;
      }
    } else {
      result = "[ERR] HTTP Error: " + String(httpResponseCode) + ", Response: " + response;
      return false;
    }
  } else {
    result = "[ERR] Connection failed, Error: " + String(httpResponseCode);
    return false;
  }

  http.end();
}
