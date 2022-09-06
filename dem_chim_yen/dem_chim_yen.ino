#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <FirebaseESP8266.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
const long utcOffsetInSeconds = 25200; 
ESP8266WebServer server(80);
WiFiUDP ntpUDP; 
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

String iddevice = "100002";

const char* myssid="iotForHung";       //viết têN wifi muốn truy cập
const char* mypassword="";                // để trống nếu không muốn có mật Khẩu
String ssid = "";
String pass = "";

int IRSensor1 = 16;
int IRSensor2 = 5;

int totalBird = 10;
int conditionBreak = 0;

int valueUARTReceivesCount = 0;
int ledbao = 16;
int ledState1 = HIGH;         // the current state of the output pin
int buttonState1;             // the current reading from the input pin
int lastButtonState1 = LOW;   // the previous reading from the input pin
unsigned long lastDebounceTime1 = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 5000;    // the debounce time; increase if the output flickers
const byte touchPin1 = 4;

String formattedDate;
String dayStamp;
String timeStamp;
String valueUARTReceive = "";
String valueUARTReceives[20];
String statusOutput = "";

#define FIREBASE_HOST "testsound-c65dd-default-rtdb.firebaseio.com"  
#define FIREBASE_AUTH "zsdBWuKvcJD4t7sLmAcGCHDjaEgrEiOPcwIVGrCQ"

FirebaseData gui;

String data = "";   //Khai báo biến data lưu lệnh mà master gửi đến
String value;


//=========Biến chứa mã HTLM Website==//
const char MainPage[] PROGMEM = R"=====(
  <!DOCTYPE html> 
  <html>
   <head> 
       <title>WIFI MANAGE</title> 
       <style> 
          body {text-align:center;background-color:#222222; color:white}
          input {
            height:25px; 
            width:270px;
            font-size:20px;
            margin: 10px auto;
          }
          #content {
            border: white solid 1px; 
            padding:5px;  
            height:375px; 
            width:330px; 
            border-radius:20px;
            margin: 0 auto
          }
          #ledconnect{
              outline: none;
              margin: 0px 5px -1px 5px;
              width: 15px;
              height: 15px;
              border: solid 1px #00EE00;
              background-color: #00EE00;
              border-radius: 50%;
              -moz-border-radius: 50%;
              -webkit-border-radius: 50%;
          }

          .button_wifi{
            height:50px; 
            width:90px; 
            margin:10px 0;
            outline:none;
            color:white;
            font-size:15px;
            font-weight: bold;
          }
          #button_save {
            background-color:#00BB00;
            border-radius:5px;
          }
          #button_restart {
            background-color:#FF9900;
            border-radius:5px;
          }
          #button_reset {
            background-color:#CC3300;
            border-radius:5px;
          }
       </style>
       <meta name="viewport" content="width=device-width,user-scalable=0" charset="UTF-8">
   </head>
   <body> 
      <div><h1>WIFI MANAGE</h1></div>
      <div id="content">
        <div id="wifisetup" style="height:340px; font-size:20px; display:none";>
          <div style="text-align:left; width:270px; margin:5px 25px">SSID: </div>
          <div><input id="ssid"/></div>
          <div style="text-align:left; width:270px; margin:5px 25px">Password: </div>
          <div><input id="pass"/></div>
          <div>
            <button id="button_save" class="button_wifi" onclick="writeEEPROM()">SAVE</button>
            <button id="button_restart" class="button_wifi" onclick="restartESP()">RESTART</button>
            <button id="button_reset" class="button_wifi" onclick="clearEEPROM()">RESET</button>
          </div>
          <div id="reponsetext"></div>
        </div>
        <div id="userlogin" style="height:340px; font-size:20px; display:none";>
            <div style="text-align:left; width:270px; margin:5px 25px">Username: </div>
            <div><input id="username"/></div>
            <div style="text-align:left; width:270px; margin:5px 25px">Password: </div>
            <div><input type="password" id="password"/></div>
            <div>
              <button id="button_login" class="button_wifi" onclick="userlogin()">LOGIN</button>
            </div>
          </div>
          <div><input id="ledconnect" type="button"/>Connection status</div>
        </div>
      <div id="footer">
      </div>
      <script>
        //-----------Hàm khởi tạo đối tượng request----------------
        function create_obj(){
          td = navigator.appName;
          if(td == "Microsoft Internet Explorer"){
            obj = new ActiveXObject("Microsoft.XMLHTTP");
          }else{
            obj = new XMLHttpRequest();
          }
          return obj;
        }
        //------------Khởi tạo biến toàn cục-----------------------------
        var xhttp = create_obj();//Đối tượng request cho setup wifi
        var xhttp_statusD = create_obj();//Đối tượng request cho cập nhật trạng thái
        var ledconnect = 1;
        //===================Khởi tạo ban đầu khi load trang=============
        window.onload = function(){
          document.getElementById("wifisetup").style.display = "none";
          document.getElementById("userlogin").style.display = "block";
          getIPconnect();//Gửi yêu cầu lấy IP kết nối
          getstatusD();//Gửi yêu cầu lấy trạng thái các chân điều khiển
        }
        //===========Configure WiFi=====================================
        function configurewifi(){
          document.getElementById("userlogin").style.display = "none";
          document.getElementById("wifisetup").style.display = "block";
        }
        //===================IPconnect====================================
        //--------Tạo request lấy địa chỉ IP kết nối----------------------
        function getIPconnect(){
          xhttp.open("GET","/getIP",true);
          xhttp.onreadystatechange = process_ip;//nhận reponse 
          xhttp.send();
        }
        //-----------Kiểm tra response IP và hiển thị------------------
        function process_ip(){
          if(xhttp.readyState == 4 && xhttp.status == 200){
            //------Updat data sử dụng javascript----------
            ketqua = xhttp.responseText; 
            document.getElementById("ipconnected").innerHTML=ketqua;       
          }
        }
        //-----------Thiết lập dữ liệu và gửi request ssid và password---
        function writeEEPROM(){
          if(Empty(document.getElementById("ssid"), "Please enter ssid!")&&Empty(document.getElementById("pass"), "Please enter password")){
            var ssid = document.getElementById("ssid").value;
            var pass = document.getElementById("pass").value;
            xhttp.open("GET","/writeEEPROM?ssid="+ssid+"&pass="+pass,true);
            xhttp.onreadystatechange = process;//nhận reponse 
            xhttp.send();
          }
        }
        function userlogin(){
          var $username = document.getElementById("username").value;
          var $password = document.getElementById("password").value;
          if ( $username == "admin" && $password == "1234"){
          // alert ("Dang nhap thanh cong");
          document.getElementById("wifisetup").style.display = "block";
          document.getElementById("userlogin").style.display = "none";
          return false;
          }
          else{
          alert ("Tai khoan hoac mat khau sai. Vui long dang nhap lai");
          }
        }
        function clearEEPROM(){
          if(confirm("Do you want to delete all saved wifi configurations?")){
            xhttp.open("GET","/clearEEPROM",true);
            xhttp.onreadystatechange = process;//nhận reponse 
            xhttp.send();
          }
        }
        function restartESP(){
          if(confirm("Do you want to reboot the device?")){
            xhttp.open("GET","/restartESP",true);
            xhttp.send();
            alert("Device is restarting! If no wifi is found please press reset!");
          }
        }
        //-----------Kiểm tra response -------------------------------------------
        function process(){
          if(xhttp.readyState == 4 && xhttp.status == 200){
            //------Updat data sử dụng javascript----------
            ketqua = xhttp.responseText; 
            document.getElementById("reponsetext").innerHTML=ketqua;       
          }
        }
       //============Hàm thực hiện chứ năng khác================================
       //--------Cập nhật trạng thái tự động sau 2000 giây----------------------
        setInterval(function() {
          getstatusD();
        },2000);
       //----------------------------CHECK EMPTY--------------------------------
       function Empty(element, AlertMessage){
          if(element.value.trim()== ""){
            alert(AlertMessage);
            element.focus();
            return false;
          }else{
            return true;
          }
       }
       //------------------------------------------------------------------------
      </script>
   </body> 
  </html>
)=====";

