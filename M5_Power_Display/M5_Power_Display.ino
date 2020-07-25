/*
  Sonoff POW R2 -> M5Stack display
  Reads JSON Data from Sonoff POW R2 and displays is on the M5Stack module
  Version 0.2 / 25.07.2020
*/

#include <M5Stack.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Preferences.h>

const char *ssid = "PowerMeter";     // Set WLAN name
const char *password = "power0102";  // Set password

int port = 80;                       // JSON Server port of Sonoff POW R2
const char * host = "192.168.4.100"; // Server IP. Set Sonoff POW to fixed IP
int connection = 0;                  // Connection re-tries. Reboot if > 10

Preferences preferences;             // Nonvolatile storage on ESP32 - To store AlarmState and LCD_Brightness

bool update = false;                 // New data. Update display
bool alarmstate = false;             // State of power loss alarm
int LCD_Brightness = 250;            // Brightness od screen

// Task handle for JSON Client (Core 0 on ESP32)
TaskHandle_t Task1;

struct tPower {
  double Total, Yesterday, Today, Power, ApparentPower, ReactivePower, Factor, Voltage, Current;
};
tPower Power;



void setup() {
  M5.begin();
  M5.Power.begin();

  Power.Total = 0.003;                // Power on Sonoff starts with this value. Why?
  Serial.begin(115200); delay(100);

  preferences.begin("nvs", false);                       // Open nonvolatile storage (nvs)
  alarmstate = preferences.getInt("alarmstate", 0);      // Read stored values
  LCD_Brightness = preferences.getInt("LCD_State", 250);
  preferences.end();                                     // Close nvs
  M5.Lcd.setBrightness(LCD_Brightness);

  WiFi.mode(WIFI_AP);                                    // WiFi Mode Access Point
  delay (100);
  WiFi.softAP(ssid, password); // AP name and password
  Serial.println("Start WLAN AP");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  Display_Main();
  update = true;

  // Create task for core 0, loop() runs on core 1
  xTaskCreatePinnedToCore(
    Get_JSON_DataTask, /* Function to implement the task */
    "Task1", /* Name of the task */
    10000,  /* Stack size in words */
    NULL,  /* Task input parameter */
    0,  /* Priority of the task */
    &Task1,  /* Task handle. */
    0); /* Core where the task should run */
}

unsigned long update_time = 0;

void Get_JSON_DataTask(void * parameter) {    // Get JSON data endlessly

  while (true) {

    if (millis() > update_time + 1000) {
      Get_JSON_Data();
      update_time = millis();
    }
  }
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
    return;
  }

  connection = 0;

  // Send HTTP request
  client.println(F("GET /cm?cmnd=status%208 HTTP/1.0"));
  client.println(F("Host: arduinojson.org"));
  client.println(F("Connection: close"));
  if (client.println() == 0) {
    Serial.println(F("Failed to send request"));
    return;
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.0 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
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
  // Extract values

  Power.Total = root["StatusSNS"]["ENERGY"]["Total"] ;
  Power.Power = root["StatusSNS"]["ENERGY"]["Power"] ;
  Power.Voltage = root["StatusSNS"]["ENERGY"]["Voltage"] ;
  Power.Current = root["StatusSNS"]["ENERGY"]["Current"] ;

  // Disconnect
  client.stop();
}


