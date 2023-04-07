/*\
 *
 * TeleAgriCulture Board Definitions
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
\*/

#include <Arduino.h>
#include <WString.h>

#define SENSORS_NUM 14      // Number of Sensors implemeted
#define MEASURMENT_NUM 8    // max. Sensor values / Sensor (Multi Gas Sensor V1 produces 8 measures to send)
#define MAX_I2C_ADDRESSES 3 // max. stored I2C addresses / Sensor
#define I2C_NUM 4
#define ADC_NUM 3
#define ONEWIRE_NUM 3
#define SPI_NUM 1
#define I2C_5V_NUM 1
#define EXTRA_NUM 2
#define JSON_BUFFER 4500 // json buffer for prototype Sensors class ( const char *sensors = R"([.....])" )

// ----- Declare Connectors ----- //

int I2C_con_table[I2C_NUM];
int ADC_con_table[ADC_NUM];
int OneWire_con_table[ONEWIRE_NUM];
int SPI_con_table[SPI_NUM];
int I2C_5V_con_table[I2C_5V_NUM];
int EXTRA_con_table[EXTRA_NUM];

// ----- Declare Connectors ----- //

// ***  initial values  ***  // will be overwritten by config file / user input at board setup

int boardID = 1003;
String API_KEY = "8i8nRED12XgHb3vBjIXCf0rXMedI8NTB"; // TODO: after all the testing has to be masked in the gitlab and changed in the app

String version = "Firmware Version 0.92";

bool useBattery = false;
bool useDisplay = true;
bool useEnterpriseWPA = false;
bool useNTP = false;
bool useCustomNTP = false;

String upload = "WIFI";
String anonym = "anonymus@example.com";
String user_CA = "-----BEGIN CERTIFICATE----- optional -----END CERTIFICATE-----";
String lora_fqz = "EU";
String OTAA_DEVEUI = "70B3D57ED005A8F4";
String OTAA_APPEUI = "70B3D57ED005A8F4";
String OTAA_APPKEY = "DF6B2A4AC0930BCA55141564D751D578";
String customNTPaddress = "129.6.15.28";
String setTime_value = "";
String timeZone = "";

// ***  initial values  ***

String hostname = "TeleAgriCulture Board";
String serverName = "https://kits.teleagriculture.org/api/kits/" + String(boardID) + "/measurements";
String api_Bearer = "Bearer " + API_KEY;
const char GET_Time_SERVER[30] = "www.teleagriculture.org";
String GET_Time_Address = "https://www.teleagriculture.org";
const int SSL_PORT =443;
const unsigned long TIMEOUT = 2500;

// TODO: have to change the POST to insecure ... no checking the Server CA

