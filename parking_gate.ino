#include <LiquidCrystal_I2C.h>  // LiquidCrystal_I2C 헤더파일 호출
#include <Servo.h>
#include "WiFi.h"
#include "ArduinoMqttClient.h"

#define ECHO 8  // 에코 핀
#define TRIG 9  // 트리거 핀
#define MOTOR 10    // 서보 모터 핀

// 서보 모터
Servo myServo;

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD의 address 주소 및 크기 입력

// wifi 설정
char ssid[] = "IOTA_24G";
char pass[] = "kosta90009";
char broker[] = "10.10.10.7";    // host 주소 IP 10.10.10.7
int port = 1883;

char s_topic[] = "server/1/parking_data";
char p_topic[] = "sensor/2/data";

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

// 함수 전방 선언
long UltraSonic();
void display();
void Open_door();
void Close_door();
void wifi_mqtt_pub();
void wifi_mqtt_sub();

void setup()  
{
    Serial.begin(115200);

    while(WiFi.begin(ssid, pass) != WL_CONNECTED)
    {
        Serial.print(".");
        delay(5000);
    }
    delay(1000);
    
    while(mqttClient.connect(broker, port) == 0)
    {
        Serial.println(mqttClient.connectError());

        WiFi.disconnect();
        while(WiFi.begin(ssid, pass) != WL_CONNECTED)
        {
            Serial.print(".");
            delay(5000);
        }
        delay(1000);
    }
    Serial.println("mqtt ok");

    // sub
    mqttClient.subscribe(s_topic);
    
    // lcd 초기 설정
    lcd.init(); 
    lcd.backlight(); 
    
    // 서보 모터 초기 설정
    myServo.attach(MOTOR);

    // 핀모드 설정
    pinMode(ECHO, INPUT);
    pinMode(TRIG, OUTPUT);
    pinMode(MOTOR, OUTPUT);
}

void loop()
{
    if(UltraSonic() < 30)   // 차량이 접근하면
    {
        Close_door();
        wifi_mqtt_pub();
        wifi_mqtt_sub();
        Open_door();
        delay(5000);
        lcd.clear();
    }
    else    // 차량이 떠나면
    {
        Close_door();
    }

    delay(1000);
}

long UltraSonic()
{
    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    long duration = pulseIn(ECHO, HIGH);    // 펄스가 다시 돌아오는 데 걸린 시간
    long distance = duration * 0.034 / 2;   // 거리 계산

    return distance;
}

void display(String parkingNumber, String time){
    lcd.clear();
    lcd.setCursor(0, 0);
    if(parkingNumber == "-1")
    {
        lcd.print("No Parking");
        lcd.setCursor(0, 1);
        lcd.print("Space Available");
    }
    else
    {
        lcd.print("Time: " + time);
        lcd.setCursor(0, 1);
        lcd.print("Number: " + parkingNumber);
    }
}


void Close_door(){
    myServo.write(90);
    delay(1000);
}

void Open_door(){
    myServo.write(0);
    delay(1000);
}

void wifi_mqtt_pub()
{
    mqttClient.beginMessage(p_topic);
    mqttClient.print('1');
    mqttClient.endMessage();
}

void wifi_mqtt_sub()
{
    // 메시지가 도착할 때까지 대기
    int messageSize = 0;
    while (messageSize == 0)
    {
        messageSize = mqttClient.parseMessage();
    }

    // 메시지가 도착하면 토픽을 출력
    Serial.println(mqttClient.messageTopic());

    // MQTT 클라이언트의 메시지 버퍼에서 데이터를 읽을 수 있는 동안 반복
    while(mqttClient.available())
    {
        // 메시지의 내용을 문자열로 읽어서 출력
        String message = mqttClient.readString();
        Serial.println(message);

        // 메시지를 파싱하여 주차 번호와 시간을 추출
        int commaIndex = message.indexOf(',');
        String parkingNumber = message.substring(16, commaIndex); // "Parking Number: "를 제외하고 추출
        String time = message.substring(commaIndex + 19); // ", Time: "를 제외하고 추출

        // LCD에 출력
        display(parkingNumber, time);
    }
}