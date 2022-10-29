#include <DFRobot_PH.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DHT.h"
#include <MiCS6814-I2C.h>
#include <FastLED.h>
#include <Servo.h>

#define TAC_ID 1001
#define TAC_API_KEY "CHANGEME"
#define ONE_WIRE_BUS 3
#define DHT_PIN 4
#define DHTTYPE DHT22
#define LED_PIN 6
#define NUM_LEDS 60

#define PH_PIN A2
#define SERVO_PIN 9
#define MOIST_PIN A3

Servo myservo;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DHT dht(DHT_PIN, DHTTYPE);
MiCS6814 gasSens;

CRGB leds[NUM_LEDS];

bool isGasSensConnected = false;
bool isSendingData = false;
byte inMsg[4];
byte outMsg[2];
uint8_t preheatTime = 10;

int posServo = 0;
float moistureValue = 0;
int phval = 0;
unsigned long int avgval; 
int buffer_arr[10],temp;
float calibration_value = 21.34;

struct RGBColor {
  byte r;
  byte g;
  byte b;
};

void setup(void)
{
  Serial.begin(9600);

  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, 0, NUM_LEDS);

  sensors.begin();
  dht.begin();
  myservo.attach(SERVO_PIN);

  isGasSensConnected = gasSens.begin();

  if (isGasSensConnected) {
    gasSens.powerOn();
  }
}
void loop(void)
{
  //first byte tells us which function (ie calibrate or send sensor data), second byte is used for data
  if(Serial.available() == 4) {
    for(int i = 0; i < 4; i++){
      inMsg[i] = Serial.read();
    }

    if(inMsg[0] == 1 && inMsg[1] == 0){
      if(isSendingData){
        isSendingData = false;
      }
      gsCalibrate();
    }

    if(inMsg[0] == 2 && inMsg[1] ==0 && inMsg[2] == 0 && inMsg[3] == 0){
      sendData();
    }

    if(inMsg[0] == 3){
      //do something with color
      RGBColor c = {inMsg[1], inMsg[2], inMsg[3]};
      setColor(c);
    }
     if(inMsg[0] == 4){
      //do something with the servo
      activateFeeder();
    }
                       

  }

}

float readPh() {

  for(int i=0;i<10;i++) { 
      buffer_arr[i]=analogRead(PH_PIN);
      delay(30);
   }
   
   for(int i=0;i<9;i++) {
    
     for(int j=i+1;j<10;j++) {
    
       if(buffer_arr[i]>buffer_arr[j]) {
          temp=buffer_arr[i];
          buffer_arr[i]=buffer_arr[j];
          buffer_arr[j]=temp;
        }
      }
    }
    
   avgval=0;
 
   for(int i=2;i<8;i++) avgval+=buffer_arr[i];
 
 
   float volt=(float)avgval*5.0/1024/6;
 
    float ph_act = -5.70 * volt + calibration_value;

    return ph_act;
}


float readMoist() {

  for (int i = 0; i <= 100; i++) 
 { 
   moistureValue = moistureValue + analogRead(MOIST_PIN); 
   delay(1); 
 } 
 moistureValue = moistureValue/100.0; 

 return moistureValue;
}

void activateFeeder() {


  myservo.attach(SERVO_PIN);
  delay(100);
  
  for (posServo = 0; posServo <= 180; posServo += 1) { 
    // in steps of 1 degree
    myservo.write(posServo);             
    delay(15);                  
  }
  for (posServo = 180; posServo >= 0; posServo -= 1) {
    myservo.write(posServo);             
    delay(15);                     
  }
  delay(100);
  myservo.detach();
}

void setColor(RGBColor c){
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i].setRGB(c.r, c.g, c.b);
  }

  FastLED.show();
}

void sendData(){
  sensors.requestTemperatures();
  Serial.print("id");
  Serial.print("\t");
  Serial.print(TAC_ID);
  Serial.print("\t");
  Serial.print("apiKey");
  Serial.print("\t");
  Serial.print(TAC_API_KEY);
  Serial.print("\t");
  Serial.print("ftTemp");
  Serial.print("\t");
  Serial.print(sensors.getTempCByIndex(0));
  Serial.print("\t");
  Serial.print("gbHum");
  Serial.print("\t");
  Serial.print(dht.readHumidity());
  Serial.print("\t");
  Serial.print("gbTemp");
  Serial.print("\t");
  Serial.print(dht.readTemperature());
  Serial.print("\t");
  Serial.print("Moist");
  Serial.print("\t");
  Serial.print(readMoist());
  Serial.print("\t");
  Serial.print("co");
  Serial.print("\t");
  Serial.print(gasSens.measureCO());
  Serial.print("\t");
  Serial.print("no2");
  Serial.print("\t");
  Serial.print(gasSens.measureNO2());
  Serial.print("\t");
  Serial.print("nh3");
  Serial.print("\t");
  Serial.print(gasSens.measureNH3());
  Serial.print("\t");
  Serial.print("c3h8");
  Serial.print("\t");
  Serial.print(gasSens.measureC3H8());
  Serial.print("\t");
  Serial.print("c4h10");
  Serial.print("\t");
  Serial.print(gasSens.measureC4H10());
  Serial.print("\t");
  Serial.print("ch4");
  Serial.print("\t");
  Serial.print(gasSens.measureCH4());
  Serial.print("\t");
  Serial.print("h2");
  Serial.print("\t");
  Serial.print(gasSens.measureH2());
  Serial.print("\t");
  Serial.print("c2h5oh");
  Serial.print("\t");
  Serial.print(gasSens.measureC2H5OH());
  Serial.print("\t");
  Serial.print("pH");
  Serial.print("\t");
  Serial.println(readPh());
}

void gsCalibrate(){
  if (isGasSensConnected) {
    while (preheatTime > 0) {
      outMsg[0] = 1;
      outMsg[1] = 2;
      Serial.write(outMsg, sizeof(outMsg));
      delay(60000);
      preheatTime = preheatTime - 1;
    }

    outMsg[0] = 1;
    outMsg[1] = 3;
    Serial.write(outMsg, sizeof(outMsg));
    gasSens.calibrate();

    outMsg[0] = 1;
    outMsg[1] = 1;
    Serial.write(outMsg, sizeof(outMsg));
  }
}
