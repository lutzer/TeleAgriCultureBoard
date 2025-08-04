#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>

#define I2C_SDA 8 // on teleAgriCulture Board V2.0 I2C_5V SDA is GPIO 15
#define I2C_SCL 9 // on teleAgriCulture Board V2.0 I2C_5V SCL is GPIO 16

RTC_DS3231 rtc2;

#define DS1307_ADDRESS 0x68

class RtcWrapper {

public:
    static DateTime getTime() {

        Wire.begin(I2C_SDA, I2C_SCL);

        Serial.print("get time: ");

        int second = readRegister(0x00);
        int minute = readRegister(0x01);
        int hour = readRegister(0x02);
        int dayOfWeek = readRegister(0x03);
        int dayOfMonth = readRegister(0x04);
        int month = readRegister(0x05);
        int year = readRegister(0x06);

        Serial.print("Time: ");
        Serial.print(decode(hour));
        Serial.print(":");
        Serial.print(decode(minute));
        Serial.print(":");
        Serial.print(decode(second));
        Serial.print("  Date: ");
        Serial.print(decode(dayOfMonth));
        Serial.print("/");
        Serial.print(decode(month));
        Serial.print("/");
        Serial.println(decode(year));

        Wire.end();

        return DateTime((u_int32_t)0);
    }

    static void setTime(const DateTime& time) {
        Wire.begin(I2C_SDA, I2C_SCL);
        rtc2.begin();

        rtc2.adjust(time);

        Wire.end();
    }

    static byte readRegister(byte reg) {
        Wire.beginTransmission(DS1307_ADDRESS);
        Wire.write(reg);
        Wire.endTransmission();
        Wire.requestFrom(DS1307_ADDRESS, 1);
        return Wire.read();
    }

    static byte decode(byte bcd) {
        return ((bcd / 16) * 10 + (bcd % 16));
    }
};