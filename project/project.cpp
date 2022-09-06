#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h> 
#include<LiquidCrystal_I2C_Hangul.h>
#include<Wire.h>
#include "RTClib.h"
#include <DHT.h> // Gọi thư viện DHT22
#define RST_PIN 5    
#define SS_PIN 53   
#define SERVO_PIN 12
#define DHT_SENSOR_TYPE DHT_TYPE_22

float setTemperatureMin = 27.00;
float setTemperatureMax = 29.00;
float setHumidityMin = 70.00;
float setHumidityMax = 80.00;
int setTimeTurnOnSpeaker = 80000;
int setTimeTurnOffSpeaker = 170000;

int IDcardbyte;
int ledStatus = LOW; 
int led = 35; //led báo mở cửa
int waterPump = 37; //bơm nước
int heater = 39; //lò sưởi
int fan = 41; //quạt
int fan2 = 45; //quạt2
int speaker = 43; //loa


int IRSensor1 = 31;
int IRSensor2 = 33;

int totalBird = 10;
int conditionBreak = 0;

int heaterStatus = 0;
int waterPumpStatus = 0;
int fanStatus = 0;
int fan2Status = 0;

int doorStatus = 0;
int speakerStatus = 0;

int valueUARTReceivesCount = 0;
int timeNowInt = 0;

unsigned long  timeBefore;
unsigned long timeBeforeUart;
unsigned long timeBeforeGetUART;
float humidity;
float temperature;

String setDoor = "";
String setwaterPumpStatus = "";
String contentLCDLine1After = "";
String contentLCDLine2After = "";
String valueUARTSend = "";
String valueUARTReceive = "";
String valueUARTReceives[20];
String IDcard = "";
String keySecurity = "3911227216";
String heaterStatusText = "";
String waterPumpStatusText = "";
String fanStatusText = "";
String fan2StatusText = "";
String doorStatusText = "";
String speakerStatusText = "";
String textInArray = "";
String value = "";
String data = "";
String timeNow = "";
String timeNowUart = "";
String dateNow = "";
String contentLCDLine1Before = "";
String contentLCDLine2Before = "";

 
const int DHTPIN = A3; //Đọc dữ liệu từ DHT22 ở chân A3 trên mạch Arduino

const int DHTTYPE = DHT22; //Khai báo loại cảm biến, có 2 loại là DHT11 và DHT22

DHT dht(DHTPIN, DHTTYPE);

LiquidCrystal_I2C_Hangul lcd(0x27,16,2); 
MFRC522 mfrc522(SS_PIN, RST_PIN); 
Servo gServo;
RTC_DS1307 RTC;

void setup() {
  Serial.begin(9600); 
  pinMode(led, OUTPUT);
  pinMode(waterPump, OUTPUT);
  pinMode(heater, OUTPUT);
  pinMode(fan, OUTPUT);
  pinMode(fan2, OUTPUT);
  pinMode(speaker, OUTPUT);
  gServo.attach(SERVO_PIN); 
  delay(200);
  SPI.begin();    
  delay(200);
  dht.begin(); // Khởi động cảm biến
  delay(200);
  mfrc522.PCD_Init(); 
  delay(200);
  RTC.begin();      // Khoi dong RTC
  delay(200);
  // Dong bo thoi gian voi may tinh
  RTC.adjust(DateTime(__DATE__, __TIME__)); 
  delay(200);
  digitalWrite(led, HIGH);
  digitalWrite(waterPump, HIGH);
  digitalWrite(heater, HIGH);
  digitalWrite(fan, HIGH);
  digitalWrite(fan2, HIGH);
  digitalWrite(speaker, HIGH);
  humidity = dht.readHumidity(); //Đọc độ ẩm
  temperature = dht.readTemperature(); //Đọc nhiệt độ 
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Setup done!");
  delay(3000);
  lcd.clear();
  gServo.write(0);
//  while(Serial.available() > 0) {
//    value = Serial.readString();
//  }
  value = "";
  delay(50);
  // Serial.println("Setup done!");
}