const char *kits_ca =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDzTCCArWgAwIBAgIQCjeHZF5ftIwiTv0b7RQMPDANBgkqhkiG9w0BAQsFADBa\n"
    "MQswCQYDVQQGEwJJRTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJl\n"
    "clRydXN0MSIwIAYDVQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTIw\n"
    "MDEyNzEyNDgwOFoXDTI0MTIzMTIzNTk1OVowSjELMAkGA1UEBhMCVVMxGTAXBgNV\n"
    "BAoTEENsb3VkZmxhcmUsIEluYy4xIDAeBgNVBAMTF0Nsb3VkZmxhcmUgSW5jIEVD\n"
    "QyBDQS0zMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEua1NZpkUC0bsH4HRKlAe\n"
    "nQMVLzQSfS2WuIg4m4Vfj7+7Te9hRsTJc9QkT+DuHM5ss1FxL2ruTAUJd9NyYqSb\n"
    "16OCAWgwggFkMB0GA1UdDgQWBBSlzjfq67B1DpRniLRF+tkkEIeWHzAfBgNVHSME\n"
    "GDAWgBTlnVkwgkdYzKz6CFQ2hns6tQRN8DAOBgNVHQ8BAf8EBAMCAYYwHQYDVR0l\n"
    "BBYwFAYIKwYBBQUHAwEGCCsGAQUFBwMCMBIGA1UdEwEB/wQIMAYBAf8CAQAwNAYI\n"
    "KwYBBQUHAQEEKDAmMCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2VydC5j\n"
    "b20wOgYDVR0fBDMwMTAvoC2gK4YpaHR0cDovL2NybDMuZGlnaWNlcnQuY29tL09t\n"
    "bmlyb290MjAyNS5jcmwwbQYDVR0gBGYwZDA3BglghkgBhv1sAQEwKjAoBggrBgEF\n"
    "BQcCARYcaHR0cHM6Ly93d3cuZGlnaWNlcnQuY29tL0NQUzALBglghkgBhv1sAQIw\n"
    "CAYGZ4EMAQIBMAgGBmeBDAECAjAIBgZngQwBAgMwDQYJKoZIhvcNAQELBQADggEB\n"
    "AAUkHd0bsCrrmNaF4zlNXmtXnYJX/OvoMaJXkGUFvhZEOFp3ArnPEELG4ZKk40Un\n"
    "+ABHLGioVplTVI+tnkDB0A+21w0LOEhsUCxJkAZbZB2LzEgwLt4I4ptJIsCSDBFe\n"
    "lpKU1fwg3FZs5ZKTv3ocwDfjhUkV+ivhdDkYD7fa86JXWGBPzI6UAPxGezQxPk1H\n"
    "goE6y/SJXQ7vTQ1unBuCJN0yJV0ReFEQPaA1IwQvZW+cwdFD19Ae8zFnWSfda9J1\n"
    "CZMRJCQUzym+5iPDuI9yP+kHyCREU3qzuWFloUwOxkgAyXVjBYdwRVKD05WdRerw\n"
    "6DEdfgkfCv4+3ao8XnTSrLE=\n"
    "-----END CERTIFICATE-----\n";

// ----- Define Pins ----- //
#define I2C_SDA 8 // on teleAgriCulture Board V2.0 I2C_5V SDA is GPIO 15
#define I2C_SCL 9 // on teleAgriCulture Board V2.0 I2C_5V SCL is GPIO 16

#define TFT_SCLK 36
#define TFT_MISO 37
#define TFT_MOSI 35
#define SPI_CON_CS 38
#define TFT_CS 15 // on teleAgriCulture Board V2.0 it is I2C_5V SDA pin  CHANGE THIS TO 38(SPI CON CS)
#define TFT_RST 1 // on teleAgriCulture Board V2.0 it is part of J3 CON
#define TFT_DC 2  // on teleAgriCulture Board V2.0 it is part of J3 CON
#define TFT_BL 48 // on teleAgriCulture Board V2.0 it is part of J4 CON

#define LORA_CS 10
#define MOSI 11
#define SCLK 12
#define MISO 13
#define LORA_RST 17
#define LORA_DI0 18
#define LORA_DI1 14 // on teleAgriCulture Board V2.0 it has to be briged to the LORA Module Connector!

#define ONEWIRE_1 39
#define ONEWIRE_2 40
#define ONEWIRE_3 41

#define ANALOG1 5 // on teleAgriCulture Board V2.0 it is GPIO 4
#define ANALOG2 6 // on teleAgriCulture Board V2.0 it is GPIO 5
#define ANALOG3 7 // on teleAgriCulture Board V2.0 it is GPIO 6
#define BATSENS 4 // on teleAgriCulture Board V2.0 it is GPIO 7

#define SW_3V3 42
#define SW_5V 47

#define LEFT_BUTTON_PIN 0
#define RIGHT_BUTTON_PIN 16 // on teleAgriCulture Board V2.0 it is I2C_5V SCL pin

#define LED 21

// ----- Define Pins ----- //

// ---- Classes and Enum ---- //

enum class ConnectorType : uint8_t
{
  I2C,
  ONE_WIRE,
  ADC,
  I2C_5V,
  SPI_CON,
  EXTRA
};

enum LoraSendType
{
  NOT = -1,
  UNIXTIME,
  LATLNG,
  UINT8,
  UINT16,
  UINT32,
  TEMP,
  HUMI,
  RAWFLOAT,
  BITMAP
};

