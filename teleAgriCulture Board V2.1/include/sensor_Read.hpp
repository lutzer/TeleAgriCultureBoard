/*\
 *
 * TeleAgriCulture Board Sensor Read
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

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "Adafruit_VEML7700.h"
#include <Multichannel_Gas_GMXXX.h>
#include <MiCS6814-I2C.h>
#include <OneWire.h>
#include <DHT.h>
#include <DallasTemperature.h>

#define DHTTYPE DHT22

// ----- Sensor section ----- //

Sensor allSensors[SENSORS_NUM];
Measurement measurements[MEASURMENT_NUM];

// flag for saving Connector data
bool shouldSaveConfig = false;

// ----- Sensor section ----- //

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;
Adafruit_VEML7700 veml = Adafruit_VEML7700();

void addMeassurments(String name, float value);
bool parseI2CAddress(const String &addressString, uint8_t *addressValue);
void readI2C_Connectors();
void readADC_Connectors();
void readOneWire_Connectors();
void readI2C_5V_Connector();
void readSPI_Connector();
void readEXTRA_Connectors();

// LevelSensor
#define NO_TOUCH 0xFE
#define THRESHOLD 100
#define ATTINY1_HIGH_ADDR 0x78
#define ATTINY2_LOW_ADDR 0x77
unsigned char low_data[8] = {0};
unsigned char high_data[12] = {0};
void getHigh12SectionValue(void);
void getLow8SectionValue(void);
// Levelsensor

// TDS Sensor
#define VREF 3.3  // analog reference voltage(Volt) of the ADC
#define SCOUNT 30 // sum of sample point
int getMedianNum(int bArray[], int iFilterLen);
// TDS Sensor

unsigned long previousMillis = 0;
const long interval = 4000;

void sensorRead()
{
    pinMode(SW_3V3, OUTPUT);
    pinMode(SW_5V, OUTPUT);

    digitalWrite(SW_3V3, HIGH);
    digitalWrite(SW_5V, HIGH);

    if (I2C_5V_con_table[0] == MULTIGAS_V1)
    {
        MiCS6814 multiGasV1;
        // Connect to sensor using default I2C address (0x04)
        // Alternatively the address can be passed to begin(addr)

        if (multiGasV1.begin() == true)
        {
            // Turn heater element on
            multiGasV1.powerOn();
            delay(20000); // heat up 20sek
        }
        else
        {
            // Print error message on failed connection
            Serial.println("MultiChannel Gas Sensor V1 not found on this address");
        }
    }

    pinMode(BATSENS, INPUT);
    // allSensors[BATTERY].measurements[0].value = analogRead(BATSENS);
    addMeassurments(allSensors[BATTERY].measurements[0].data_name, analogRead(BATSENS));

    Serial.println("SensorRead.....");
    Serial.println();

    readI2C_Connectors();
    readADC_Connectors();
    readOneWire_Connectors();
    // readI2C_5V_Connector();
    readEXTRA_Connectors();

    digitalWrite(SW_3V3, LOW);
    digitalWrite(SW_5V, LOW);
}

void readI2C_Connectors()
{
    TwoWire I2CCON = TwoWire(0);
    I2CCON.begin(I2C_SDA, I2C_SCL);

    for (int j = 0; j < I2C_NUM; j++)
    {
        switch (I2C_con_table[j])
        {
        case NO:
        {
            Serial.print("\nNo Sensor attaches at ");
            Serial.print("I2C_");
            Serial.println(j + 1);
        }
        break;

        case BM280:
        {
            //          https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/
            unsigned status;
            uint8_t addressValue;

            // Parse the I2C address string
            if (!parseI2CAddress((allSensors[j].i2c_add), &addressValue))
            {
                printf("Error: Invalid I2C address %s\n", (allSensors[j].i2c_add));
                break;
            }

            status = bme.begin(addressValue, &I2CCON);
            if (!status)
            {
                Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
                Serial.print("SensorID was: 0x");
                Serial.println(bme.sensorID(), 16);
                Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
                Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
                Serial.print("        ID of 0x60 represents a BME 280.\n");
                Serial.println("        ID of 0x61 represents a BME 680.\n");
                break;
            }
            // Serial.print(bme.readTemperature());
            // Serial.print(1.8 * bme.readTemperature() + 32);
            // allSensors[BM280].measurements[0].value = bme.readHumidity();
            // allSensors[BM280].measurements[1].value = bme.readTemperature();
            // allSensors[BM280].measurements[2].value = (bme.readPressure() / 100.0F);
            addMeassurments(allSensors[BM280].measurements[0].data_name, bme.readHumidity());
            addMeassurments(allSensors[BM280].measurements[1].data_name, bme.readTemperature());
            addMeassurments(allSensors[BM280].measurements[2].data_name, (bme.readPressure() / 100.0F));
        }
        break;

        case LEVEL:
        {
            //          https://github.com/SeeedDocument/Grove-Water-Level-Sensor/blob/master/water-level-sensor-demo.ino
            int sensorvalue_min = 250;
            int sensorvalue_max = 255;
            uint32_t touch_val = 0;
            uint8_t trig_section = 0;
            int low_count = 0;
            int high_count = 0;

            for (int i = 0; i < 8; i++)
            {

                if (low_data[i] >= sensorvalue_min && low_data[i] <= sensorvalue_max)
                {
                    low_count++;
                }
            }
            for (int i = 0; i < 12; i++)
            {
                if (high_data[i] >= sensorvalue_min && high_data[i] <= sensorvalue_max)
                {
                    high_count++;
                }
            }

            for (int i = 0; i < 8; i++)
            {
                if (low_data[i] > THRESHOLD)
                {
                    touch_val |= 1 << i;
                }
            }
            for (int i = 0; i < 12; i++)
            {
                if (high_data[i] > THRESHOLD)
                {
                    touch_val |= (uint32_t)1 << (8 + i);
                }
            }

            while (touch_val & 0x01)
            {
                trig_section++;
                touch_val >>= 1;
            }
            // allSensors[LEVEL].measurements[0].value = trig_section * 5;
            addMeassurments(allSensors[LEVEL].measurements[0].data_name, trig_section * 5);
        }
        break;

        case VEML7700:
        {
            //          See Vishy App Note "Designing the VEML7700 Into an Application"
            //          Vishay Document Number: 84323, Fig. 24 Flow Chart
            if (!veml.begin(&I2CCON))
            {
                Serial.println("Sensor VEML7700 not found");
                break;
            }
            // allSensors[VEML7700].measurements[0].value = veml.readLux(VEML_LUX_AUTO);
            addMeassurments(allSensors[VEML7700].measurements[0].data_name, veml.readLux(VEML_LUX_AUTO));
        }
        break;

        default:
            break;
        }
    }
}

void readADC_Connectors()
{
    for (int i = 0; i < ADC_NUM; i++)
    {
        switch (ADC_con_table[i])
        {
        case NO:
        {
            Serial.print("\nNo Sensor attaches at ");
            Serial.print("ADC_");
            Serial.println(i + 1);
        }
        break;

        case TDS:
        {
            //          www.cqrobot.wiki/index.php/TDS_(Total_Dissolved_Solids)_Meter_Sensor_SKU:_CQRSENTDS01
            int analogBuffer[SCOUNT]; // store the analog value in the array, read from ADC
            int analogBufferTemp[SCOUNT];
            int analogBufferIndex = 0, copyIndex = 0;
            float averageVoltage = 0, tdsValue = 0, temperature = 25;

            int tdsSensorPin;

            if (i == 0)
            {
                tdsSensorPin = ANALOG1;
            }

            if (i == 1)
            {
                tdsSensorPin = ANALOG2;
            }

            if (i == 3)
            {
                tdsSensorPin = ANALOG3;
            }

            pinMode(tdsSensorPin, INPUT);

            unsigned long messureTime = millis();
            do
            {
                static unsigned long analogSampleTimepoint = millis();
                if (millis() - analogSampleTimepoint > 40U) // every 40 milliseconds,read the analog value from the ADC
                {
                    analogSampleTimepoint = millis();
                    analogBuffer[analogBufferIndex] = analogRead(tdsSensorPin); // read the analog value and store into the buffer
                    analogBufferIndex++;
                    if (analogBufferIndex == SCOUNT)
                        analogBufferIndex = 0;
                }
                static unsigned long printTimepoint = millis();
                if (millis() - printTimepoint > 1800U)
                {
                    printTimepoint = millis();
                    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
                        analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
                    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0;                                                                                                  // read the analog value more stable by the median filtering algorithm, and convert to voltage value
                    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);                                                                                                               // temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
                    float compensationVolatge = averageVoltage / compensationCoefficient;                                                                                                            // temperature compensation
                    tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; // convert voltage value to tds value
                    // allSensors[TDS].measurements[0].value = tdsValue;
                    addMeassurments(allSensors[TDS].measurements[0].data_name, tdsValue);
                }
            } while (millis() - messureTime < 2000U);
        }
        break;

        case CAP_SOIL:
        {
            int cap_SoilPin;
            if (i == 0)
            {
                cap_SoilPin = ANALOG1;
            }

            if (i == 1)
            {
                cap_SoilPin = ANALOG2;
            }

            if (i == 3)
            {
                cap_SoilPin = ANALOG3;
            }

            pinMode(cap_SoilPin, INPUT);
            // allSensors[CAP_SOIL].measurements[0].value = analogRead(cap_SoilPin);
            addMeassurments(allSensors[CAP_SOIL].measurements[0].data_name, analogRead(cap_SoilPin));
        }
        break;

        case CAP_GROOVE:
        {
            int cap_GroovePin;
            bool cal_grooveConnected = false;
            if (i == 0)
            {
                cap_GroovePin = ANALOG1;
            }

            if (i == 1)
            {
                cap_GroovePin = ANALOG2;
            }

            if (i == 3)
            {
                cap_GroovePin = ANALOG3;
            }

            pinMode(cap_GroovePin, INPUT);
            // allSensors[CAP_GROOVE].measurements[0].value = analogRead(cap_GroovePin);
            addMeassurments(allSensors[CAP_SOIL].measurements[0].data_name, analogRead(cap_GroovePin));
        }
        break;

        default:
            break;
        }
    }
}

void readOneWire_Connectors()
{
    for (int i = 0; i < ONEWIRE_NUM; i++)
    {
        switch (OneWire_con_table[i])
        {
        case NO:
        {
            Serial.print("\nNo Sensor attaches at ");
            Serial.print("1-Wire_");
            Serial.println(i + 1);
        }
        break;

        case DHT_22:
        {
            int dht22SensorPin;
            if (i == 0)
            {
                dht22SensorPin = ONEWIRE_1;
            }

            if (i == 1)
            {
                dht22SensorPin = ONEWIRE_2;
            }

            if (i == 3)
            {
                dht22SensorPin = ONEWIRE_3;
            }

            DHT dht(dht22SensorPin, DHTTYPE);
            dht.begin(dht22SensorPin);
            delay(2000);
            // allSensors[DHT_22].measurements[0].value = dht.readTemperature();
            // allSensors[DHT_22].measurements[1].value = dht.readHumidity();
            addMeassurments(allSensors[DHT_22].measurements[0].data_name, dht.readTemperature());
            addMeassurments(allSensors[DHT_22].measurements[1].data_name, dht.readHumidity());
            break;
        }

        case DS18B20:
        {
            int ds18b20SensorPin;
            if (i == 0)
            {
                ds18b20SensorPin = ONEWIRE_1;
            }

            if (i == 1)
            {
                ds18b20SensorPin = ONEWIRE_2;
            }

            if (i == 3)
            {
                ds18b20SensorPin = ONEWIRE_3;
            }

            OneWire oneWire(ds18b20SensorPin);
            DallasTemperature sensors(&oneWire);
            sensors.begin();
            delay(2000);
            // allSensors[DS18B20].measurements[0].value = sensors.getTempCByIndex(0);
            addMeassurments(allSensors[DS18B20].measurements[0].data_name, sensors.getTempCByIndex(0));
            break;
        }

        default:
            break;
        }
    }
}

void readI2C_5V_Connector()
{
    if (I2C_5V_con_table[0] == MULTIGAS)
    {
        static uint8_t recv_cmd[8] = {};
        int PPM = 0;
        double lgPPM;
        GAS_GMXXX<TwoWire> gas;

        /* Multichannel sensor Grove V2, by vcoder 2021
            CO range 0 - 1000 PPM
            Calibrated according calibration curve by Winsen: 0 - 150 PPM

        RS/R0       PPM
        1           0
        0.77        1
        0.6         3
        0.53        5
        0.4         10
        0.29        20
        0.21        50
        0.17        100
        0.15        150
        */

        gas.begin(Wire, 0x04); // use the hardware I2C

        uint8_t len = 0;
        uint8_t addr = 0;
        uint8_t i;
        uint32_t val = 0;

        float sensor_volt;
        float RS_gas;
        float R0;

        float ratio;
        unsigned long messureTime = millis();
        do
        {
            float sensorValue = 0;
            for (int x = 0; x < 100; x++)
            {
                sensorValue = gas.measure_CO();
            }
            sensor_volt = (sensorValue / 1024) * 3.3;
            RS_gas = (3.3 - sensor_volt) / sensor_volt;

            R0 = 3.21; // measured on ambient air
            ratio = RS_gas / R0;
            // ratio = 1; //it is for tests of the calibration curve

            lgPPM = (log10(ratio) * -2.82) - 0.12; //- 3.82) - 0.66; - default      - 2.82) - 0.12; - best for range up to 150 ppm

            PPM = pow(10, lgPPM);

        } while (millis() - messureTime < 2000U);
        // allSensors[MULTIGAS].measurements[0].value = PPM;
        addMeassurments(allSensors[MULTIGAS].measurements[0].data_name, PPM);

        /* Multichannel sensor Grove V2, by vcoder 2021
            NO2 range 0 - 10 PPM
            Calibrated according calibration curve by Winsen: 0 - 10 PPM

        RS/R0       PPM
        1           0
        1.4         1
        1.8         2
        2.25        3
        2.7         4
        3.1         5
        3.4         6
        3.8         7
        4.2         8
        4.4         9
        4.7         10
        */

        messureTime = millis();
        do
        {
            float sensorValue = 0;
            for (int x = 0; x < 100; x++)
            {
                sensorValue = gas.measure_NO2();
            }

            sensor_volt = (sensorValue / 1024) * 3.3;
            RS_gas = (3.3 - sensor_volt) / sensor_volt;

            R0 = 1.07; // measured on ambient air
            ratio = RS_gas / R0;
            // ratio = 4.7; //for tests of the calibration curve

            lgPPM = (log10(ratio) * +1.9) - 0.2; //+2   -0.3

            PPM = pow(10, lgPPM);
        } while (millis() - messureTime < 2000U);

        // allSensors[MULTIGAS].measurements[1].value = PPM;
        addMeassurments(allSensors[MULTIGAS].measurements[0].data_name, PPM);
    }
    else if (I2C_5V_con_table[0] == MULTIGAS_V1)
    {
        MiCS6814 multiGasV1;
        // Connect to sensor using default I2C address (0x04)
        // Alternatively the address can be passed to begin(addr)

        multiGasV1.begin(0x04);
        multiGasV1.powerOn();

        addMeassurments(allSensors[MULTIGAS_V1].measurements[0].data_name, multiGasV1.measureCO());
        addMeassurments(allSensors[MULTIGAS_V1].measurements[1].data_name, multiGasV1.measureNO2());
        addMeassurments(allSensors[MULTIGAS_V1].measurements[2].data_name, multiGasV1.measureNH3());
        addMeassurments(allSensors[MULTIGAS_V1].measurements[3].data_name, multiGasV1.measureC3H8());
        addMeassurments(allSensors[MULTIGAS_V1].measurements[4].data_name, multiGasV1.measureC4H10());
        addMeassurments(allSensors[MULTIGAS_V1].measurements[5].data_name, multiGasV1.measureCH4());
        addMeassurments(allSensors[MULTIGAS_V1].measurements[6].data_name, multiGasV1.measureH2());
        addMeassurments(allSensors[MULTIGAS_V1].measurements[7].data_name, multiGasV1.measureC2H5OH());
    }
    else
    {
        Serial.print("\nNo Sensor attaches at ");
        Serial.println("I2C_5V");
    }
}