void loop() 
{ 
//  getValueUART();
  checkManual();
  doorAccessControl();
  speakerControl();
  getValueDHT22();
  demChim();
  controlOutput();
  printLCD();
  // serialPrint();
  if ((unsigned long) (millis() - timeBeforeUart) > 1000)
  {
    valueUARTSend = String(doorStatus) + "#" + String(speakerStatus) + "#" + String(waterPumpStatus) + "#" + String(heaterStatus) + "#" + String(fanStatus) + "#" + String(temperature) + "#" + String(humidity) + "#" + String(totalBird) + "#" + String(fan2Status) + "#";
    Serial.println(valueUARTSend);
    timeBeforeUart = millis();
  }

}
void serialPrint() {
  Serial.print("temperature: ");
  Serial.println(temperature);
  Serial.print("humidity: ");
  Serial.println(humidity);
  Serial.print("totalBird: ");
  Serial.println(totalBird);
  Serial.print("timeNowInt: ");
  Serial.println(timeNowInt);
  Serial.print("doorStatus: ");
  Serial.println(doorStatus);
  Serial.print("heaterStatus: ");
  Serial.println(heaterStatus);
  Serial.print("waterPumpStatus: ");
  Serial.println(waterPumpStatus);
  Serial.print("fanStatus: ");
  Serial.println(fanStatus);
  Serial.print("speakerStatus: ");
  Serial.println(speakerStatus);
  Serial.println(contentLCDLine1After);
  Serial.println(contentLCDLine2After);
  Serial.println("==========");
}
void speakerControl() {
  DateTime now = RTC.now(); // Thoi gian = thoi gian RTC hien tai
  timeNow = String(now.hour(), DEC) + String(now.minute(), DEC) + String(now.second(), DEC);
  timeNowUart = String(now.hour(), DEC) + ":" + String(now.minute(), DEC) + ":" + String(now.second(), DEC);
  dateNow = String(now.year(), DEC) + "-" + String(now.month(), DEC) + "-" + String(now.day(), DEC);
  timeNowInt = timeNow.toInt();
  if (valueUARTReceives[0] != "1") {
    if (timeNowInt >= setTimeTurnOnSpeaker and timeNowInt < setTimeTurnOffSpeaker) {
      speakerStatus = 1;
    }
    if (timeNowInt < setTimeTurnOnSpeaker or timeNowInt >= setTimeTurnOffSpeaker) {
      speakerStatus = 0;
    }
  }
  return;
}


