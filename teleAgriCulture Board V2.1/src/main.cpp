/*\
 *
 * TeleAgriCulture Board Firmware
 *
 * Copyright (c) 2023 artdanion
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * For Board ID, API KEY, GPIOs and implemented Sensors see sensor_Board.h
\*/

/*
      First UPLOAD: create Connectors Table, after this it can be commented out (loads from SPIFF)

      TODO: change CA handling than both errors are gone (MBEDTLS is configured in espressif IDE and precompiled)
      two errors show up: httpClient.cpp : (if _cacert==NULL) --> comment out .... wc.insecure()  .... just the else path is used
                          ssl_client.cpp : comment out ---> #ifndef MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED #endif

*/

#include <Arduino.h>
#include <FS.h>
#include "SPIFFS.h"
#include <vector>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include <time.h>
// #include <ESP32Time.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LoraMessage.h>

#include <sensor_Board.hpp>
#include <sensor_Read.hpp>

#include <WiFiManager.h>
#include <tac_logo.h>
#include <customTitle_picture.h>
#include <WiFiManagerTz.h>

#include <Wire.h>
#include <SPI.h>

#include <Adafruit_GFX.h>
#include <FreeSans12pt.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Adafruit_ST7735.h>

// ----- Deep Sleep related -----//
#define BUTTON_PIN_BITMASK 0x10001 // GPIOs 0 and 16
#define uS_TO_S_FACTOR 1000000     /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 20           /* Time ESP32 will go to sleep (in seconds) */

// ESP32Time rtc;
RTC_DATA_ATTR int bootCount = 0;

void print_wakeup_reason();
void print_GPIO_wake_up();
// ----- Deep Sleep related -----//

// ----- Function declaration -----//

void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void on_time_available(struct timeval *t);
void printDigits(int digits);
void digitalClockDisplay(int x, int y);
void mainPage();
void checkButton(void);
void load_Sensors(void);
void load_Connectors(void);
void save_Connectors(void);
void checkLoadedStuff(void);
void printConnectors(ConnectorType typ);
void printProtoSensors(void);
void showSensors(ConnectorType type);
void load_WiFiConfig(void);
void save_WiFIConfig(void);
void addMeassurments(String name, float value);
void printMeassurments(void);
void wifi_sendData(void);
void lora_sendData(void);

// ----- Function declaration -----//

// ----- Initialize TFT ----- //
#define ST7735_TFTWIDTH 128
#define ST7735_TFTHEIGHT 160
#define background_color 0x07FF

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, MOSI, SCLK, TFT_RST);

int backlight_pwm = 255;

bool display_state = true; // Display is awake when true and sleeping when false
// ----- Initialize TFT ----- //

// ----- WiFiManager section ---- //
#define WM_NOHELP 1

char test_input[6];
bool portalRunning = false;
bool _enteredConfigMode = false;

String lastUpload;

WiFiManager wifiManager;

WiFiManagerParameter p_lineBreak_notext("<p></p>");
WiFiManagerParameter p_lineBreak_text("<p>Choose one of the following options:</p>");
WiFiManagerParameter p_Test_Input("testInput", "Test Input", test_input, 5);
WiFiManagerParameter p_wifi("wifi", "wifi", "T", 2, "type=\"checkbox\" ", WFM_LABEL_AFTER);
WiFiManagerParameter p_lora("lora", "lora", "T", 2, "type=\"checkbox\" ", WFM_LABEL_AFTER);
// ----- WiFiManager section ---- //

