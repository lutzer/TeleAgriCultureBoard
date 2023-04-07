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
 *

//TODO: LoRa send to TTN
//TODO: TTN decoder
//TODO: LoRa frequency change based on UI
//TODO: Cert read and set
//TODO: WiFISecure Cert check
//TODO: Battery optimation
//TODO: Sensor test


/*
 *
 * For defines, GPIOs and implemented Sensors see sensor_Board.hpp
 *
 * Config Portal Access Point:   SSID: TeleAgriCulture Board
 *                               pasword: enter123
 *
 * main() handles Config Accesspoint, WiFi, LoRa, load, save and display UI
 *
 * Global vector to store connected Sensor data:
 * std::vector<Sensor> sensorVector;
 * Global vector to store Measurement data:
 * std::vector<Measurement> show_measurements;
/*/

/*
   to add new Sensors
   -> sensor_Board.hpp
   -> add new SensorName to ENUM SensorsImplemented (use Name that has no conflicts with your sensor library)
   -> add new Sensor to json styled proto_sensors (use same format)

   -> include SENSOR library to sensor_Read.hpp
   -> add implementation to   readI2C_Connectors()
                              readADC_Connectors()
                              readOneWire_Connectors()
                              readI2C_5V_Connector()
                              readSPI_Connector()
                              readEXTRA_Connectors()
      corresponding to your Sensortype with case statement using your Sensor ENUM
*/

// TODO: Power and timing optimisation on battery power

#include <Arduino.h>
#include <FS.h>
#include "SPIFFS.h"
#include <vector>
#include <WString.h>
#include <Ticker.h>

#include <driver/spi_master.h>
#include "sdkconfig.h"
#include "esp_system.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include <time.h>
#include <sys/time.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <lmic.h>
#include <hal/hal.h>
#include <LoraMessage.h>

#include <tac_logo.h>
#include <customTitle_picture.h>
#include <sensor_Board.hpp>
#include <sensor_Read.hpp>

#define ESP_DRD_USE_SPIFFS true
#include <ESP_DoubleResetDetector.h>
#include <Button.h>
#include <servers.h>
#include <WiFiManager.h>
#include <WiFiManagerTz.h>

#include <Wire.h>
#include <SPI.h>

#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Adafruit_ST7735.h>