void setup() {
  Serial.begin(9600);
  EEPROM.begin(512);       //Khởi tạo bộ nhớ EEPROM
  pinMode(touchPin1, INPUT);
  pinMode(IRSensor1, INPUT);
  pinMode(IRSensor2 , INPUT);
  pinMode(5,OUTPUT);
  digitalWrite(5,LOW);
  delay(10);
  timeClient.begin();
  
if(read_EEPROM()){
    WiFi.begin(ssid,pass);
    int count=0;
    while(count < 30){
      if(WiFi.status() == WL_CONNECTED){
        Serial.println();
        Serial.print("Connected to ");
        Serial.println(ssid);
        Serial.print("Web Server IP Address: ");
        Serial.println(WiFi.localIP());
        //neu ket noi duoc wifi thi nhay led 10 lan
        int n = 0;
        while(n < 10){
          digitalWrite(ledbao, HIGH);
          delay(1000);
          digitalWrite(ledbao, LOW);
          delay(1000);
          n++;
        }
        break;
      }
      digitalWrite(ledbao, HIGH);
      delay(200);
      digitalWrite(ledbao, LOW);
      delay(200);
      Serial.print(".");
      count++;
    }
  }
  //neu khong ket noi duoc wifi thi nhay led 3 lan
  if(WiFi.status() != WL_CONNECTED){
    int n = 0;
    while(n < 3){
      digitalWrite(ledbao, HIGH);
      delay(1000);
      digitalWrite(ledbao, LOW);
      delay(1000);
      n++;
    }
    Serial.println(".");
    Serial.println("Timed out.");
    Serial.print("Tạo Chế Độ Ap ");
    WiFi.softAP(myssid,mypassword);    
    delay(5000);
    //Print local IP address and start web server
    Serial.println("");
    Serial.println("Địa Chỉ IP là: ");
    Serial.println(WiFi.localIP());
    Serial.println("Chuẩn Đoán Wifi");
    WiFi.printDiag(Serial);
    server.on("/",mainpage);
    server.on("/getIP",get_IP);
    server.on("/writeEEPROM",write_EEPROM);
    server.on("/restartESP",restart_ESP);
    server.on("/clearEEPROM",clear_EEPROM);
    server.begin();
    while(1){
      server.handleClient(); 
    }
  }
  Serial.println("Thoat setup");
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  //Firebase.setString(gui,"SD","2");
}
void loop() {
    getValueUART() ;
    demChim();
    updateFirebaseRealTime();
    updateFirebaseWithTime();
    checkbuttonreset(); 
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
    value = "1$0$1$1$1$0$1";
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
  for (int i=0; i<7 ;i++) {
    if (i == 0 ) {
        statusOutput = "doorStatus";
    }
    else if (i == 1 ) {
        statusOutput = "speakerStatus";
    }
    else if (i == 2 ) {
        statusOutput = "temperature";
    }
    else if (i == 3 ) {
        statusOutput = "humidity";
    }
    else if (i == 4 ) {
        statusOutput = "waterPumpStatus";
    }
    else if (i == 5 ) {
        statusOutput = "heaterStatus";
    }
    else if (i == 6 ) {
        statusOutput = "fanStatus";
    }
    String DBaddH = iddevice + "/dataRealtime/" + statusOutput;
    Firebase.setString(gui,DBaddH,valueUARTReceives[i]); //
  }
  Firebase.setString(gui,iddevice + "/totalBird/",String(totalBird));

}