enum SensorsImplemented
{
  NO = -1,
  BM280,
  LEVEL,
  VEML7700,
  TDS,
  CAP_SOIL,
  CAP_GROOVE,
  DHT_22,
  DS18B20,
  MULTIGAS,
  MULTIGAS_V1,
  DS3231,
  BATTERY,
  WS2812,
  SERVO
};

class Measurement
{
public:
  double value;
  int loraSend;
  String unit;
  String data_name;
};

class Sensor
{
public:
  SensorsImplemented sensor_id;
  String sensor_name;
  String con_typ;
  int returnCount;
  Measurement measurements[8];
  String i2c_add;
  String possible_i2c_add[2];

public:
  Sensor() : sensor_id(), sensor_name(), con_typ(), returnCount(0), i2c_add()
  {
    for (int i = 0; i < 8; ++i)
    {
      measurements[i] = Measurement();
    }
    for (int i = 0; i < 2; ++i)
    {
      possible_i2c_add[i] = "";
    }
  }

  Sensor(const Sensor &s) : sensor_id(s.sensor_id), sensor_name(s.sensor_name), con_typ(s.con_typ), returnCount(s.returnCount), i2c_add(s.i2c_add)
  {
    for (int i = 0; i < 8; ++i)
    {
      measurements[i] = s.measurements[i];
    }
    for (int i = 0; i < 2; ++i)
    {
      possible_i2c_add[i] = s.possible_i2c_add[i];
    }
  }
};

// ---- Classes and Enum ---- //

// ---- PROTOTYPE JSON for implemented sensors (gets loaded at start)

