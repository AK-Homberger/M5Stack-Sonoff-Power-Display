/*
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
  Sonoff POW R2 -> ESP8266 (e.g. Wemos D1 mini) and SSD1306 display
  Reads JSON Data from Sonoff POW R2 and displays is on the SSD1306 OLED module
  Version 0.1 / 26.07.2020
*/

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUZZER_PIN 12  // Optional Buzzer connected to GPIO 12

/* OLED SSD1306 Connection

SSD1306   ESP8266

Vin       3.3V
GND       GND
SCL       GPIO 5 (D1)
SDA       GPIO 4 (D2)
*/

const char *ssid = "PowerMeter";     // Set WLAN name
const char *password = "power0102";  // Set password

int port = 80;                       // JSON Server port of Sonoff POW R2
const char * host = "192.168.4.100"; // Server IP. Set Sonoff POW to fixed IP

int connection = 0;                  // Connection re-tries. Alarm if > 5
unsigned long update_time = 0;       // Update timer
bool update = false;                 // New data. Update display
bool alarm = false;                  // Alarm buzzer on/off
bool sharp = false;                  // Alarm only after first successfull connection

struct tPower {
  double Total, Yesterday, Today, Power, ApparentPower, ReactivePower, Factor, Voltage, Current;
};
tPower Power;



void setup() {

  Power.Total = 0.003;                // Power on Sonoff starts with this value. Why?
  Serial.begin(115200); delay(100);

  pinMode(BUZZER_PIN, OUTPUT);                // Buzzer connected to GPIO12

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  display.display();    // Show Adafruit splash screen
  delay(1000);          // Pause for 1 second
  display.clearDisplay();

  WiFi.softAP(ssid, password); // AP name and password
  Serial.println("Start WLAN AP");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
}



void Get_JSON_Data() {   // Read Energy Data from Sonoff POW

  // Allocate JsonBuffer
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<1000> root;

  WiFiClient client;
  client.setTimeout(500);

  // Connect to HTTP server
  if (!client.connect(host, port)) {
    Serial.println(F("Connection failed"));
    Power.Voltage = 0;
    Power.Total = 0.003 ;
    Power.Power = 0 ;
    Power.Voltage = 0 ;
    Power.Current = 0 ;
    update = true;
    connection++;
    // Disconnect
    client.stop();
    return;
  }

  connection = 0;

  // Send HTTP request
  client.println(F("GET /cm?cmnd=status%208 HTTP/1.0"));
  client.println(F("Host: arduinojson.org"));
  client.println(F("Connection: close"));
  if (client.println() == 0) {
    Serial.println(F("Failed to send request"));
    // Disconnect
    client.stop();
    return;
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.0 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    // Disconnect
    client.stop();
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    // Disconnect
    client.stop();
    return;
  }

  // Parse JSON object
  DeserializationError error = deserializeJson(root, client);

  if (error) {
    Serial.println(F("Parsing failed!"));
    return;
  }

  Serial.println(F("Parsing sucess!"));
  update = true;
  sharp = true;
  // Extract values

  Power.Total = root["StatusSNS"]["ENERGY"]["Total"] ;
  Power.Power = root["StatusSNS"]["ENERGY"]["Power"] ;
  Power.Voltage = root["StatusSNS"]["ENERGY"]["Voltage"] ;
  Power.Current = root["StatusSNS"]["ENERGY"]["Current"] ;

  // Disconnect
  client.stop();
}




void DisplayData(void) {

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  display.printf("%7.0f V\n", Power.Voltage);
  display.printf("%7.2f A\n", Power.Current);
  display.printf("%7.0f W\n", Power.Power);
  display.printf("%6.2f kWh", Power.Total - 0.003);

  display.display();
}



void loop() {

  if (millis() > update_time + 1000) {       // Get data once per second
    Get_JSON_Data();
    update_time = millis();
    alarm = !alarm;                          // Toggle alarm sound (on/off) every second
  }

  if ((connection > 5) && (sharp == true)) { // Power off alarm on
    digitalWrite(BUZZER_PIN, alarm);
  }

  if (update) {     // Received new data via JSON from Sonoff POW
    DisplayData();
    Serial.println(F("Update"));
    if (connection == 0) digitalWrite(BUZZER_PIN, 0);// Alarm off
    update = false;
  }
}
