#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h" //https://github.com/tzapu/WiFiManager
#include <FirebaseESP8266.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define FIREBASE_HOST "testsound-c65dd-default-rtdb.firebaseio.com"  
#define FIREBASE_AUTH "zsdBWuKvcJD4t7sLmAcGCHDjaEgrEiOPcwIVGrCQ"

int ledbao = 0;
int ledState1 = HIGH;         // the current state of the output pin
int buttonState1;             // the current reading from the input pin
int lastButtonState1 = LOW;   // the previous reading from the input pin
int timeBefore = 0;
int valueUARTReceivesCount = 0;
unsigned long lastDebounceTime1 = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 5000;    // the debounce time; increase if the output flickers
const byte touchPin1 = D5;
String iddevice = "100002";
String value = "";
String data = "";
String formattedDate;
String dayStamp;
String timeStamp;
String valueUARTReceive = "";
String valueUARTReceives[20];
String statusOutput = "";

WiFiManager wifiManager;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
FirebaseData fireBase;

void configModeCallback (WiFiManager *myWiFiManager)
{
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}
void checkButtonReset() {
   int reading1 = digitalRead(touchPin1); //gia tri nut nhan
   if (reading1 == HIGH) {
       digitalWrite(ledbao, !reading1);
   }
    if (reading1 != lastButtonState1) { //neu gia tri nut nhan hien tai ko bang gtri trc do
        lastDebounceTime1 = millis(); // thi set thoi gian bang tgian htai
    }
    if ((millis() - lastDebounceTime1) > debounceDelay) {
        if (reading1 != buttonState1) {
            buttonState1 = reading1;
            if (buttonState1 == HIGH) {
                ledState1 = !ledState1;
                digitalWrite(ledbao, HIGH);
                Serial.println("RESET ESP");
                wifiManager.resetSettings();
                ESP.reset();
            }
        }
    }
    lastButtonState1 = reading1;
}

void setup()
{
  Serial.begin(9600);
  pinMode(ledbao, OUTPUT);
  //Khai bao wifiManager
  //Setup callback de khoi dong AP voi SSID "ESP+chipID"
  wifiManager.setAPCallback(configModeCallback);
  if (!wifiManager.autoConnect("Hung-IOT", "123456789"))
  {
    Serial.println("failed to connect and hit timeout");
    //Neu ket noi that bai thi reset
    ESP.reset();
    delay(1000);
  }
  // Thanh cong thi bao ra man hinh
  Serial.println("connected...");
  int n = 0;
  int k = 0;
  while(n < 10){
    digitalWrite(ledbao, LOW);
    Serial.println("Bat ledbao");
    while (k < 100) //~~ 1s
    {
        checkButtonReset();
        delay(10);
        k++;
    }
    k = 0;
    digitalWrite(ledbao, HIGH);
    Serial.println("Tat ledbao");
    while (k < 100) //~~ 1s
    {
        checkButtonReset();
        delay(10);
        k++;
    }
    k = 0;
    n++;
  }
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  timeClient.begin();
  timeClient.setTimeOffset(25200);
  Serial.println("done");
}
void loop()
{
    getValueUART();
    getTime();
    updateFirebaseRealTime();
    updateFirebaseWithTime();
}

void getTime() {
  if( millis( ) - timeBefore > 1000ul )
  {
    while(!timeClient.update()) {
    timeClient.forceUpdate();
    }
    formattedDate = timeClient.getFormattedDate();
    Serial.println(formattedDate);

    // Extract date
    int splitT = formattedDate.indexOf("T");
    
    dayStamp = formattedDate.substring(0, splitT);

    Serial.print("DATE: ");
    Serial.println(dayStamp);
    // Extract time
    timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
    Serial.print("HOUR: ");
    Serial.println(timeStamp); 
    timeBefore = millis();
  }
}
void getValueUART() 
{   
//   if (Serial1.available() > 0) // Nếu có lệnh gửi đến
//   {
//     char c = Serial1.read(); //Đọc từng kí tự
//     // Kiểm tra kết thúc câu lệnh
//     if (c == '@'){
//       value = data; 
//       data = "";
//     } 
//     else // Nếu chưa phát hiện kí tự kết thúc câu lệnh thì tiết tục ghi nhận.
//     {
//       data = data + c;
//     }
//   }
    // door / speaker / waterPump / heater / fan / temperature / humidity / totalBird / "@";
    value = "1$0$1$1$1$33.00$75.30$35";
//   if (valueUARTReceive != value) {
    valueUARTReceive = value; //Serial đọc chuỗi
    if (valueUARTReceive != "") {
//      keyStartcheckManual = 1;
      while (valueUARTReceive.length() > 0)
      {
        int index = valueUARTReceive.indexOf('$');
        if (index == -1)
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
//   }
  return;
}

void updateFirebaseRealTime(){
  for (int i=0; i<8 ;i++) {
    if (i == 0 ) {
        statusOutput = "doorStatus";
    }
    else if (i == 1 ) {
        statusOutput = "speakerStatus";
    }
    else if (i == 2 ) {
        statusOutput = "waterPumpStatus";
    }
    else if (i == 3 ) {
        statusOutput = "heaterStatus";
    }
    else if (i == 4 ) {
        statusOutput = "fanStatus";
    }
    else if (i == 5 ) {
        statusOutput = "temperature";
    }
    else if (i == 6 ) {
        statusOutput = "humidity";
    }
    else if (i == 7 ) {
        statusOutput = "totalBird";
    }
    String DBaddH = iddevice + "/dataRealtime/" + statusOutput;
    Firebase.setString(fireBase,DBaddH,valueUARTReceives[i]); //
  }

}

void updateFirebaseWithTime(){
  String DBaddH = iddevice + "/" + "dataWithTime" + "/" + dayStamp + "/" + timeStamp + "/" + "temperature/" + "Value";
  Firebase.setString(fireBase,DBaddH,valueUARTReceives[5]); //
  
  String DBaddHs = iddevice + "/" + "dataWithTime" + "/" + dayStamp + "/" + timeStamp + "/" + "humidity/" + "Value";
  Firebase.setString(fireBase,DBaddHs,valueUARTReceives[6]);

}
