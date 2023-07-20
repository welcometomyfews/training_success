#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <Adafruit_NeoPixel.h>

#define PINLED        25
#define NUMPIXELS  20
#define ROWS        20
#define COLS        16

Adafruit_NeoPixel pixels(NUMPIXELS, PINLED, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 500

const char* ssid = "ESP32";
const char* password = "11111111";

WiFiServer server(80);
String selectedLights = "";
int redValue = 0;
int greenValue = 0;
int blueValue = 0;

void setup() {
  pinMode(PINLED, OUTPUT);
  digitalWrite(PINLED, LOW);

  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  }
  pixels.show();

  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  if (!WiFi.softAP(ssid, password)) {
    Serial.println("Soft AP creation failed.");
    while (1);
  }

  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  server.begin();

  Serial.println("Server started");
}

void turnOnSelectedLights() {
  pixels.clear();
  // Split the selected lights string into individual numbers
  int lengthS = selectedLights.length();
  String lightIndex;
  for (int i = 0; i < lengthS; i++) {
    char c = selectedLights.charAt(i);
    if (c >= '0' && c <= '9') {
      lightIndex += c;
    } else {
      if (lightIndex.length() > 0) {
        int index = lightIndex.toInt();
        if (index >= 0 && index < NUMPIXELS) {
          pixels.setPixelColor(index, pixels.Color(redValue, greenValue, blueValue));
        }
        lightIndex = "";
      }
    }
  }

  pixels.show();
}

void turnOffAllLights() {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  }
  pixels.show();
}

void handleWebRequest(WiFiClient client) {
  String currentLine = "";

  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      Serial.write(c);
      if (c == '\n') {
        if (currentLine.length() == 0) {
          break;
        } else {
          currentLine = "";
        }
      } else if (c != '\r') {
        currentLine += c;
      } if (currentLine.startsWith("GET /on")) {
        int paramIndex = currentLine.indexOf("light=");
        if (paramIndex != -1) {
          selectedLights = currentLine.substring(paramIndex + 6);
          turnOnSelectedLights();
        }
      } else if (currentLine.startsWith("GET /reset")) {
        selectedLights = "";
        turnOffAllLights();
      } else if (currentLine.startsWith("GET /color")) {
        int redIndex = currentLine.indexOf("red=");
        int greenIndex = currentLine.indexOf("green=");
        int blueIndex = currentLine.indexOf("blue=");
        if (redIndex != -1 && greenIndex != -1 && blueIndex != -1) {
          redValue = currentLine.substring(redIndex + 4, greenIndex).toInt();
          greenValue = currentLine.substring(greenIndex + 6, blueIndex).toInt();
          blueValue = currentLine.substring(blueIndex + 5).toInt();
          turnOnSelectedLights();
        }
      }
    } 
  }
  
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: text/html\r\n";
  response += "\r\n";
  response += "<!DOCTYPE HTML>\r\n";
  response += "<html>\r\n";
  response += "<head>\r\n";
  response += "<style>\r\n";
  response += "body { font-family: Arial, sans-serif; }\r\n";
  response += "h1 { color: #333333; }\r\n";
  response += "table { border-collapse: collapse; }\r\n";
  response += "td { padding: 5px; }\r\n";
  response += "input[type='checkbox'] { margin-right: 5px; }\r\n";
  response += "input[type='submit'] { background-color: #4CAF50; color: white; border: none; padding: 10px 20px; text-decoration: none; cursor: pointer; }\r\n";
  response += "</style>\r\n";
  response += "<script>\r\n";
  response += "function showAlert() {\r\n";
  response += "  alert('Lights turned on!');\r\n";
  response += "}\r\n";
  response += "</script>\r\n";
  response += "</head>\r\n";
  response += "<body>\r\n";
  response += "<h1>ESP32 Web Server</h1>\r\n";
  response += "<p>Select lights to turn on:</p>\r\n";
  response += "<form method=\"get\" action=\"/on\">\r\n";
  response += "<table>\r\n";

  // Generate table with checkboxes for each light
  int lengthS = selectedLights.length();
  selectedLights = selectedLights.substring(0, lengthS - 8);
  for (int y = 0; y < ROWS; y++) {
    response += "<tr>\r\n";
    for (int x = 0; x < COLS; x++) {
      int index = x * ROWS + y;
      response += "<td>\r\n";
      response += "<label>\r\n";
      response += "<input type=\"checkbox\" name=\"light\" value=\"" + String(index) + "\"";
      if (selectedLights.indexOf(String(index)) != -1) {
        response += " checked";
      }
      response += ">\r\n";
      response += index;
      response += "</label>\r\n";
      response += "</td>\r\n";
    }
    response += "</tr>\r\n";
  }

  response += "</table>\r\n";
  response += "<input type=\"submit\" value=\"Turn On\" onclick=\"showAlert()\"><br>\r\n";
  response += "</form><br>\r\n";
  response += "<form method=\"get\" action=\"/reset\">\r\n";
  response += "<input type=\"submit\" value=\"Reset Lights\">\r\n";
  response += "</form>\r\n";
  response += "<p>Set light color:</p>\r\n";
  response += "<form method=\"get\" action=\"/color\">\r\n";
  response += "Red: <input type=\"text\" name=\"red\" value=\"" + String(redValue) + "\"><br>\r\n";
  response += "Green: <input type=\"text\" name=\"green\" value=\"" + String(greenValue) + "\"><br>\r\n";
  response += "Blue: <input type=\"text\" name=\"blue\" value=\"" + String(blueValue) + "\"><br>\r\n";
  response += "<input type=\"submit\" value=\"Set Color\">\r\n";
  response += "</form><br>\r\n";
  
  // Add buttons to set specific colors
  response += "<form method=\"get\" action=\"/color\">\r\n";
  response += "<input type=\"hidden\" name=\"red\" value=\"191\">\r\n";
  response += "<input type=\"hidden\" name=\"green\" value=\"0\">\r\n";
  response += "<input type=\"hidden\" name=\"blue\" value=\"64\">\r\n";
  response += "<input type=\"submit\" value=\"Set Color (191, 0, 64)\">\r\n";
  response += "</form><br>\r\n";
  // Add buttons to set specific colors
  response += "<form method=\"get\" action=\"/color\">\r\n";
  response += "<input type=\"hidden\" name=\"red\" value=\"157\">\r\n";
  response += "<input type=\"hidden\" name=\"green\" value=\"0\">\r\n";
  response += "<input type=\"hidden\" name=\"blue\" value=\"98\">\r\n";
  response += "<input type=\"submit\" value=\"Set Color (157, 0, 98)\">\r\n";
  response += "</form><br>\r\n";
  // Add buttons to set specific colors
  response += "<form method=\"get\" action=\"/color\">\r\n";
  response += "<input type=\"hidden\" name=\"red\" value=\"204\">\r\n";
  response += "<input type=\"hidden\" name=\"green\" value=\"0\">\r\n";
  response += "<input type=\"hidden\" name=\"blue\" value=\"51\">\r\n";
  response += "<input type=\"submit\" value=\"Set Color (204, 0, 51)\">\r\n";
  response += "</form>\r\n";
  
  response += "</body>\r\n";
  response += "</html>\r\n";

  client.print(response);
  delay(1);
  client.stop();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    handleWebRequest(client);
  }
}
