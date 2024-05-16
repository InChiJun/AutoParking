#include <LiquidCrystal_I2C.h>  // LiquidCrystal_I2C 헤더파일 호출
#include <Servo.h>
#include "WiFi.h"
#include <time.h>

#define ECHO 8  // 에코 핀
#define TRIG 9  // 트리거 핀
#define MOTOR 10

WiFiClient client;
Servo myServo;
LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD의 address 주소 및 크기 입력
char ssid[] = "IOTA_24G";
char pass[] = "kosta90009";
char curTime[30];
time_t now = time(nullptr);
struct tm *t;


void display();
void Open_door();
void Close_door();

void setup()  
{
    lcd.init(); 
    lcd.backlight(); 
    while(WiFi.begin(ssid, pass) != WL_CONNECTED){
    }
    //setSyncProvider(getArduinoDueTime);
    //configTime(9*3600, 0, "pool.ntp.org", "time.nist.gov");  // Timezone 9 for Korea
    while (!time(nullptr)) delay(500);
    myServo.attach(MOTOR);
    pinMode(ECHO, INPUT);
    pinMode(TRIG, OUTPUT);
    pinMode(MOTOR, OUTPUT);
}

void loop(){
    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    long duration = pulseIn(ECHO, HIGH);    // 펄스가 다시 돌아오는 데 걸린 시간
    long distance = duration * 0.034 / 2;   // 거리 계산
    if(distance > 1000){
        lcd.clear();
        display();
        delay(5000);
        Open_door();
        delay(3000);
        Close_door();
    }else{
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print(distance, 10); 
    }

    delay(1000);
}

void display(){
    lcd.setCursor(0, 0);
    //시간 서버에서 전송
    lcd.print("3:17");

}

String getTime(){
    /*
    struct tm timeinfo;
    t = localtime(&now);
    sprintf(curTime,"%02d:%02d", t->tm_hour, t->tm_min);
    return curTime;
    */
    while (!client.connect("google.co.kr", 80)) {}
    client.print("HEAD / HTTP/1.1\r\n\r\n");
    while(!client.available()) {}

    while(client.available()){
        if (client.read() == '\n') {    
            if (client.read() == 'D') {    
                if (client.read() == 'a') {    
                    if (client.read() == 't') {    
                        if (client.read() == 'e') {    
                            if (client.read() == ':') {    
                                client.read();
                                String s = client.readStringUntil('\r');
                                client.stop();
                                String timeData;
                                for(int i =16; i < 22; i++){
                                    timeData += s[i];
                                }
                                return timeData;
                            }
                        }
                    }
               }
            }
        }
    }
    
}

void Close_door(){
    myServo.writeMicroseconds(1450);
    delay(1000);
    myServo.writeMicroseconds(1500);
    delay(1000);
}

void Open_door(){
    myServo.writeMicroseconds(1570);
    delay(1000);
    myServo.writeMicroseconds(1500);
    delay(1000);
}