// ----- Deep Sleep related -----//
#define BUTTON_PIN_BITMASK 0x10001 // GPIOs 0 and 16
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 20           /* Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;

void print_wakeup_reason();
void print_GPIO_wake_up();
// ----- Deep Sleep related -----//

// ----- LED Timer related -----//
#define TIMER_DIVIDER 16
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER)
#define BLINK_INTERVAL_MS 500
// ----- LED Timer related -----//

// ----- Function declaration -----//
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void on_time_available(struct timeval *t);
void configModeCallback(WiFiManager *myWiFiManager);
void printDigits(int digits);
void digitalClockDisplay(int x, int y, bool date);
void drawBattery(int x, int y);
void renderPage(int page);
int countMeasurements(const std::vector<Sensor> &sensors);
void mainPage(void);
void I2C_ConnectorPage(void);
void ADC_ConnectorPage(void);
void OneWire_ConnectorPage(void);
void measurementsPage(int page);
void checkButton(void);
LoraSendType getLoraSendTypeFromString(String str);
void load_Sensors(void);
void load_Connectors(void);
void save_Connectors(void);
void load_Config(void);
void save_Config(void);
void checkLoadedStuff(void);
void printConnectors(ConnectorType typ);
void printProtoSensors(void);
void showSensors(ConnectorType type);
void printMeassurments(void);
void printSensors(void);
void wifi_sendData(void);
void lora_sendData(void);
void get_time_in_timezone(const char *timezone);
int seconds_to_next_hour();
void toggleLED(void);
void startBlinking(void);
void stopBlinking(void);

String get_header();
String getDateTime(String header);
time_t convertDateTime(String dateTimeStr);
void setEsp32Time(const char *timeStr);

void printHex2(unsigned v);
void onEvent(ev_t ev);
void do_send(osjob_t *j);

// ----- Function declaration -----//

// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
static const u1_t PROGMEM APPEUI[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
void os_getArtEui(u1_t *buf) { memcpy_P(buf, APPEUI, 8); }

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8] = {0xF4, 0xA8, 0x05, 0xD0, 0x7E, 0xD5, 0xB3, 0x70};
void os_getDevEui(u1_t *buf) { memcpy_P(buf, DEVEUI, 8); }

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
static const u1_t PROGMEM APPKEY[16] = {0xDF, 0x6B, 0x2A, 0x4A, 0xC0, 0x93, 0x0B, 0xCA, 0x55, 0x14, 0x15, 0x64, 0xD7, 0x51, 0xD5, 0x78};
void os_getDevKey(u1_t *buf) { memcpy_P(buf, APPKEY, 16); }

static uint8_t mydata[] = "Hello, world!";
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 60;

static const int spiClk = 8000000; // 8 MHz

const lmic_pinmap lmic_pins = {
    .nss = LORA_CS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LORA_RST,
    .dio = {LORA_DI0, LORA_DI1, LMIC_UNUSED_PIN},
    .spi_freq = 8000000,
};

// ----- Initialize TFT ----- //
#define ST7735_TFTWIDTH 128
#define ST7735_TFTHEIGHT 160
#define background_color 0x07FF

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

int backlight_pwm = 250;
// String tempUnit = "\u00B0";

bool display_state = true; // Display is awake when true and sleeping when false
// ----- Initialize TFT ----- //

// ----- WiFiManager section ---- //
#define WM_NOHELP 1

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 5

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

DoubleResetDetector *drd;
Ticker blinker;
Button upButton(LEFT_BUTTON_PIN);
Button downButton(RIGHT_BUTTON_PIN);

WiFiManager wifiManager;

#define NUM_PAGES 4   // pages with out the measurement pages
#define NUM_PERPAGE 9 // max measurement values per page

char test_input[6];
bool portalRunning = false;
bool _enteredConfigMode = false;
bool connectorsSaved = false;
bool configSaved = false;
int total_measurement_pages = 0;

int currentPage = 0;
int lastPage = -1; // Store the last page to avoid refreshing unnecessarily
int num_pages = NUM_PAGES;

String lastUpload;
bool initialState; // state of the LED
bool ledState = false;
bool sendDataWifi = true;
bool gotoSleep = true;

unsigned long previousMillis = 0;
unsigned long upButtonsMillis = 0;
const long interval = 200000; // screen backlight darken time 20s

WiFiClientSecure client;

void setup()
{
   pinMode(BATSENS, INPUT_PULLDOWN);
   pinMode(TFT_BL, OUTPUT);
   pinMode(LED, OUTPUT);
   pinMode(LORA_CS, OUTPUT);
   
   
   // ----- Initiate the TFT display and Start Image----- //
   tft.initR(INITR_GREENTAB); // work around to set protected offset values
   tft.initR(INITR_BLACKTAB); // change the colormode back, offset values stay as "green display"

   analogWrite(TFT_BL, backlight_pwm); // turn TFT Backlight on

   tft.cp437(true);
   tft.setCursor(0, 0);
   tft.setRotation(3);
   tft.fillScreen(background_color);
   tft.drawRGBBitmap(0, 0, tac_logo, 160, 128);
   // ----- Initiate the TFT display  and Start Image----- //

   delay(1000);
   Serial.setTxTimeoutMs(5); // set USB CDC Time TX
   Serial.begin(115200);     // start Serial for debuging

   bool forceConfig = false;

   /* ------------  Test DATA for connected Sensors  --------------------------

       ---> comes from Web Config normaly or out of SPIFF

   I2C_con_table[0] = NO;
   I2C_con_table[1] = NO;
   I2C_con_table[2] = NO;
   I2C_con_table[3] = NO;
   ADC_con_table[0] = CAP_SOIL;
   ADC_con_table[1] = TDS;
   ADC_con_table[2] = TDS;
   OneWire_con_table[0] = DHT_22;
   OneWire_con_table[1] = DHT_22;
   OneWire_con_table[2] = DHT_22;
   SPI_con_table[0] = NO;
   I2C_5V_con_table[0] = MULTIGAS_V1;
   EXTRA_con_table[0] = NO;
   EXTRA_con_table[1] = NO;

   save_Connectors(); // <------------------ called normaly after Web Config

   Test DATA for connected Sensors ---> come from Web Config normaly

   ---------------------------------------------------------------------------------*/

   // Increment boot number and print it every reboot
   ++bootCount;
   // Serial.println("\nBoot number: " + String(bootCount));

   // Print the wakeup reason for ESP32
   print_wakeup_reason();
   print_GPIO_wake_up();

   drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
   if (drd->detectDoubleReset())
   {
      Serial.println(F("\nForcing config mode as there was a Double reset detected"));
      forceConfig = true;
   }

   esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ALL_LOW);

   // Initialize SPIFFS file system
   if (!SPIFFS.begin(true))
   {
      Serial.println("Failed to mount SPIFFS file system");
      return;
   }
   // listDir(SPIFFS, "/", 0);
   // Serial.println();

   load_Sensors();    // Prototypes get loaded
   load_Connectors(); // Connectors lookup table
   load_Config();     // loade config Data

   // checkLoadedStuff();

   if (customNTPaddress != NULL)
   {
      if (WiFiManagerNS::NTP::NTP_Servers.size() == NUM_PREDIFINED_NTP)
      {
         const std::string constStr = customNTPaddress.c_str();
         WiFiManagerNS::NTP::NTP_Server newServer = {"Custom NTP Server", constStr};
         WiFiManagerNS::NTP::NTP_Servers.push_back(newServer);
      }
   }

   // attach board-setup page to the WiFi Manager
   WiFiManagerNS::init(&wifiManager);

   if (WiFiManagerNS::NTPEnabled)
   {
      // optionally attach external RTC update callback
      WiFiManagerNS::NTP::onTimeAvailable(&on_time_available);
   }
   // reset settings - wipe stored credentials for testing
   // these are stored by the WiFiManager library
   // wifiManager.resetSettings();

   wifiManager.setDebugOutput(false);
   wifiManager.setHostname(hostname.c_str());
   wifiManager.setTitle("Board Config");
   wifiManager.setCustomHeadElement(custom_Title_Html.c_str());

   // set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
   wifiManager.setAPCallback(configModeCallback);

   // wifiManager.setDebugOutput(false);
   wifiManager.setCleanConnect(true);

   // /!\ make sure "custom" is listed there as it's required to pull the "Board Setup" button
   std::vector<const char *> menu = {"custom", "wifi", "info", "update", "restart", "exit"};
   wifiManager.setMenu(menu);

   // wifiManager.setShowInfoUpdate(false);
   wifiManager.setDarkMode(true);
   // wifiManager.setSaveConfigCallback([]() { configSaved = true; }); // restart on credentials save, ESP32 doesn't like to switch between AP/STA

   startBlinking();
   if (forceConfig)
   {
      if (!wifiManager.startConfigPortal("TeleAgriCulture Board", "enter123"))
      {
         Serial.println("failed to connect and hit timeout");
         delay(3000);
         // reset and try again, or maybe put it to deep sleep
         ESP.restart();
         delay(5000);
      }
   }
   else
   {
      if (!wifiManager.autoConnect("TeleAgriCulture Board", "enter123"))
      {
         Serial.println("failed to connect and hit timeout");
         delay(3000);
         // if we still have not connected restart and try all over again
         ESP.restart();
         delay(5000);
      }
   }
   stopBlinking();

   delay(100);

   if (upload == "WIFI")
   {
      if (WiFiManagerNS::NTPEnabled)
      {
         if (WiFi.status() == WL_CONNECTED)
         {
            WiFiManagerNS::configTime();
            struct tm timeInfo;
            getLocalTime(&timeInfo, 1000);
            Serial.println(&timeInfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
            setTime(timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec, timeInfo.tm_mday, timeInfo.tm_mon + 1, timeInfo.tm_year + 1900);

            // every hour
            sendDataWifi = true;

            lastUpload = "";

            if (timeInfo.tm_hour < 10)
               lastUpload += '0';
            lastUpload += String(timeInfo.tm_hour) + ':';

            if (timeInfo.tm_min < 10)
               lastUpload += '0';
            lastUpload += String(timeInfo.tm_min) + ':';

            if (timeInfo.tm_sec < 10)
               lastUpload += '0';

            lastUpload += String(timeInfo.tm_sec);
         }
      }
      else
      {
         String header = get_header();
         String Time1 = getDateTime(header);
         setEsp32Time(Time1.c_str());
      }

      sendDataWifi = true;
   }

   upButton.begin();
   downButton.begin();

   // LMIC init
   os_init();
   // Reset the MAC state. Session and pending data transfers will be discarded.
   LMIC_reset();

   // Start job (sending automatically starts OTAA too)
   do_send(&sendjob);
}