void updateFirebaseWithTime(){
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

  String DBaddH = iddevice + "/" + "dataWithTime" + "/" + dayStamp + "/" + timeStamp + "/" + "temperature/" + "Value";
  Firebase.setString(gui,DBaddH,valueUARTReceives[2]); //
  
  String DBaddHs = iddevice + "/" + "dataWithTime" + "/" + dayStamp + "/" + timeStamp + "/" + "humidity/" + "Value";
  Firebase.setString(gui,DBaddHs,valueUARTReceives[3]);

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
//==========Chương trình con=================//
void mainpage(){
  String s = FPSTR(MainPage);
  server.send(200,"text/html",s);
}

void get_IP(){
  String s = WiFi.localIP().toString();
  server.send(200,"text/html", s);
}
boolean read_EEPROM(){
  //Serial.println("Reading EEPROM...");
  if(EEPROM.read(0)!=0){
    ssid = "";
    pass = "";
    for (int i=0; i<32; ++i){
      ssid += char(EEPROM.read(i));
    }
    Serial.print("SSID: ");
    Serial.println(ssid);
    for (int i=32; i<96; ++i){
      pass += char(EEPROM.read(i));
    }
    Serial.print("PASSWORD: ");
    Serial.println(pass);
    ssid = ssid.c_str();
    pass = pass.c_str();
    return true;
  }else{
    Serial.println("Data wifi not found!");
    return false;
  }
}

void write_EEPROM(){
  ssid = server.arg("ssid");
  pass = server.arg("pass");
  Serial.println("Clear EEPROM!");
  for (int i = 0; i < 96; ++i) {
    EEPROM.write(i, 0);           
    delay(10);
  }
  for (int i = 0; i < ssid.length(); ++i) {
    EEPROM.write(i, ssid[i]);
  }
  for (int i = 0; i < pass.length(); ++i) {
    EEPROM.write(32 + i, pass[i]);
  }
  EEPROM.commit();
  Serial.println("Writed to EEPROM!");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("PASS: ");
  Serial.println(pass);
  String s = "Wifi configuration saved!";
  server.send(200, "text/html", s);
}
void restart_ESP(){
  ESP.restart();
}
void clear_EEPROM(){
  Serial.println("Clear EEPROM!");
  for (int i = 0; i < 512; ++i) {
    EEPROM.write(i, 0);           
    delay(10);
  }
  Serial.println("Clear EEPROM DONE!");
  EEPROM.commit();
  String s = "Device has been reset!";
  server.send(200,"text/html", s);
}
void checkbuttonreset(){
  int reading1 = digitalRead(touchPin1); //gia tri nut nhan
    if (reading1 != lastButtonState1) { //neu gia tri nut nhan hien tai ko bang gtri trc do
      lastDebounceTime1 = millis(); // thi set thoi gian bang tgian htai
    }
    if ((millis() - lastDebounceTime1) > debounceDelay) {
      if (reading1 != buttonState1) {
        buttonState1 = reading1;
        if (buttonState1 == HIGH) {
          ledState1 = !ledState1;
          int i = 0;
          while(i < 4){
            digitalWrite(5, ledState1);
            delay(1000);
            digitalWrite(5, !ledState1);
            delay(1000);
            i++;
          }
          digitalWrite(5, LOW);
          clear_EEPROM();
          Serial.println("Clear EEPROM thanh cong, chuan bi reset ESP");
          restart_ESP();
        }
      }
    }
    lastButtonState1 = reading1;
}