void EnergyReset(void) {  // Reset energy total count on Sonoff POW to zero

  WiFiClient client;
  client.setTimeout(500);

  // Connect to HTTP server

  if (!client.connect(host, port)) {
    Serial.println(F("Connection failed"));
    return;
  }


  // Send HTTP request
  client.println(F("GET /cm?cmnd=EnergyReset%203 HTTP/1.0"));
  client.println(F("Host: arduinojson.org"));
  client.println(F("Connection: close"));
  if (client.println() == 0) {
    Serial.println(F("Failed to send request"));
    return;
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.0 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Disconnect
  client.stop();
}


void DisplayData(void) {
  M5.Lcd.fillRect(0, 31, 320, 178, 0x139);
  M5.Lcd.setCursor(0, 50, 2);
  M5.Lcd.setTextSize(3);
  M5.Lcd.printf(" %3.0f V %5.2f A\n", Power.Voltage, Power.Current);
  M5.Lcd.printf("%7.1f W\n", Power.Power);
  M5.Lcd.printf("%6.3f kWh", Power.Total - 0.003);
}


void DiplayAlarmState(void) {
  M5.Lcd.setTextFont(1);
  M5.Lcd.setTextSize(2);                                      /* Set text size to 2 */
  M5.Lcd.setTextColor(WHITE);                                 /* Set text color to white */
  M5.Lcd.fillRect(265, 0, 320, 30, 0x1E9F);

  M5.Lcd.setCursor(265, 7);
  if (connection > 0) M5.Lcd.printf("C:%1.0d", connection); else  M5.Lcd.printf("    ");

  M5.Lcd.fillRect(0, 210, 320, 30, 0x1E9F);
  M5.Lcd.setCursor(10, 218);
  if (alarmstate) M5.Lcd.print("Alarm=On  "); else M5.Lcd.print("Alarm=Off ");
  M5.Lcd.setCursor(225, 218);
  M5.Lcd.printf("Dim=%3.0d ", LCD_Brightness);
}



void Display_Main (void)
{
  M5.Lcd.setTextFont(1);
  M5.Lcd.fillRect(0, 0, 320, 30, 0x1E9F);                      /* Upper dark blue area */
  M5.Lcd.fillRect(0, 31, 320, 178, 0x139);                   /* Main light blue area */
  M5.Lcd.fillRect(0, 210, 320, 30, 0x1E9F);                    /* Lower dark blue area */
  M5.Lcd.fillRect(0, 30, 320, 4, 0xffff);                     /* Upper white line */
  M5.Lcd.fillRect(0, 208, 320, 4, 0xffff);                    /* Lower white line */
  //  M5.Lcd.fillRect(0, 92, 320, 4, 0xffff);                     /* First vertical white line */
  //  M5.Lcd.fillRect(0, 155, 320, 4, 0xffff);                    /* Second vertical white line */
  //  M5.Lcd.fillRect(158, 30, 4, 178, 0xffff);                   /* First horizontal white line */
  M5.Lcd.setTextSize(2);                                      /* Set text size to 2 */
  M5.Lcd.setTextColor(WHITE);                                 /* Set text color to white */
  M5.Lcd.setCursor(10, 7);                                    /* Display header info */
  M5.Lcd.print("Power Display");
  //M5.Lcd.setCursor(210, 7);
  //M5.Lcd.print("Batt");
}


void loop() {
  M5.update();

  if (update == true && connection > 1 && alarmstate == true) {  // Power off alarm
    M5.Speaker.tone(661, 500);
  }

  if (update) {     // Received new data via JSON from Sonoff POW
    DisplayData();
    DiplayAlarmState();
    update = false;
  }


  if (M5.BtnA.wasPressed() == true) {       // Button A pressed ? --> Change alarm state
    M5.Speaker.tone(600, 10);
    alarmstate = !alarmstate;
    update = true;
    preferences.begin("nvs", false);
    preferences.putInt("alarmstate", alarmstate);
    preferences.end();
  }


  if (M5.BtnB.wasPressed() == true) {       // Button B pressed ? --> Reset energy to 0
    M5.Speaker.tone(600, 5);
    EnergyReset();
    update = true;
  }


  if (M5.BtnC.wasPressed() == true)                         /* Button C pressed ? --> Change brightness */
  {
    M5.Speaker.tone(600, 10);

    if (LCD_Brightness < 250)                               /* Maximum brightness not reached ? */
    {
      LCD_Brightness = LCD_Brightness + 10;                 /* Increase brightness */
    }
    else                                                    /* Maximum brightness reached ? */
    {
      LCD_Brightness = 10;                                  /* Set brightness to lowest value */
    }
    M5.Lcd.setBrightness(LCD_Brightness);                   /* Change brightness value */

    preferences.begin("nvs", false);
    preferences.putInt("LCD_State", LCD_Brightness);
    preferences.end();
    update = true;
  }

  if (connection > 10) ESP.restart();       // No connection. Restart after 10 tries!!
}