void setup()
{
   Serial.begin(115200);
   delay(1000); // Take some time to open up the Serial Monitor

   // Increment boot number and print it every reboot
   ++bootCount;
   Serial.println("Boot number: " + String(bootCount));

   // Print the wakeup reason for ESP32
   // print_wakeup_reason();
   // print_GPIO_wake_up();

   esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ALL_LOW);
   esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
   Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

   pinMode(BATSENS, INPUT_PULLDOWN);
   pinMode(TFT_BL, OUTPUT);
   pinMode(LED, OUTPUT);
   // pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
   // pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);

   load_Sensors(); // Prototypes get loaded

   analogWrite(TFT_BL, backlight_pwm);
   digitalWrite(LED, HIGH);

   // ----- Initiate the TFT display and Start Image----- //
   tft.initR(INITR_GREENTAB); // work around to set protected offset values
   tft.initR(INITR_BLACKTAB); // change the colormode back, offset values stay as "green display"

   tft.cp437(true);
   tft.setCursor(0, 0);
   tft.setRotation(3);
   tft.fillScreen(background_color);
   tft.drawRGBBitmap(0, 0, tac_logo, 160, 128);
   // ----- Initiate the TFT display  and Start Image----- //

   delay(4000);

   // Initialize SPIFFS file system
   if (!SPIFFS.begin(true))
   {
      Serial.println("Failed to mount SPIFFS file system");
      return;
   }

   listDir(SPIFFS, "/", 0);
   Serial.println();

   // ---  Test DATA for connected Sensors ---> comes from Web Config normaly or out of SPIFF
   // commented out after first upload

   // I2C_con_table[0] = BM280;
   // I2C_con_table[1] = VEML7700;
   // I2C_con_table[2] = NO;
   // I2C_con_table[3] = VEML7700;

   // ADC_con_table[0] = CAP_SOIL;
   // ADC_con_table[1] = TDS;
   // ADC_con_table[2] = NO;

   // OneWire_con_table[0] = DHT_22;
   // OneWire_con_table[1] = DS18B20;
   // OneWire_con_table[2] = NO;

   // SPI_con_table[0] = NO;

   // I2C_5V_con_table[0] = NO;

   // EXTRA_con_table[0] = NO;
   // EXTRA_con_table[1] = NO;

   // save_Connectors(); // <------------------ called normaly after Web Config

   // Test DATA for connected Sensors ---> come from Web Config normaly

   load_Connectors(); // Connectors lookup table
   Serial.println();

   // checkLoadedStuff();
   //  load_WiFiConfig();

   // reset settings - wipe stored credentials for testing
   // these are stored by the esp library
   // wifiManager.resetSettings();
   // wifiManager.setDebugOutput(false);

   // optionally attach external RTC update callback
   WiFiManagerNS::NTP::onTimeAvailable(&on_time_available);

   // attach board-setup page to the WiFi Manager
   WiFiManagerNS::init(&wifiManager);

   wifiManager.setHostname(hostname.c_str());
   wifiManager.setTitle("Board Config");
   wifiManager.setCustomHeadElement(custom_Title_Html.c_str());

   // wifiManager.setDebugOutput(false);
   wifiManager.setCleanConnect(true);

   // /!\ make sure "custom" is listed there as it's required to pull the "Board Setup" button
   std::vector<const char *> menu = {"custom", "wifi", "info", "update", "restart", "exit"};
   wifiManager.setMenu(menu);

   // wifiManager.setShowInfoUpdate(false);
   wifiManager.setDarkMode(true);

   // some WiFi AP test parameters
   wifiManager.addParameter(&p_lineBreak_text); // linebreak
   wifiManager.addParameter(&p_wifi);
   wifiManager.addParameter(&p_lora);
   wifiManager.addParameter(&p_lineBreak_notext); // linebreak
   wifiManager.addParameter(&p_Test_Input);

   wifiManager.autoConnect("TeleAgriCulture Board", "concesso");

   Serial.println("");
   Serial.println("WiFi connected");
   Serial.println("IP address: ");
   Serial.println(WiFi.localIP());

   delay(1000);
   if (WiFi.status() == WL_CONNECTED)
   {
      WiFiManagerNS::configTime();
   }

   printMeassurments();
   Serial.println();

   //   tft.fillScreen(background_color);
   mainPage();
   // showSensors(ConnectorType::I2C);
}

time_t prevDisplay = 0; // when the digital clock was displayed