void readSPI_Connector()
{
    if (SPI_con_table[0] != NO)
    {
        Serial.println("\nNo SPI Sensor implemented now...");
    }
    else
    {
        Serial.print("\nNo Sensor attaches at ");
        Serial.println("SPI_CON");
    }
}

void readEXTRA_Connectors()
{
}

bool parseI2CAddress(const String &addressString, uint8_t *addressValue)
{
    char buffer[5];
    addressString.toCharArray(buffer, sizeof(buffer));
    char *endPtr;
    *addressValue = strtol(buffer, &endPtr, 16);

    if (*endPtr != '\0' || endPtr == buffer ||
        *addressValue > 0xFF)
    {
        return false;
    }

    return true;
}

void getHigh12SectionValue(void)
{
    memset(high_data, 0, sizeof(high_data));
    Wire.requestFrom(ATTINY1_HIGH_ADDR, 12);
    while (12 != Wire.available())
        ;

    for (int i = 0; i < 12; i++)
    {
        high_data[i] = Wire.read();
    }
    delay(10);
}

void getLow8SectionValue(void)
{
    memset(low_data, 0, sizeof(low_data));
    Wire.requestFrom(ATTINY2_LOW_ADDR, 8);
    while (8 != Wire.available())
        ;

    for (int i = 0; i < 8; i++)
    {
        low_data[i] = Wire.read(); // receive a byte as character
    }
    delay(10);
}

int getMedianNum(int bArray[], int iFilterLen)
{
    int bTab[iFilterLen];
    for (byte i = 0; i < iFilterLen; i++)
        bTab[i] = bArray[i];
    int i, j, bTemp;
    for (j = 0; j < iFilterLen - 1; j++)
    {
        for (i = 0; i < iFilterLen - j - 1; i++)
        {
            if (bTab[i] > bTab[i + 1])
            {
                bTemp = bTab[i];
                bTab[i] = bTab[i + 1];
                bTab[i + 1] = bTemp;
            }
        }
    }
    if ((iFilterLen & 1) > 0)
        bTemp = bTab[(iFilterLen - 1) / 2];
    else
        bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
    return bTemp;
}