const char *proto_sensors = R"([
  {
    "sensor-id": 1,
    "name": "BM280",
    "con_typ": "I2C",
    "returnCount": 3,
    "measurements": [
      {
        "value": 56,
        "loraSend": "HUMI",
        "unit": "%",
        "data_name": "hum"
      },
      {
        "value": 20.3,
        "loraSend": "TEMP",
        "unit": "°C",
        "data_name": "temp"
      },
      {
        "value": 1000.5,
        "loraSend": "RAWFLOAT",
        "unit": "hPa",
        "data_name": "press"
      }
    ],
    "i2c_add": "0x76",
    "possible_i2c_add": [
      {
        "standard": "0x76"
      },
      {
        "alt_1": "0x77"
      }
    ]
  },
  {
    "sensor-id": 2,
    "name": "Level",
    "con_typ": "I2C",
    "returnCount": 1,
    "measurements": [
      {
        "value": 14.5,
        "loraSend": "TEMP",
        "unit": "mm",
        "data_name": "height"
      }
    ],
    "i2c_add": "0x78",
    "possible_i2c_add": [
      {
        "standard": "0x78"
      },
      {
        "alt_1": "0x77"
      }
    ]
  },
  {
    "sensor-id": 3,
    "name": "VELM7700",
    "con_typ": "I2C",
    "returnCount": 2,
    "measurements": [
      {
        "value": 11255,
        "loraSend": "UINT16",
        "unit": "lux",
        "data_name": "lux"
      },
      {
        "value": 55200,
        "loraSend": "UINT16",
        "unit": ".",
        "data_name": "ambient"
      }
    ],
    "i2c_add": "0x10",
    "possible_i2c_add": [
      {
        "standard": "0x10"
      }
    ]
  },
  {
    "sensor-id": 4,
    "name": "TDS",
    "con_typ": "ADC",
    "returnCount": 1,
    "measurements": [
      {
        "value": 300,
        "loraSend": "UINT16",
        "unit": "ppm",
        "data_name": "TDS"
      }
    ]
  },
  {
    "sensor-id": 5,
    "name": "CAP Soil",
    "con_typ": "ADC",
    "returnCount": 1,
    "measurements": [
      {
        "value": 300,
        "loraSend": "UINT16",
        "unit": ".",
        "data_name": "mois"
      }
    ]
  },
  {
    "sensor-id": 6,
    "name": "CAP Groove",
    "con_typ": "ADC",
    "returnCount": 1,
    "measurements": [
      {
        "value": 300,
        "loraSend": "UINT16",
        "unit": ".",
        "data_name": "mois"
      }
    ]
  },
  {
    "sensor-id": 7,
    "name": "DHT22",
    "con_typ": "ONE_WIRE",
    "returnCount": 2,
    "measurements": [
      {
        "value": 20.2,
        "loraSend": "TEMP",
        "unit": "°C",
        "data_name": "temp"
      },
      {
        "value": 55,
        "loraSend": "HUMI",
        "unit": "%",
        "data_name": "hum"
      }
    ]
  },
  {
    "sensor-id": 8,
    "name": "DS18B20",
    "con_typ": "ONE_WIRE",
    "returnCount": 1,
    "measurements": [
      {
        "value": 20.2,
        "loraSend": "TEMP",
        "unit": "°C",
        "data_name": "temp"
      }
    ]
  },
  {
    "sensor-id": 9,
    "name": "MultiGasV2",
    "con_typ": "I2C_5V",
    "returnCount": 2,
    "measurements": [
      {
        "value": 5,
        "loraSend": "UINT16",
        "unit": "ppm",
        "data_name": "CO"
      },
      {
        "value": 5,
        "loraSend": "UINT16",
        "unit": "ppm",
        "data_name": "NO2"
      }
    ],
    "i2c_add": "0x08",
    "possible_i2c_add": [
      {
        "standard": "0x08"
      },
      {
        "alt_1": "0x55"
      }
    ]
  },
  {
    "sensor-id": 10,
    "name": "MultiGasV1",
    "con_typ": "I2C_5V",
    "returnCount": 8,
    "measurements": [
      {
        "value": 5,
        "loraSend": "UINT16",
        "unit": "ppm",
        "data_name": "CO"
      },
      {
        "value": 5,
        "loraSend": "UINT16",
        "unit": "ppm",
        "data_name": "NO2"
      },
      {
        "value": 5,
        "loraSend": "UINT16",
        "unit": "ppm",
        "data_name": "NH3"
      },
      {
        "value": 5,
        "loraSend": "UINT16",
        "unit": "ppm",
        "data_name": "C3H8"
      },
      {
        "value": 5,
        "loraSend": "UINT16",
        "unit": "ppm",
        "data_name": "C4H10"
      },
      {
        "value": 5,
        "loraSend": "UINT16",
        "unit": "ppm",
        "data_name": "CH4"
      },
      {
        "value": 5,
        "loraSend": "UINT16",
        "unit": "ppm",
        "data_name": "H2"
      },
      {
        "value": 5,
        "loraSend": "UINT16",
        "unit": "ppm",
        "data_name": "C2H5OH"
      }
    ],
    "i2c_add": "0x04",
    "possible_i2c_add": [
      {
        "standard": "0x04"
      },
      {
        "alt_1": "0x19"
      }
    ]
  },
  {
    "sensor-id": 11,
    "name": "RTC DS3231",
    "con_typ": "I2C",
    "returnCount": 1,
    "measurements": [
      {
        "value": 20,
        "loraSend": "TEMP",
        "unit": "°C",
        "data_name": "Temp"
      }
    ],
    "i2c_add": "0x68",
    "possible_i2c_add": [
      {
        "standard": "0x04"
      },
      {
        "alt_1": "0x08"
      }
    ]
  },
  {
    "sensor-id": 12,
    "name": "Battery",
    "con_typ": "EXTRA",
    "returnCount": 1,
    "measurements": [
      {
        "value": 3.7,
        "loraSend": "RAWFLOAT",
        "unit": "V",
        "data_name": "Volt"
      }
    ]
  },
  {
    "sensor-id": 13,
    "name": "WS2812",
    "con_typ": "EXTRA",
    "returnCount": 1,
    "measurements": [
      {
        "value": 1,
        "loraSend": "UINT8",
        "unit": "OK",
        "data_name": "LED"
      }
    ]
  },
  {
    "sensor-id": 14,
    "name": "Servo",
    "con_typ": "EXTRA",
    "returnCount": 1,
    "measurements": [
      {
        "value": 90,
        "loraSend": "UINT8",
        "unit": "°",
        "data_name": "angle"
      }
    ]
  }
])";