void checkManual() {
  
  if (valueUARTReceives[0] == "1") { //chế độ tay
    if (valueUARTReceives[1] != "") {
      setTemperatureMin = (valueUARTReceives[1]).toFloat();
    }

    if (valueUARTReceives[2] != "") {
      setTemperatureMax = (valueUARTReceives[2]).toFloat();
    }

    if (valueUARTReceives[3] != "") {
      setHumidityMin = (valueUARTReceives[3]).toFloat();
    }

    if (valueUARTReceives[4] != "") {
      setHumidityMax = (valueUARTReceives[4]).toFloat();
    }

    if (valueUARTReceives[5] != "") {
      speakerStatus = (valueUARTReceives[5]).toInt();
    }

    if (valueUARTReceives[6] != "") {
      doorStatus = (valueUARTReceives[6]).toInt();
    }

    if (valueUARTReceives[7] != "") {
      waterPumpStatus = (valueUARTReceives[7]).toInt();
    }

    if (valueUARTReceives[8] != "") {
      heaterStatus = (valueUARTReceives[8]).toInt();
    }

    if (valueUARTReceives[9] != "") {
      fanStatus = (valueUARTReceives[9]).toInt();
    }

    if (valueUARTReceives[10] != "") {
      setTimeTurnOnSpeaker = (valueUARTReceives[10]).toInt();
    }
    
    if (valueUARTReceives[11] != "") {
      setTimeTurnOffSpeaker = (valueUARTReceives[11]).toInt();
    }
    if (valueUARTReceives[12] != "") {
      fan2Status = (valueUARTReceives[12]).toInt();
    }
    valueUARTReceivesCount = 0;
  }
}
void getValueUART() {
  if((unsigned long) (millis() - timeBeforeGetUART) > 200)
  {
    if (Serial.available() > 0) {
      value = Serial.readString();
    }
    if (valueUARTReceive != value) {
      valueUARTReceive = value; //Serial đọc chuỗi
      if (valueUARTReceive != "") {
        while (valueUARTReceive.length() > 0)
        {
          int index = valueUARTReceive.indexOf('#');
          if (index == -1) // No space found
          {
            valueUARTReceives[valueUARTReceivesCount++] = valueUARTReceive;
            valueUARTReceivesCount = 0;
            break;
          }
          else
          {
            valueUARTReceives[valueUARTReceivesCount++] = valueUARTReceive.substring(0, index);
            valueUARTReceive = valueUARTReceive.substring(index+1);
          }
        }
      } 
      // for (int i=0; i<12; i++)
      // {
      //   Serial.print(valueUARTReceives[i] + "|");
      // }
    }
    timeBeforeGetUART = millis();
  }
  value = "";
  return;
}
void getValueDHT22() {
  if((unsigned long) (millis() - timeBefore) > 5000)
  {
    humidity = dht.readHumidity(); //Đọc độ ẩm
    temperature = dht.readTemperature(); //Đọc nhiệt độ
    timeBefore = millis();
  }
  if (valueUARTReceives[0] != "1") {
    if (temperature > setTemperatureMax) {
      fanStatus = 1;
      heaterStatus = 0;
    }
    if (temperature < setTemperatureMin) {
        fanStatus = 0;
        heaterStatus = 1;
    }
    if (temperature > setTemperatureMin and temperature < setTemperatureMax) {
        fanStatus = 0;
        heaterStatus = 0;
    }
    
    if (humidity < setHumidityMin) {
      fan2Status = 0;
      waterPumpStatus = 1;
    }
    if (humidity > setHumidityMax) {
        fan2Status = 1;
        waterPumpStatus = 0;
    }
    if (humidity > setHumidityMin and humidity < setHumidityMax) {
        fan2Status = 0;
        waterPumpStatus = 0;

    }
  }
  return;
}
void doorAccessControl() {
  if(mfrc522.PICC_IsNewCardPresent()) 
  {
    if(mfrc522.PICC_ReadCardSerial()) 
    {
      for (byte i = 0; i < mfrc522.uid.size; i++) 
      {
          IDcardbyte = mfrc522.uid.uidByte[i];
          IDcard = IDcard + String(IDcardbyte);  
      }  
      mfrc522.PICC_HaltA();    
    } 
  }
  if (IDcard == keySecurity) 
  {
    ledStatus = !ledStatus;
    if (ledStatus == HIGH)
    {
      doorStatus = 1;
    }
    else 
    {
      doorStatus = 0;
    }
  }
  IDcard = "";
  return;  
}
void demChim() {
  //dem chim vao
  if (digitalRead(IRSensor2) == 0) {
      while(1) {
          if (digitalRead(IRSensor2) == 1) {
              if (digitalRead(IRSensor1) == 0){
                  while(1) {
                    if (digitalRead(IRSensor1) == 1) {
                        totalBird = totalBird + 1;
                        conditionBreak = 1;
                        break;
                    }
                    delay(100);
                  }
              }
          }
          delay(100);
          if (conditionBreak == 1) {
              conditionBreak = 0;
              break;
          }
      }
  }
  //dem chim ra
  if (digitalRead(IRSensor1) == 0) {
      while(1) {
          if (digitalRead(IRSensor1) == 1) {
              if (digitalRead(IRSensor2) == 0){
                  while(1) {
                      if (digitalRead(IRSensor2) == 1) {
                          totalBird = totalBird - 1;
                          conditionBreak = 1;
                          break;
                      }
                      delay(100);
                  }
              }
          }
          delay(100);
          if (conditionBreak == 1) {
              conditionBreak = 0;
              break;
          }
      }
  }
  return;
}
void controlOutput() {
  if (speakerStatus == 1) {
    digitalWrite(speaker, LOW);
    speakerStatusText = "O";
  }
  if (speakerStatus == 0) {
    digitalWrite(speaker, HIGH);
    speakerStatusText = "F";
  }
  
  if (heaterStatus == 1) {
    digitalWrite(heater, LOW);
    heaterStatusText = "O";
  }
  if (heaterStatus == 0) {
    digitalWrite(heater, HIGH);
    heaterStatusText = "F";
  }

  if (waterPumpStatus == 1) {
    digitalWrite(waterPump, LOW);
    waterPumpStatusText = "O";
  }
  if (waterPumpStatus == 0) {
    digitalWrite(waterPump, HIGH);
    waterPumpStatusText = "F";
  }

  if (fanStatus == 1) {
    digitalWrite(fan, LOW);
    fanStatusText = "O";
  }
  if (fanStatus == 0) {
    digitalWrite(fan, HIGH);
    fanStatusText = "F";
  }

  if (fan2Status == 1) {
    digitalWrite(fan2, LOW);
    fan2StatusText = "O";
  }
  if (fan2Status == 0) {
    digitalWrite(fan2, HIGH);
    fan2StatusText = "F";
  }
  
  if (doorStatus == 1) 
  {
    gServo.write(180);
    digitalWrite(led, LOW);
    doorStatusText = "O";
  }
  if (doorStatus == 0) 
  {
    gServo.write(0);
    digitalWrite(led, HIGH);
    doorStatusText = "C";
  }
}
void printLCD() {
  contentLCDLine1After = doorStatusText + "|" + fan2StatusText + "|" + waterPumpStatusText + "|" + heaterStatusText + "|" + fanStatusText + "|" + speakerStatusText + "|" + String(totalBird) + "      ";
  contentLCDLine2After = "T:" + String(temperature) + "|" + "H:" + String(humidity);
  if (contentLCDLine1After != contentLCDLine1Before) {
    lcd.setCursor(0, 0);
    lcd.print(contentLCDLine1After);
    contentLCDLine1Before = contentLCDLine1After;
  }
  if (contentLCDLine2After != contentLCDLine2Before) {
    lcd.setCursor(0, 1);
    lcd.print(contentLCDLine2After);
    contentLCDLine2Before = contentLCDLine2After;
  }
}