void loop()
{
   unsigned long currentMillis = millis();

   if (currentMillis - previousMillis >= interval)
   {
      // print
      previousMillis = currentMillis;
   }

   delay(100);

   if (timeStatus() != timeNotSet)
   {
      if (now() != prevDisplay)
      { // update the display only if time has changed
         prevDisplay = now();
         digitalClockDisplay(5, 75);
      }
   }
   // Serial.println(digitalRead(LEFT_BUTTON_PIN));
   // Serial.println(digitalRead(RIGHT_BUTTON_PIN));
   // digitalWrite(LED, LOW);
   // backlight_pwm = 0;
   // analogWrite(TFT_BL, backlight_pwm);

   // Serial.println("Going to sleep now");
   // Serial.flush();
   // esp_deep_sleep_start();
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
   Serial.printf("Listing directory: %s\r\n", dirname);

   File root = fs.open(dirname);
   if (!root)
   {
      Serial.println("− failed to open directory");
      return;
   }
   if (!root.isDirectory())
   {
      Serial.println(" − not a directory");
      return;
   }

   File file = root.openNextFile();
   while (file)
   {
      if (file.isDirectory())
      {
         Serial.print("  DIR : ");
         Serial.println(file.name());
         if (levels)
         {
            listDir(fs, file.name(), levels - 1);
         }
      }
      else
      {
         Serial.print("  FILE: ");
         Serial.print(file.name());
         Serial.print("\tSIZE: ");
         Serial.println(file.size());
      }
      file = root.openNextFile();
   }
}

void readFile(fs::FS &fs, const char *path)
{
   Serial.printf("Reading file: %s\r\n", path);

   File file = fs.open(path);
   if (!file || file.isDirectory())
   {
      Serial.println("− failed to open file for reading");
      return;
   }

   Serial.println("− read from file:");
   while (file.available())
   {
      Serial.write(file.read());
   }
}