time_t prevDisplay = 0; // when the digital clock was displayed

void loop()
{
   drd->loop();
   analogWrite(TFT_BL, backlight_pwm); // turn TFT Backlight on

   os_runloop_once();

   unsigned long currentMillis = millis();

   if (currentMillis - previousMillis >= interval)
   {
      backlight_pwm = 5; // turns Backlight down
      previousMillis = currentMillis;
      gotoSleep = true;
   }

   show_measurements.clear();

   for (int i = 0; i < sensorVector.size(); i++)
   {
      for (int j = 0; j < sensorVector[i].returnCount; j++)
      {
         show_measurements.push_back(sensorVector[i].measurements[j]);
      }
   }

   total_measurement_pages = ceil(show_measurements.size() / NUM_PERPAGE) + 1;

   num_pages = NUM_PAGES + total_measurement_pages;

   if (downButton.pressed())
   {
      currentPage = (currentPage + 1) % num_pages;
      backlight_pwm = 250;
      gotoSleep = false;
   }

   if (upButton.pressed())
   {
      currentPage = (currentPage - 1 + num_pages) % num_pages;
      backlight_pwm = 250;
      upButtonsMillis = millis();
      gotoSleep = false;
   }

   if (upButton.released())
   {
      if (millis() - upButtonsMillis > 5000)
      {
         startBlinking();
         if (!wifiManager.startConfigPortal("TeleAgriCulture Board", "enter123"))
         {
            Serial.println("failed to connect and hit timeout");
            delay(3000);
            // reset and try again, or maybe put it to deep sleep
            ESP.restart();
            delay(5000);
         }
      }
      upButtonsMillis = 0;
      stopBlinking();
   }

   // Only refresh the screen if the current page has changed
   if (currentPage != lastPage)
   {
      lastPage = currentPage;
      renderPage(currentPage);
      Serial.println(seconds_to_next_hour());
   }

   if (currentPage == 0)
   {
      if (now() != prevDisplay)
      { // update the display only if time has changed
         prevDisplay = now();
         digitalClockDisplay(5, 95, false);
      }
   }

   if (seconds_to_next_hour() < 1)
   {
      // execute task
      sendDataWifi = true;
   }

   if (sendDataWifi)
   {
      sensorRead();
      // Serial.println("\nPrint Measurements: ");
      // printMeassurments();
      // Serial.println("\nPrint Sensors connected: ");
      // printSensors();
      // Serial.println();
      wifi_sendData();
      sendDataWifi = false;

      struct tm timeinfo;
      getLocalTime(&timeinfo);

      // Serial.println("\nESP Time set via HTTP get:");
      // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
      if (WiFiManagerNS::NTPEnabled)
      {
         lastUpload = "";

         if (timeinfo.tm_hour < 10)
            lastUpload += '0';
         lastUpload += String(timeinfo.tm_hour) + ':';

         if (timeinfo.tm_min < 10)
            lastUpload += '0';
         lastUpload += String(timeinfo.tm_min) + ':';

         if (timeinfo.tm_sec < 10)
            lastUpload += '0';

         lastUpload += String(timeinfo.tm_sec);
      }
      else
      {
         String header = get_header();
         String Time1 = getDateTime(header);
         setEsp32Time(Time1.c_str());
      }

      renderPage(currentPage);
   }

   if (useBattery && gotoSleep)
   {
      esp_sleep_enable_timer_wakeup(seconds_to_next_hour() * uS_TO_S_FACTOR);
      Serial.println("Setup ESP32 to sleep for " + String(seconds_to_next_hour()) + " Seconds");
      Serial.flush();
      esp_deep_sleep_start();
   }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
   Serial.printf("\nListing directory: %s\r\n", dirname);

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

// Optional callback function, fired when NTP gets updated.
// Used to print the updated time or adjust an external RTC module.
void on_time_available(struct timeval *t)
{
   Serial.println("Received time adjustment from NTP");
   struct tm timeInfo;
   getLocalTime(&timeInfo, 1000);
   Serial.println(&timeInfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
   setTime(timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec, timeInfo.tm_mday, timeInfo.tm_mon + 1, timeInfo.tm_year + 1900);

   lastUpload = "";

   if (timeInfo.tm_hour < 10)
      lastUpload += '0';
   lastUpload += String(timeInfo.tm_hour) + ':';

   if (timeInfo.tm_min < 10)
      lastUpload += '0';
   lastUpload += String(timeInfo.tm_min) + ':';

   if (timeInfo.tm_sec < 10)
      lastUpload += '0';
   lastUpload += String(timeInfo.tm_sec);
}

void digitalClockDisplay(int x, int y, bool date)
{
   tft.setTextSize(1);
   tft.setCursor(x, y);
   tft.setTextColor(ST7735_WHITE);

   tft.fillRect(x - 5, y - 2, 60, 10, ST7735_BLACK);

   struct tm actualTime;
   getLocalTime(&actualTime);

   tft.print(actualTime.tm_hour);
   printDigits(actualTime.tm_min);
   printDigits(actualTime.tm_sec);

   if (date)
   {
      tft.print("   ");
      tft.print(actualTime.tm_mday);
      tft.print(" ");
      tft.print(actualTime.tm_mon + 1);
      tft.print(" ");
      tft.print(actualTime.tm_year - 100 + 2000);
      tft.println();
   }
}

void drawBattery(int x, int y)
{
   int bat = BL.getBatteryChargeLevel();
   tft.setCursor(x, y);
   tft.setTextColor(0xCED7);
   tft.print("Bat: ");
   if (bat < 1)
   {
      tft.setTextColor(ST7735_WHITE);
      tft.print("NO");
   }
   if (bat > 1 && bat < 20)
   {
      tft.setTextColor(ST7735_RED);
   }
   if (bat >= 20 && bat < 40)
   {
      tft.setTextColor(ST7735_ORANGE);
   }
   if (bat >= 40 && bat < 60)
   {
      tft.setTextColor(ST7735_YELLOW);
   }
   if (bat >= 60 && bat < 80)
   {
      tft.setTextColor(0xAEB2);
   }
   if (bat >= 80)
   {
      tft.setTextColor(ST7735_GREEN);
   }
   tft.print(bat);
   tft.print(" %");
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
   Serial.println();

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
            allSensors[i].measurements[j].loraSend = getLoraSendTypeFromString(measurementObj["loraSend"].as<String>());
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
      // Serial.println("mounted file system");
      if (SPIFFS.exists("/connectors.json"))
      {
         // file exists, reading and loading
         // Serial.println("reading config file");
         File connectorsFile = SPIFFS.open("/connectors.json", "r");
         if (connectorsFile)
         {
            // Serial.println("opened config file");
            size_t size = connectorsFile.size();
            // Allocate a buffer to store contents of the file.
            std::unique_ptr<char[]> buf(new char[size]);

            connectorsFile.readBytes(buf.get(), size);

            // DynamicJsonDocument json(CON_BUFFER);
            StaticJsonDocument<800> doc;
            auto deserializeError = deserializeJson(doc, buf.get());
            // serializeJson(doc, Serial);
            if (!deserializeError)
            {
               // Serial.println("\nparsed json");

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
   serializeJson(doc, connectorsFile);

   connectorsFile.close();
   // end save
}

void renderPage(int page)
{
   // Update the TFT display with content for the specified page
   if (page == 0)
   {
      mainPage();
   }
   if (page == 1)
   {
      I2C_ConnectorPage();
   }
   if (page == 2)
   {
      ADC_ConnectorPage();
   }
   if (page == 3)
   {
      OneWire_ConnectorPage();
   }
   if (page > NUM_PAGES - 1)
   {
      measurementsPage(page);
   }
}

int countMeasurements(const std::vector<Sensor> &sensors)
{
   int count = 0;
   for (const auto &sensor : sensors)
   {
      count += sensor.returnCount;
   }
   return count;
}

void mainPage()
{
   tft.fillScreen(ST7735_BLACK);
   tft.setTextColor(0x9E6F);
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
   tft.print("Board ID: ");
   tft.print(boardID);
   tft.setCursor(5, 55);
   tft.print(version);
   tft.setTextColor(0xCED7);
   tft.setCursor(5, 65);
   tft.print("WiFi: ");
   tft.setTextColor(ST7735_WHITE);
   tft.print(WiFi.SSID());
   tft.setCursor(5, 75);
   tft.setTextColor(0xCED7);
   tft.print("IP: ");
   tft.setTextColor(ST7735_WHITE);
   tft.print(WiFi.localIP());
   tft.setCursor(5, 85);
   tft.setTextColor(0xCED7);
   tft.print("MAC: ");
   tft.setTextColor(ST7735_WHITE);
   tft.print(WiFi.macAddress());

   digitalClockDisplay(5, 95, true);

   tft.setCursor(5, 105);
   tft.print("last data UPLOAD: ");
   tft.setTextColor(ST7735_ORANGE);
   tft.print(upload);

   drawBattery(83, 115);

   tft.setTextColor(ST7735_ORANGE);
   tft.setCursor(5, 115);
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

void printSensors()
{
   for (int i = 0; i < sensorVector.size(); ++i)
   {
      Serial.print("Sensor ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(sensorVector[i].sensor_name);
   }
}

void I2C_ConnectorPage()
{
   tft.fillScreen(ST7735_BLACK);
   tft.setTextSize(2);
   tft.setCursor(10, 10);
   tft.setTextColor(ST7735_WHITE);
   tft.print("I2C Con");

   tft.setTextSize(1);
   int cursor_y = 35;

   for (int i = 0; i < I2C_NUM; i++)
   {
      tft.setCursor(5, cursor_y);
      tft.setTextColor(ST7735_YELLOW);

      tft.print("I2C_");
      tft.print(i + 1);
      tft.setCursor(80, cursor_y);
      if (I2C_con_table[i] == -1)
      {
         tft.setTextColor(ST7735_RED);
         tft.print("NO");
      }
      else
      {
         tft.setTextColor(ST7735_GREEN);
         tft.print(allSensors[I2C_con_table[i]].sensor_name);
      }
      cursor_y += 10;
   }
   cursor_y += 10;
   tft.setTextSize(2);
   tft.setCursor(10, cursor_y);
   tft.setTextColor(ST7735_WHITE);
   tft.print("I2C_5V Con");

   cursor_y += 25;

   tft.setTextSize(1);
   tft.setCursor(5, cursor_y);
   tft.setTextColor(ST7735_YELLOW);

   tft.print("I2C_5V");

   tft.setCursor(80, cursor_y);
   if (I2C_5V_con_table[0] == -1)
   {
      tft.setTextColor(ST7735_RED);
      tft.print("NO");
   }
   else
   {
      tft.setTextColor(ST7735_GREEN);
      tft.print(allSensors[I2C_5V_con_table[0]].sensor_name);
   }
}

void ADC_ConnectorPage()
{
   tft.fillScreen(ST7735_BLACK);
   tft.fillScreen(ST7735_BLACK);
   tft.setTextSize(2);
   tft.setCursor(10, 10);
   tft.setTextColor(ST7735_WHITE);
   tft.print("ADC Con");

   tft.setTextSize(1);
   int cursor_y = 45;

   for (int i = 0; i < ADC_NUM; i++)
   {
      tft.setCursor(5, cursor_y);
      tft.setTextColor(ST7735_YELLOW);

      tft.print("ADC_");
      tft.print(i + 1);
      tft.setCursor(80, cursor_y);
      if (ADC_con_table[i] == -1)
      {
         tft.setTextColor(ST7735_RED);
         tft.print("NO");
      }
      else
      {
         tft.setTextColor(ST7735_GREEN);
         tft.print(allSensors[ADC_con_table[i]].sensor_name);
      }
      cursor_y += 10;
   }
}

void OneWire_ConnectorPage()
{
   tft.fillScreen(ST7735_BLACK);

   tft.fillScreen(ST7735_BLACK);
   tft.setTextSize(2);
   tft.setCursor(10, 10);
   tft.setTextColor(ST7735_WHITE);
   tft.print("OneWire Con");

   tft.setTextSize(1);
   int cursor_y = 45;

   for (int i = 0; i < ONEWIRE_NUM; i++)
   {
      tft.setCursor(5, cursor_y);
      tft.setTextColor(ST7735_YELLOW);

      tft.print("1-Wire_");
      tft.print(i + 1);
      tft.setCursor(80, cursor_y);
      if (OneWire_con_table[i] == -1)
      {
         tft.setTextColor(ST7735_RED);
         tft.print("NO");
      }
      else
      {
         tft.setTextColor(ST7735_GREEN);
         tft.print(allSensors[OneWire_con_table[i]].sensor_name);
      }
      cursor_y += 10;
   }
}

void measurementsPage(int page)
{
   tft.fillScreen(ST7735_BLACK);
   tft.setTextSize(2);
   tft.setCursor(10, 10);
   tft.setTextColor(ST7735_WHITE);
   tft.print("Sensor Data");

   tft.setTextSize(1);

   int cursor_y = 35;

   int startIndex = (page - NUM_PAGES) * NUM_PERPAGE;
   int endIndex = startIndex + NUM_PERPAGE;
   if (endIndex > show_measurements.size())
   {
      endIndex = show_measurements.size();
   }

   for (int i = startIndex; i < endIndex; i++)
   {
      tft.setCursor(5, cursor_y);
      tft.setTextColor(ST7735_BLUE);
      tft.print(show_measurements[i].data_name);
      tft.print(": ");

      tft.setCursor(60, cursor_y);
      tft.setTextColor(ST7735_YELLOW);

      if (!isnan(show_measurements[i].value))
      {
         tft.print(show_measurements[i].value);
      }
      else
      {
         tft.print("NAN");
      }

      tft.print(" ");

      if (!(show_measurements[i].unit == "°C"))
      {
         tft.print(show_measurements[i].unit);
      }
      else
      {
         tft.drawChar(tft.getCursorX(), tft.getCursorY(), 0xF8, ST7735_YELLOW, ST7735_BLACK, 1);
         tft.setCursor(tft.getCursorX() + 7, tft.getCursorY());
         tft.print("C");
      }
      cursor_y += 10;
   }
}

void checkLoadedStuff(void)
{
   Serial.println();
   Serial.println("---------------Prototype Sensors loaded -----------");
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
   Serial.println("---------------Config loaded -----------");

   Serial.print("\nBoard ID: ");
   Serial.println(boardID);
   Serial.print("Battery powered: ");
   Serial.println(useBattery);
   Serial.print("Display: ");
   Serial.println(useDisplay);
   Serial.print("use Enterprise WPA: ");
   Serial.println(useEnterpriseWPA);
   Serial.print("use Custom NTP: ");
   Serial.println(useCustomNTP);
   Serial.print("use NTP: ");
   Serial.println(useNTP);
   Serial.print("API Key: ");
   Serial.println(API_KEY);
   Serial.print("Timezone: ");
   Serial.println(timeZone);
   Serial.print("Upload: ");
   Serial.println(upload);
   Serial.print("Anonym ID: ");
   Serial.println(anonym);
   Serial.print("\nUser CA: ");
   Serial.println(user_CA);
   Serial.print("Custom NTP Address: ");
   Serial.println(customNTPaddress);
   Serial.print("Lora Frequency: ");
   Serial.println(lora_fqz);
   Serial.print("OTAA DEVEUI: ");
   Serial.println(OTAA_DEVEUI);
   Serial.print("OTAA APPEUI: ");
   Serial.println(OTAA_APPEUI);
   Serial.print("OTAA APPKEY: ");
   Serial.println(OTAA_APPKEY);

   Serial.println("\n---------------DEBUGG END -----------");

   Serial.println();
}

void save_Config(void)
{
   StaticJsonDocument<2500> doc;

   doc["BoardID"] = boardID;
   doc["useBattery"] = useBattery;
   doc["useDisplay"] = useDisplay;
   doc["useEnterpriseWPA"] = useEnterpriseWPA;
   doc["useCustomNTP"] = useCustomNTP;
   doc["useNTP"] = useNTP;
   doc["API_KEY"] = API_KEY;
   doc["upload"] = upload;
   doc["anonym"] = anonym;
   doc["user_CA"] = user_CA;
   doc["customNTPadress"] = customNTPaddress;
   doc["timeZone"] = timeZone;
   doc["lora_fqz"] = lora_fqz;
   doc["OTAA_DEVEUI"] = OTAA_DEVEUI;
   doc["OTAA_APPEUI"] = OTAA_APPEUI;
   doc["OTAA_APPKEY"] = OTAA_APPKEY;

   File configFile = SPIFFS.open("/board_config.json", "w");
   if (!configFile)
   {
      Serial.println("failed to open config file for writing");
   }

   serializeJson(doc, configFile);

   configFile.close();
   // end save
}

void load_Config(void)
{
   if (SPIFFS.begin())
   {
      if (SPIFFS.exists("/board_config.json"))
      {
         // file exists, reading and loading
         // Serial.println("reading config file");
         File configFile = SPIFFS.open("/board_config.json", "r");
         if (configFile)
         {
            // Serial.println("opened config file");
            size_t size = configFile.size();
            // Allocate a buffer to store contents of the file.
            std::unique_ptr<char[]> buf(new char[size]);

            configFile.readBytes(buf.get(), size);

            StaticJsonDocument<2500> doc;
            auto deserializeError = deserializeJson(doc, buf.get());

            if (!deserializeError)
            {
               boardID = doc["BoardID"];
               useBattery = doc["useBattery"];
               useDisplay = doc["useDisplay"];
               useEnterpriseWPA = doc["useEnzerpriseWPA"];
               useCustomNTP = doc["useCustomNTP"];
               useNTP = doc["useNTP"];
               API_KEY = doc["API_KEY"].as<String>();
               upload = doc["upload"].as<String>();
               anonym = doc["anonym"].as<String>();
               user_CA = doc["user_CA"].as<String>();
               customNTPaddress = doc["customNTPadress"].as<String>();
               timeZone = doc["timeZone"].as<String>();
               lora_fqz = doc["lora_fqz"].as<String>();
               OTAA_DEVEUI = doc["OTAA_DEVEUI"].as<String>();
               OTAA_APPEUI = doc["OTAA_APPEUI"].as<String>();
               OTAA_APPKEY = doc["OTAA_APPKEY"].as<String>();
            }
         }
         else
         {
            Serial.println("failed to load json config");
         }
         configFile.close();
      }
   }
   else
   {
      Serial.println("failed to mount FS");
   }
}

void printMeassurments()
{
   // Loop through all the elements in the vector
   for (int i = 0; i < sensorVector.size(); ++i)
   {
      for (int j = 0; j < sensorVector[i].returnCount; j++)
      {
         Serial.print(sensorVector[i].measurements[j].data_name);
         Serial.print(":  ");
         Serial.println(sensorVector[i].measurements[j].value);
      }
   }
   Serial.print("\nVector elements: ");
   Serial.print(sensorVector.size() + 1);
   Serial.print("\nSize of Vector: ");
   Serial.println(sizeof(sensorVector));
}

void wifi_sendData(void)
{
   // ----- code later used to post measurements

   DynamicJsonDocument docMeasures(2000);

   for (int i = 0; i < sensorVector.size(); ++i)
   {
      for (int j = 0; j < sensorVector[i].returnCount; j++)
      {
         docMeasures[sensorVector[i].measurements[j].data_name] = sensorVector[i].measurements[j].value;
      }
   }

   String output;
   serializeJson(docMeasures, output);

   // Serial.println("\nsend Data:");
   // serializeJson(docMeasures, Serial);

   // --------------------------   test all hard coded ....

   // Check WiFi connection status
   if (WiFi.status() == WL_CONNECTED)
   {
      // WiFiClient client;
      WiFiClientSecure client;
      client.setCACert(kits_ca);

      HTTPClient https;

      //    StaticJsonDocument<100> doc;

      //    doc["test"] = 16;

      //    String output;

      //    serializeJson(doc, output);

      //    Serial.print("\njson Data for POST: ");
      //    serializeJson(doc, Serial);

      //    Serial.println();

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

      // if (httpResponseCode > 0)
      // {
      //    String payload = https.getString();
      //    Serial.println(payload);
      // }

      // Free resources
      https.end();
   }
   else
   {
      Serial.println("WiFi Disconnected");
   }

   DynamicJsonDocument deallocate(docMeasures);

   Serial.println();
}

void lora_sendData(void)
{
   // https://github.com/thesolarnomad/lora-serialization

   byte message[190]; // max. 11 Sensor with id Uint8_t (1 byte) + (MultigasV1 8*float) + (10 Sensors with 3*float) 11 + 1*8*4  + 10*3*4 = 163
   LoraEncoder encoder(message);

   for (int i = 0; i < sensorVector.size(); ++i)
   {
      int k = static_cast<uint8_t>(sensorVector[i].sensor_id); // send Sensor ID as UINT8
      encoder.writeUint8(k);

      for (int j = 0; j < sensorVector[i].returnCount; j++)
      {
         switch (sensorVector[i].measurements[j].loraSend) // send Measurment values as different packeges
         {
         case UNIXTIME:
            encoder.writeUnixtime(static_cast<uint32_t>(round(sensorVector[i].measurements[j].value)));
            break;
         // case LATLNG:
         //    encoder.writeLatLng(sensorVector[i].measurements[j].value.latitude, sensorVector[i].measurements[j].value.longitude);
         //    break;
         case UINT8:
            encoder.writeUint8(static_cast<uint8_t>(round(sensorVector[i].measurements[j].value)));
            break;
         case UINT16:
            encoder.writeUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
            break;
         case UINT32:
            encoder.writeUint32(static_cast<uint32_t>(round(sensorVector[i].measurements[j].value)));
            break;
         case TEMP:
            encoder.writeTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
            break;
         case HUMI:
            encoder.writeHumidity(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
            break;
         case RAWFLOAT:
            encoder.writeRawFloat(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
            break;
            // case BITMAP:
            //    encoder.writeBitmap(sensorVector[i].measurements[j].value, false, false, false, false, false, false, false);
            //    break;
         }
      }
   }

   // TODO: lora send message
}

void get_time_in_timezone(const char *timezone)
{
   struct timeval tv;
   gettimeofday(&tv, NULL);
   time_t now = tv.tv_sec;
   struct tm *local = localtime(&now);
   setenv("TZ", timezone, 1);
   tzset();
   struct tm *gmt = gmtime(&now);
   int diff_hours = (local->tm_hour - gmt->tm_hour);
   int diff_minutes = (local->tm_min - gmt->tm_min);
   Serial.printf("Timezone: %s\n", timezone);
   Serial.printf("Current time: %02d:%02d\n", local->tm_hour, local->tm_min);
   Serial.printf("GMT time: %02d:%02d\n", gmt->tm_hour, gmt->tm_min);
   Serial.printf("Difference from GMT: %d:%02d\n", diff_hours, diff_minutes);
}

int seconds_to_next_hour()
{
   struct tm timeinfo;
   getLocalTime(&timeinfo);

   // time_t now = time(NULL);
   // struct tm *tm_now = localtime(&now);
   int seconds = (60 - timeinfo.tm_sec) + (59 - timeinfo.tm_min) * 60;
   return seconds;
}

void startBlinking()
{
   // Store initial state of LED
   initialState = digitalRead(LED);

   // Start Timer
   blinker.attach(1.0, toggleLED);
}

void stopBlinking()
{
   // Stop Timer
   blinker.detach();

   // Restore initial state of LED
   digitalWrite(LED, initialState);
}

void toggleLED()
{
   digitalWrite(LED, ledState);
   ledState = !ledState;
}

void configModeCallback(WiFiManager *myWiFiManager)
{
   // Serial.println("Entered Conf Mode");
   // Serial.print("Config SSID: ");
   // Serial.println(myWiFiManager->getConfigPortalSSID());
   // Serial.print("Config IP Address: ");
   // Serial.println(WiFi.softAPIP());

   tft.fillScreen(ST7735_BLACK);
   tft.setTextColor(ST7735_WHITE);
   tft.setFont(&FreeSans9pt7b);
   tft.setTextSize(1);
   tft.setCursor(5, 17);
   tft.print("TeleAgriCulture");
   tft.setCursor(5, 38);
   tft.print("Board V2.1");

   tft.setTextColor(ST7735_RED);
   tft.setFont();
   tft.setTextSize(2);
   tft.setCursor(5, 50);
   tft.print("Config MODE");
   tft.setTextColor(ST7735_WHITE);
   tft.setTextSize(1);
   tft.setCursor(5, 73);
   tft.print("SSID:");
   tft.setCursor(5, 85);
   tft.print(myWiFiManager->getConfigPortalSSID());
   tft.setCursor(5, 95);
   tft.print("IP: ");
   tft.print(WiFi.softAPIP());
   tft.setCursor(5, 108);
   tft.print("MAC: ");
   tft.print(WiFi.macAddress());
   tft.setCursor(5, 117);
   tft.setTextColor(ST7735_BLUE);
   tft.print(version);
}

LoraSendType getLoraSendTypeFromString(String str)
{
   if (str == "UNIXTIME")
   {
      return UNIXTIME;
   }
   else if (str == "LATLNG")
   {
      return LATLNG;
   }
   else if (str == "UINT8")
   {
      return UINT8;
   }
   else if (str == "UINT16")
   {
      return UINT16;
   }
   else if (str == "UINT32")
   {
      return UINT32;
   }
   else if (str == "TEMP")
   {
      return TEMP;
   }
   else if (str == "HUMI")
   {
      return HUMI;
   }
   else if (str == "RAWFLOAT")
   {
      return RAWFLOAT;
   }
   else if (str == "BITMAP")
   {
      return BITMAP;
   }
   else
   {
      return NOT;
   }
}

String get_header()
{
   WiFiClientSecure client;

   // makes a HTTP request
   unsigned long timeNow;
   bool HeaderComplete = false;
   bool currentLineIsBlank = true;
   String header = "";
   client.stop(); // Close any connection before send a new request.  This will free the socket on the WiFi
   if (client.connect(GET_Time_SERVER, SSL_PORT))
   { // if there's a successful connection:
      client.println("GET " + GET_Time_Address + " HTTP/1.1");
      client.print("HOST: ");
      client.println(GET_Time_SERVER);
      client.println();
      timeNow = millis();
      while (millis() - timeNow < TIMEOUT)
      {
         while (client.available())
         {
            char c = client.read();
            if (!HeaderComplete)
            {
               if (currentLineIsBlank && c == '\n')
               {
                  HeaderComplete = true;
               }
               else
               {
                  header = header + c;
               }
            }
            if (c == '\n')
            {
               currentLineIsBlank = true;
            }
            else if (c != '\r')
            {
               currentLineIsBlank = false;
            }
         }
         if (HeaderComplete)
            break;
      }
   }
   else
   {
      Serial.println("Connection failed");
   }
   if (client.connected())
   {
      client.stop();
   }
   return header;
}

time_t convertDateTime(String dateTimeStr)
{
   struct tm tm;
   memset(&tm, 0, sizeof(struct tm));
   strptime(dateTimeStr.c_str(), "%a, %d %b %Y %H:%M:%S %Z", &tm);
   time_t t = mktime(&tm);
   return t;
}

String getDateTime(String header)
{
   int index = header.indexOf("Date:");
   if (index == -1)
   {
      return "";
   }
   String dateTimeStr = header.substring(index + 6, index + 31);
   time_t t = convertDateTime(dateTimeStr);
   char buf[20];
   strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M", localtime(&t));
   return String(buf);
}

void setEsp32Time(const char *timeStr)
{
   // TODO: fix 1h off

   struct tm t;
   struct tm now;
   strptime(timeStr, "%Y-%m-%dT%H:%M", &t);
   now.tm_hour = t.tm_hour;
   now.tm_min = t.tm_min;
   now.tm_sec = 0;
   now.tm_mday = t.tm_mday;
   now.tm_mon = t.tm_mon;
   now.tm_year = t.tm_year;

   delay(100);

   const time_t sec = mktime(&now); // make time_t
   Serial.printf("Setting time: %s", asctime(&now));
   // Serial.print("\nsec: ");
   // Serial.println(sec);

   struct timeval set_Time = {.tv_sec = sec};
   settimeofday(&set_Time, NULL);

   // setTime(sec);
   // setTime(now.tm_hour, now.tm_min, now.tm_sec, now.tm_mday, now.tm_mon, now.tm_year);
   setenv("TZ", timeZone.c_str(), 1);
   tzset();

   struct tm timeinfo;
   getLocalTime(&timeinfo);

   Serial.println("\nESP Time set via HTTP get:");
   Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");

   lastUpload = "";

   if (timeinfo.tm_hour < 10)
      lastUpload += '0';
   lastUpload += String(timeinfo.tm_hour) + ':';

   if (timeinfo.tm_min < 10)
      lastUpload += '0';
   lastUpload += String(timeinfo.tm_min) + ':';

   if (timeinfo.tm_sec < 10)
      lastUpload += '0';

   lastUpload += String(timeinfo.tm_sec);
}

void printHex2(unsigned v)
{
   v &= 0xff;
   if (v < 16)
      Serial.print('0');
   Serial.print(v, HEX);
}

void onEvent(ev_t ev)
{
   Serial.print(os_getTime());
   Serial.print(": ");
   switch (ev)
   {
   case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
   case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
   case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
   case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
   case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
   case EV_JOINED:
      Serial.println(F("EV_JOINED"));
      {
         u4_t netid = 0;
         devaddr_t devaddr = 0;
         u1_t nwkKey[16];
         u1_t artKey[16];
         LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
         Serial.print("netid: ");
         Serial.println(netid, DEC);
         Serial.print("devaddr: ");
         Serial.println(devaddr, HEX);
         Serial.print("AppSKey: ");
         for (size_t i = 0; i < sizeof(artKey); ++i)
         {
            if (i != 0)
               Serial.print("-");
            printHex2(artKey[i]);
         }
         Serial.println("");
         Serial.print("NwkSKey: ");
         for (size_t i = 0; i < sizeof(nwkKey); ++i)
         {
            if (i != 0)
               Serial.print("-");
            printHex2(nwkKey[i]);
         }
         Serial.println();
      }
      // Disable link check validation (automatically enabled
      // during join, but because slow data rates change max TX
      // size, we don't use it in this example.
      LMIC_setLinkCheckMode(0);
      break;
   /*
   || This event is defined but not used in the code. No
   || point in wasting codespace on it.
   ||
   || case EV_RFU1:
   ||     Serial.println(F("EV_RFU1"));
   ||     break;
   */
   case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      break;
   case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
   case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
         Serial.println(F("Received ack"));
      if (LMIC.dataLen)
      {
         Serial.print(F("Received "));
         Serial.print(LMIC.dataLen);
         Serial.println(F(" bytes of payload"));
      }
      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;
   case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
   case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
   case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
   case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
   case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
   /*
   || This event is defined but not used in the code. No
   || point in wasting codespace on it.
   ||
   || case EV_SCAN_FOUND:
   ||    Serial.println(F("EV_SCAN_FOUND"));
   ||    break;
   */
   case EV_TXSTART:
      Serial.println(F("EV_TXSTART"));
      break;
   case EV_TXCANCELED:
      Serial.println(F("EV_TXCANCELED"));
      break;
   case EV_RXSTART:
      /* do not print anything -- it wrecks timing */
      break;
   case EV_JOIN_TXCOMPLETE:
      Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
      break;

   default:
      Serial.print(F("Unknown event: "));
      Serial.println((unsigned)ev);
      break;
   }
}

void do_send(osjob_t *j)
{
   // Check if there is not a current TX/RX job running
   if (LMIC.opmode & OP_TXRXPEND)
   {
      Serial.println(F("OP_TXRXPEND, not sending"));
   }
   else
   {
      // Prepare upstream data transmission at the next possible time.
      LMIC_setTxData2(1, mydata, sizeof(mydata) - 1, 0);
      Serial.println(F("Packet queued"));
   }
   // Next TX is scheduled after TX_COMPLETE event.
}