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
unsigned long timeBefore;
unsigned long timeBeforeManual;
unsigned long timeBeforeGetUART;
unsigned long timeBeforeSendUART;
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
String get_modeStatusFirebase;
String get_setTemperatureMinFirebase;
String get_setTemperatureMaxFirebase;
String get_setHumidityMinFirebase;
String get_setHumidityMaxFirebase;
String get_speakerStatusFirebase;
String get_doorStatusFirebase;
String get_waterPumpStatusFirebase;
String get_heaterStatusFirebase;
String get_fanStatusFirebase;
String get_fan2StatusFirebase;

String get_setTimeTurnOnSpeakerFirebase;
String get_setTimeTurnOffSpeakerFirebase;
String stringManual;

WiFiManager wifiManager;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
FirebaseData fireBase;

void configModeCallback (WiFiManager *myWiFiManager)
{
  // Serial.println("Entered config mode");
  // Serial.println(WiFi.softAPIP());
  // Serial.println(myWiFiManager->getConfigPortalSSID());
  WiFi.softAPIP();
  myWiFiManager->getConfigPortalSSID();
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
    // Serial.println("failed to connect and hit timeout");
    //Neu ket noi that bai thi reset
    ESP.reset();
    delay(1000);
  }
  // Thanh cong thi bao ra man hinh
  // Serial.println("connected...");
  int n = 0;
  int k = 0;
  while(n < 1) {
    digitalWrite(ledbao, LOW);
    while (k < 100) //~~ 1s
    {
        checkButtonReset();
        delay(10);
        k++;
    }
    k = 0;
    digitalWrite(ledbao, HIGH);
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
  // Serial.println("done");
  delay(10000);
}
void loop()
{
  getValueUART();
  getFirebase();
  if (get_modeStatusFirebase == "1") {
    if((unsigned long) (millis() - timeBeforeManual) > 1000)
    {
      for(int i=5; i < 8; i++) {
        if (i == 5 ) {
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
      timeBeforeManual = millis();
      stringManual = get_modeStatusFirebase + "#" + get_setTemperatureMinFirebase + "#" + get_setTemperatureMaxFirebase + "#" + get_setHumidityMinFirebase + "#" + get_setHumidityMaxFirebase + "#" + get_speakerStatusFirebase + "#" + get_doorStatusFirebase + "#" + get_waterPumpStatusFirebase + "#" + get_heaterStatusFirebase + "#" + get_fanStatusFirebase + "#" + get_setTimeTurnOnSpeakerFirebase + "#" + get_setTimeTurnOffSpeakerFirebase + "#" + get_fan2StatusFirebase + "#";
      //mode|TemperatureMin|TemperatureMax|HumidityMin|HumidityMax|speaker|door|waterPump|heater|fan|TimeTurnOnSpeaker|TimeTurnOffSpeaker + "$";
    }
  }
  else {
      updateFirebaseRealTime();
      if (stringManual != "0############") {
        stringManual = "0############";
      }
  }
  if((unsigned long) (millis() - timeBeforeSendUART) > 1000)
  {
    Serial.println(stringManual);
    timeBeforeSendUART = millis();
  }
  if((unsigned long) (millis() - timeBefore) > 5000)
  {
      getTime();
      updateFirebaseWithTime();
      timeBefore = millis();
  }
  valueUARTReceivesCount = 0;
}


void getTime() {
    while(!timeClient.update()) {
        timeClient.forceUpdate();
    }
    formattedDate = timeClient.getFormattedDate();
    int splitT = formattedDate.indexOf("T");
    dayStamp = formattedDate.substring(0, splitT);
    timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
    // dayStamp = valueUARTReceives[8];
    // timeStamp = valueUARTReceives[9];
    return;
}
void getValueUART() 
{
  if((unsigned long) (millis() - timeBeforeGetUART) > 200)
  {   
    while(Serial.available() > 0) {
      value = Serial.readString();
    }
      // door / speaker / waterPump / heater / fan / temperature / humidity / totalBird / "@";
    if (valueUARTReceive != value) {
      valueUARTReceive = value; //Serial đọc chuỗi
      if (valueUARTReceive != "") {
        while (valueUARTReceive.length() > 0)
        {
          int index = valueUARTReceive.indexOf('#');
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
    }
  }
  value = "";
  return;
}

void updateFirebaseRealTime(){
  for (int i=0; i<9 ;i++) {
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
    else if (i == 8 ) {
        statusOutput = "fan2Status";
    }
    String DBaddH = iddevice + "/dataRealtime/" + statusOutput;
    Firebase.setString(fireBase,DBaddH,valueUARTReceives[i]); //
  }

}

void updateFirebaseWithTime(){
  String DBaddH = iddevice + "/" + "dataWithTime" + "/" + dayStamp + "/" + timeStamp + "/" + "temperature";
  Firebase.setString(fireBase,DBaddH,valueUARTReceives[5]); //

  String DBaddHs = iddevice + "/" + "dataWithTime" + "/" + dayStamp + "/" + timeStamp + "/" + "humidity";
  Firebase.setString(fireBase,DBaddHs,valueUARTReceives[6]);

  String DBaddHss = iddevice + "/" + "dataWithTime" + "/" + dayStamp + "/" + timeStamp + "/" + "timeStamp";
  Firebase.setString(fireBase,DBaddHss,timeStamp);
}
void getFirebase() {
    Firebase.getString(fireBase,iddevice + "/modeStatus");
    get_modeStatusFirebase = fireBase.stringData();
    if (get_modeStatusFirebase == "1") {
        Firebase.getString(fireBase,iddevice + "/setValue/setHumidityMin");
        get_setHumidityMinFirebase = fireBase.stringData();

        Firebase.getString(fireBase,iddevice + "/setValue/setHumidityMax");
        get_setHumidityMaxFirebase = fireBase.stringData();

        Firebase.getString(fireBase,iddevice + "/setValue/setTemperatureMin");
        get_setTemperatureMinFirebase = fireBase.stringData();

        Firebase.getString(fireBase,iddevice + "/setValue/setTemperatureMax");
        get_setTemperatureMaxFirebase = fireBase.stringData();

        Firebase.getString(fireBase,iddevice + "/setValue/setTimeTurnOnSpeaker");
        get_setTimeTurnOnSpeakerFirebase = fireBase.stringData();

        Firebase.getString(fireBase,iddevice + "/setValue/setTimeTurnOffSpeaker");
        get_setTimeTurnOffSpeakerFirebase = fireBase.stringData();


        Firebase.getString(fireBase,iddevice + "/dataRealtime/doorStatus");
        get_doorStatusFirebase = fireBase.stringData();

        Firebase.getString(fireBase,iddevice + "/dataRealtime/fanStatus");
        get_fanStatusFirebase = fireBase.stringData();

        Firebase.getString(fireBase,iddevice + "/dataRealtime/fan2Status");
        get_fan2StatusFirebase = fireBase.stringData();

        Firebase.getString(fireBase,iddevice + "/dataRealtime/heaterStatus");
        get_heaterStatusFirebase = fireBase.stringData();

        Firebase.getString(fireBase,iddevice + "/dataRealtime/speakerStatus");
        get_speakerStatusFirebase = fireBase.stringData();

        Firebase.getString(fireBase,iddevice + "/dataRealtime/waterPumpStatus");
        get_waterPumpStatusFirebase = fireBase.stringData();
    }
    return;
}
void serialPrint() {
}