// Optional callback function, fired when NTP gets updated.
// Used to print the updated time or adjust an external RTC module.
void on_time_available(struct timeval *t)
{
   Serial.println("Received time adjustment from NTP");
   struct tm timeInfo;
   getLocalTime(&timeInfo, 1000);
   Serial.println(&timeInfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
   lastUpload=String(timeInfo.tm_hour) + ":" + String(timeInfo.tm_min) + ":" + String(timeInfo.tm_sec);
   setTime(timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec, timeInfo.tm_mday, timeInfo.tm_mon + 1, timeInfo.tm_year + 1900);

   // every hour
   sensorRead();
   wifi_sendData();
}

void digitalClockDisplay(int x, int y)
{
   tft.setTextSize(1);
   tft.setCursor(x, y);
   tft.setTextColor(ST7735_BLACK);

   tft.fillRect(x - 5, y - 5, 120, 15, background_color);

   tft.print(hour());
   printDigits(minute());
   printDigits(second());
   tft.print(" ");
   tft.print(day());
   tft.print(" ");
   tft.print(month());
   tft.print(" ");
   tft.print(year());
   tft.println();
}

void printDigits(int digits)
{
   // utility for digital clock display: prints preceding colon and leading 0
   tft.print(":");
   if (digits < 10)
      tft.print('0');
   tft.print(digits);
}

void checkButton()
{
   // is auto timeout portal running
   if (portalRunning)
   {
      wifiManager.process();
   }

   // is configuration portal requested?
   if (digitalRead(LEFT_BUTTON_PIN) == LOW)
   {
      delay(50);
      if (digitalRead(LEFT_BUTTON_PIN) == LOW)
      {
         if (!portalRunning)
         {
            Serial.println("Button Pressed, Starting Portal");
            WiFi.mode(WIFI_STA);
            wifiManager.startWebPortal();
            portalRunning = true;
         }
         else
         {
            Serial.println("Button Pressed, Stopping Portal");
            wifiManager.stopWebPortal();
            portalRunning = false;
         }
      }
   }
}

void print_GPIO_wake_up()
{
   uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
   Serial.print("GPIO that triggered the wake up: GPIO ");
   Serial.println((log(GPIO_reason)) / log(2), 0);
}

void print_wakeup_reason()
{
   esp_sleep_wakeup_cause_t wakeup_reason;

   wakeup_reason = esp_sleep_get_wakeup_cause();

   switch (wakeup_reason)
   {
   case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Wakeup caused by external signal using RTC_IO");
      break;
   case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("Wakeup caused by external signal using RTC_CNTL");
      break;
   case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Wakeup caused by timer");
      break;
   case ESP_SLEEP_WAKEUP_TOUCHPAD:
      Serial.println("Wakeup caused by touchpad");
      break;
   case ESP_SLEEP_WAKEUP_ULP:
      Serial.println("Wakeup caused by ULP program");
      break;
   default:
      Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
      break;
   }
}

void load_Sensors()
{
   DynamicJsonDocument doc(JSON_BUFFER);

   DeserializationError error = deserializeJson(doc, proto_sensors);

   // Check for parsing errors
   if (error)
   {
      Serial.print(F("Failed to parse JSON: "));
      Serial.println(error.c_str());
      return;
   }

   for (size_t i = 0; i < SENSORS_NUM; i++)
   {
      JsonObject sensorObj = doc[i];

      allSensors[i].sensor_id = NO;
      allSensors[i].sensor_name = " ";

      allSensors[i].sensor_id = sensorObj["sensor-id"].as<SensorsImplemented>();
      allSensors[i].sensor_name = sensorObj["name"].as<String>();
      allSensors[i].con_typ = sensorObj["con_typ"].as<String>();
      allSensors[i].returnCount = sensorObj["returnCount"].as<int>();

      if (sensorObj.containsKey("measurements"))
      {
         JsonArray measurementsArr = sensorObj["measurements"].as<JsonArray>();
         size_t numMeasurements = measurementsArr.size();

         for (size_t j = 0; j < numMeasurements; j++)
         {
            JsonObject measurementObj = measurementsArr[j].as<JsonObject>();
            if (!measurementObj.containsKey("data_name") || !measurementObj.containsKey("value") || !measurementObj.containsKey("unit"))
            {
               // The measurement object is missing one or more required fields, handle the error here...
               continue;
            }

            allSensors[i].measurements[j].data_name = measurementObj["data_name"].as<String>();
            allSensors[i].measurements[j].value = measurementObj["value"].as<float>();
            allSensors[i].measurements[j].unit = measurementObj["unit"].as<String>();
         }
      }
      if (sensorObj.containsKey("i2c_add"))
      {
         allSensors[i].i2c_add = sensorObj["i2c_add"].as<String>();
      }
      if (sensorObj.containsKey("possible_i2c_add"))
      {
         JsonArray i2c_alt_Arr = sensorObj["possible_i2c_add"].as<JsonArray>();
         size_t numI2Cadd = i2c_alt_Arr.size();

         for (size_t j = 0; j < numI2Cadd; j++)
         {
            JsonObject i2c_alt_Obj = i2c_alt_Arr[j].as<JsonObject>();
            if (!i2c_alt_Obj.containsKey("standard"))
            {
               // The measurement object is missing one or more required fields, handle the error here...
               continue;
            }

            if (i2c_alt_Obj.containsKey("standard"))
            {
               allSensors[i].possible_i2c_add[0] = i2c_alt_Obj["standard"].as<String>();
            }

            if (i2c_alt_Obj.containsKey("alt_1"))
            {
               allSensors[i].possible_i2c_add[1] = i2c_alt_Obj["alt_1"].as<String>();
            }
            if (i2c_alt_Obj.containsKey("alt_2"))
            {
               allSensors[i].possible_i2c_add[2] = i2c_alt_Obj["alt_2"].as<String>();
            }
         }
      }
   }

   DynamicJsonDocument deallocate(doc);
}

void load_Connectors()
{
   if (SPIFFS.begin())
   {
      Serial.println("mounted file system");
      if (SPIFFS.exists("/connectors.json"))
      {
         // file exists, reading and loading
         Serial.println("reading config file");
         File connectorsFile = SPIFFS.open("/connectors.json", "r");
         if (connectorsFile)
         {
            Serial.println("opened config file");
            size_t size = connectorsFile.size();
            // Allocate a buffer to store contents of the file.
            std::unique_ptr<char[]> buf(new char[size]);

            connectorsFile.readBytes(buf.get(), size);

            // DynamicJsonDocument json(CON_BUFFER);
            StaticJsonDocument<800> doc;
            auto deserializeError = deserializeJson(doc, buf.get());
            serializeJson(doc, Serial);
            if (!deserializeError)
            {
               Serial.println("\nparsed json");

               JsonArray jsonI2C_connectors = doc[0]["I2C_connectors"];
               for (int i = 0; i < I2C_NUM; i++)
               {
                  I2C_con_table[i] = jsonI2C_connectors[i];
               }

               JsonArray jsonOneWire_connectors = doc[1]["OneWire_connectors"];
               for (int i = 0; i < ONEWIRE_NUM; i++)
               {
                  OneWire_con_table[i] = jsonOneWire_connectors[i];
               }

               JsonArray jsonADC_connectors = doc[2]["ADC_connectors"];
               for (int i = 0; i < ADC_NUM; i++)
               {
                  ADC_con_table[i] = jsonADC_connectors[i];
               }

               SPI_con_table[0] = doc[3]["SPI_connectors"][0];

               I2C_5V_con_table[0] = doc[4]["I2C_5V_connectors"][0];

               EXTRA_con_table[0] = doc[5]["EXTRA_connectors"][0];
               EXTRA_con_table[1] = doc[5]["EXTRA_connectors"][1];
            }
            else
            {
               Serial.println("failed to load json config");
            }
            connectorsFile.close();
         }
      }
   }
   else
   {
      Serial.println("failed to mount FS");
   }
   // end read
}

void save_Connectors()
{
   // save the custom parameters to FS

   Serial.println("saving Connectors");
   StaticJsonDocument<800> doc;

   JsonArray jsonI2C_connectors = doc[0].createNestedArray("I2C_connectors");
   for (int j = 0; j < I2C_NUM; j++)
   {
      jsonI2C_connectors.add(I2C_con_table[j]);
   }

   JsonArray jsonOneWire_connectors = doc[1].createNestedArray("OneWire_connectors");
   for (int i = 0; i < ONEWIRE_NUM; i++)
   {
      jsonOneWire_connectors.add(OneWire_con_table[i]);
   }
   JsonArray jsonADC_connectors = doc[2].createNestedArray("ADC_connectors");
   for (int i = 0; i < ADC_NUM; i++)
   {
      jsonADC_connectors.add(ADC_con_table[i]);
   }

   doc[3]["SPI_connectors"][0] = SPI_con_table[0];
   doc[4]["I2C_5V_connectors"][0] = I2C_5V_con_table[0];

   JsonArray jsonEXTRA_connectors = doc[5].createNestedArray("EXTRA_connectors");
   jsonEXTRA_connectors.add(EXTRA_con_table[0]);
   jsonEXTRA_connectors.add(EXTRA_con_table[1]);

   File connectorsFile = SPIFFS.open("/connectors.json", "w");
   if (!connectorsFile)
   {
      Serial.println("failed to open config file for writing");
   }

   serializeJson(doc, Serial);
   serializeJson(doc, connectorsFile);

   Serial.println("Connector Table Saved!");

   connectorsFile.close();
   // end save
}

void mainPage()
{
   tft.fillScreen(background_color);
   tft.setTextColor(ST7735_BLACK);
   tft.setFont(&FreeSans9pt7b);
   tft.setTextSize(1);
   tft.setCursor(5, 17);
   tft.print("TeleAgriCulture");
   tft.setCursor(5, 38);
   tft.print("Board V2.1");

   tft.setTextColor(ST7735_BLUE);
   tft.setFont();
   tft.setTextSize(1);
   tft.setCursor(5, 45);
   tft.print(version);
   tft.setTextColor(ST7735_BLACK);
   tft.setCursor(5, 60);
   tft.print("IP: ");
   tft.print(WiFi.localIP());
   tft.setCursor(5, 90);
   tft.print("last data UPLOAD: ");
   tft.print(lastUpload);
}

void printConnectors(ConnectorType typ)
{
   switch (typ)
   {
   case ConnectorType::I2C:
   {
      Serial.println("---- I2C ----");
      for (int i = 0; i < I2C_NUM; i++)
      {
         Serial.print("I2C_");
         Serial.print(i + 1);

         if (I2C_con_table[i] == NO)
         {
            Serial.println("  NO");
         }
         else
         {
            Serial.print("  ");
            Serial.print(allSensors[I2C_con_table[i]].sensor_name);
            Serial.print(" (");
            Serial.print(allSensors[I2C_con_table[i]].con_typ);
            Serial.print(") - ");
            for (int j = 0; j < allSensors[I2C_con_table[i]].returnCount; j++)
            {
               Serial.print(allSensors[I2C_con_table[i]].measurements[j].data_name);
               Serial.print(": ");
               Serial.print(allSensors[I2C_con_table[i]].measurements[j].value);
               Serial.print(allSensors[I2C_con_table[i]].measurements[j].unit);
               Serial.print(", ");
            }
            Serial.println();
         }
      }
   }
   break;

   case ConnectorType::ADC:
   {

      Serial.println("---- ADC ----");
      for (int i = 0; i < ADC_NUM; i++)
      {
         Serial.print("ADC_");
         Serial.print(i + 1);

         if (ADC_con_table[i] == NO)
         {
            Serial.println("  NO");
         }
         else
         {
            Serial.print("  ");
            Serial.print(allSensors[ADC_con_table[i]].sensor_name);
            Serial.print(" (");
            Serial.print(allSensors[ADC_con_table[i]].con_typ);
            Serial.print(") - ");
            for (int j = 0; j < allSensors[ADC_con_table[i]].returnCount; j++)
            {
               Serial.print(allSensors[ADC_con_table[i]].measurements[j].data_name);
               Serial.print(": ");
               Serial.print(allSensors[ADC_con_table[i]].measurements[j].value);
               Serial.print(allSensors[ADC_con_table[i]].measurements[j].unit);
               Serial.print(", ");
            }
            Serial.println();
         }
      }
   }
   break;

   case ConnectorType::ONE_WIRE:
   {
      Serial.println("---- 1-Wire ----");
      for (int i = 0; i < ONEWIRE_NUM; i++)
      {
         Serial.print("1-Wire_");
         Serial.print(i + 1);

         if (OneWire_con_table[i] == NO)
         {
            Serial.println("  NO");
         }
         else
         {
            Serial.print("  ");
            Serial.print(allSensors[OneWire_con_table[i]].sensor_name);
            Serial.print(" (");
            Serial.print(allSensors[OneWire_con_table[i]].con_typ);
            Serial.print(") - ");
            for (int j = 0; j < allSensors[OneWire_con_table[i]].returnCount; j++)
            {
               Serial.print(allSensors[OneWire_con_table[i]].measurements[j].data_name);
               Serial.print(": ");
               Serial.print(allSensors[OneWire_con_table[i]].measurements[j].value);
               Serial.print(allSensors[OneWire_con_table[i]].measurements[j].unit);
               Serial.print(", ");
            }
            Serial.println();
         }
      }
   }
   break;

   case ConnectorType::I2C_5V:
   {
      Serial.println("---- I2C_5V ----");
      Serial.print("I2C_5V");

      if (I2C_5V_con_table[0] == NO)
      {
         Serial.println("  NO");
      }
      else
      {
         Serial.print("  ");
         Serial.print(allSensors[I2C_5V_con_table[0]].sensor_name);
         Serial.print(" (");
         Serial.print(allSensors[I2C_5V_con_table[0]].con_typ);
         Serial.print(") - ");

         for (int j = 0; j < allSensors[I2C_5V_con_table[0]].returnCount; j++)
         {
            Serial.print(allSensors[I2C_5V_con_table[0]].measurements[j].data_name);
            Serial.print(": ");
            Serial.print(allSensors[I2C_5V_con_table[0]].measurements[j].value);
            Serial.print(allSensors[I2C_5V_con_table[0]].measurements[j].unit);
            Serial.print(", ");
         }
         Serial.println();
      }
   }
   break;

   case ConnectorType::SPI_CON:
   {
      Serial.println("---- SPI CON ----");
      Serial.print("SPI");

      if (SPI_con_table[0] == NO)
      {
         Serial.println("  NO");
      }
      else
      {
         Serial.print("  ");
         Serial.print(allSensors[SPI_con_table[0]].sensor_name);
         Serial.print(" (");
         Serial.print(allSensors[SPI_con_table[0]].con_typ);
         Serial.print(") - ");

         for (int j = 0; j < allSensors[SPI_con_table[0]].returnCount; j++)
         {
            Serial.print(allSensors[SPI_con_table[0]].measurements[j].data_name);
            Serial.print(": ");
            Serial.print(allSensors[SPI_con_table[0]].measurements[j].value);
            Serial.print(allSensors[SPI_con_table[0]].measurements[j].unit);
            Serial.print(", ");
         }
         Serial.println();
      }
   }
   break;

   case ConnectorType::EXTRA:
   {
      Serial.println("---- EXTRA CON ----");
      for (int i = 0; i < EXTRA_NUM; i++)
      {
         Serial.print("EXTRA_");
         Serial.print(i + 1);

         if (EXTRA_con_table[i] == NO)
         {
            Serial.println("  NO");
         }
         else
         {
            Serial.print("  ");
            Serial.print(allSensors[EXTRA_con_table[i]].sensor_name);
            Serial.print(" (");
            Serial.print(allSensors[EXTRA_con_table[i]].con_typ);
            Serial.print(") - ");
            for (int j = 0; j < allSensors[EXTRA_con_table[i]].returnCount; j++)
            {
               Serial.print(allSensors[EXTRA_con_table[i]].measurements[j].data_name);
               Serial.print(": ");
               Serial.print(allSensors[EXTRA_con_table[i]].measurements[j].value);
               Serial.print(allSensors[EXTRA_con_table[i]].measurements[j].unit);
               Serial.print(", ");
            }
            Serial.println();
         }
      }
   }
   break;

   default:
      break;
   }
}

void printProtoSensors(void)
{
   for (int z = 0; z < SENSORS_NUM; z++)
   {
      Serial.print(allSensors[z].sensor_id);
      Serial.print("  ");
      Serial.print(allSensors[z].sensor_name);
      Serial.print("  ");
      Serial.print(allSensors[z].con_typ);
      Serial.print(" returnCount:  ");
      Serial.println(allSensors[z].returnCount);

      for (size_t j = 0; j < allSensors[z].returnCount; j++)
      {
         Serial.print("         ");
         Serial.print(allSensors[z].measurements[j].data_name);
         Serial.print("  ");
         Serial.print(allSensors[z].measurements[j].value);
         Serial.println(allSensors[z].measurements[j].unit);
      }
   }
   Serial.println();
}

void showSensors(ConnectorType sensor_type)
{
   tft.fillScreen(ST7735_BLACK);
   tft.setTextSize(1);

   int cursor_y = 5;

   for (int i = 0; i < SENSORS_NUM; i++)
   {
      if (sensor_type == ConnectorType::I2C && !(allSensors[i].con_typ == "I2C"))
      {
         continue;
      }
      if (sensor_type == ConnectorType::ONE_WIRE && !(allSensors[i].con_typ == "ONE_WIRE"))
      {
         continue;
      }
      if (sensor_type == ConnectorType::ADC && !(allSensors[i].con_typ == "ADC"))
      {
         continue;
      }
      if (sensor_type == ConnectorType::SPI_CON && !(allSensors[i].con_typ == "SPI"))
      {
         continue;
      }

      tft.setCursor(5, cursor_y);
      tft.setTextColor(ST7735_YELLOW);

      tft.print(allSensors[i].sensor_name);
      tft.setCursor(80, cursor_y);
      tft.setTextColor(ST7735_GREEN);

      tft.print(allSensors[i].con_typ);
      cursor_y += 10;

      for (int j = 0; j < MEASURMENT_NUM; j++)
      {
         if (allSensors[i].measurements[j].data_name == "")
         {
            continue;
         }
         tft.setCursor(30, cursor_y);
         tft.setTextColor(ST7735_BLUE);

         tft.print(allSensors[i].measurements[j].data_name);
         tft.setCursor(80, cursor_y);
         tft.setTextColor(ST7735_WHITE);

         tft.print(allSensors[i].measurements[j].value);
         tft.print(" ");
         if (allSensors[i].measurements[j].unit == "°C")
         {
            tft.drawChar(114, cursor_y - 4, 9, ST7735_WHITE, ST7735_BLACK, 1);
            tft.print(" C");
         }
         if (allSensors[i].measurements[j].unit == "°F")
         {
            tft.drawChar(114, cursor_y - 4, 9, ST7735_WHITE, ST7735_BLACK, 1);
            tft.print(" F");
         }
         if (!(allSensors[i].measurements[j].unit == "°C" || allSensors[i].measurements[j].unit == "°F"))
         {
            tft.print(allSensors[i].measurements[j].unit);
         }
         cursor_y += 10;
      }
   }
}

void checkLoadedStuff(void)
{
   Serial.println("---------------Prototype Sensors loaded -----------");
   // sensorRead();

   printProtoSensors();
   Serial.println();
   Serial.println("---------------Connector table loaded -----------");

   printConnectors(ConnectorType::I2C);
   printConnectors(ConnectorType::ADC);
   printConnectors(ConnectorType::ONE_WIRE);
   printConnectors(ConnectorType::I2C_5V);
   printConnectors(ConnectorType::SPI_CON);
   printConnectors(ConnectorType::EXTRA);

   Serial.println();
}

void saveWiFiConfig(void)
{
}

void loadWiFiConfig(void)
{
}

// Function to add a name-value pair to the global vector
void addMeassurments(String name, float value)
{
   std::pair<std::string, float> pair(name.c_str(), value);
   measuredVector.push_back(pair);
}

void printMeassurments()
{
   // Loop through all the elements in the vector
   for (auto &pair : measuredVector)
   {
      // Print the name and value using Serial.println()
      Serial.print("Name: ");
      Serial.print(pair.first.c_str());
      Serial.print(", Value: ");
      Serial.println(pair.second);
   }
}

void wifi_sendData(void)
{
   // ----- code later used to post measurements

   // StaticJsonDocument<300> docMeasures;
   // for (auto &pair : measuredVector)
   // {
   //    docMeasures[pair.first.c_str()] = pair.second;
   // }
   // Serial.println();
   // serializeJson(docMeasures, Serial);
   // Serial.println();

   // DynamicJsonDocument deallocate(docMeasures);

   // Serial.println();

   // --------------------------   test all hard coded ....

   // Check WiFi connection status
   if (WiFi.status() == WL_CONNECTED)
   {
      // WiFiClient client;
      WiFiClientSecure client;
      client.setCACert(kits_ca);

      HTTPClient https;

      StaticJsonDocument<32> doc;

      doc["test"] = 46;

      String output;

      serializeJson(doc, output);

      Serial.print("\njson Data for POST: ");
      serializeJson(doc, Serial);

      Serial.println();

      // https://gitlab.com/teleagriculture/community/-/blob/main/API.md

      // python example
      // https://gitlab.com/teleagriculture/community/-/blob/main/RPI/tacserial.py

      // should show up there:
      // https://kits.teleagriculture.org/kits/1003
      // Sensor: test

      // https.begin(client, "https://kits.teleagriculture.org/api/kits/1003/measurements");
      https.begin(client, serverName);

      https.addHeader("Content-Type", "application/json");
      https.addHeader("Authorization", api_Bearer);

      int httpResponseCode = https.POST(output);

      Serial.print("\nHTTP Response code: ");
      Serial.println(httpResponseCode);
      Serial.println();

      // Free resources
      https.end();
   }
   else
   {
      Serial.println("WiFi Disconnected");
   }
}

void lora_sendData(void)
{

   // https://github.com/thesolarnomad/lora-serialization

   byte message[measuredVector.size()];
   LoraEncoder encoder(message);

   for (auto &pair : measuredVector)
   {
      uint8_t j = 0;
      for (int i = 0; i < pair.first.size(); i++)
      {
         j = pair.first[i];
         encoder.writeUint8(j);
      }

      encoder.writeRawFloat(pair.second); // TODO: switch case to change type
   }
   Serial.println();
   Serial.print(sizeof(message));
   Serial.println();

   // TODO: lora send message
}