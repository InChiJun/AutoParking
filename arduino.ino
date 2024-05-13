// wifi-mqtt 설정
#include "WiFi.h"
#include "ArduinoMqttClient.h"

char ssid[] = "IOTA_24G";
char pass[] = "kosta90009";
char broker[] = "10.10.10.7";    // host 주소 IP 10.10.10.7
int port = 1883;

char s_topic[] = "sensor/";
char p_topic[] = "sensor/1/data";

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

// 주차공간 확인 센서 설정
#define MAX_PARKING_SPACES 2
const int CDS[MAX_PARKING_SPACES] = {A0, A1};

#define MAX_LED 2
const int led[MAX_LED] = {2, 3};

// 함수 전방 선언
void CDS_set();
void LED_set();

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

    // 조도 센서 setup
    CDS_set();
    analogReadResolution(14);

    // LED set
    LED_set();
}

void loop()
{
    byte message = 0;
    // 0 : 공석, 1 : 자리 있음
    for(int i = 0; i < MAX_PARKING_SPACES; ++i)
    {
        if(analogRead(CDS[i]) < 10000)
        {
            message |= (1 << i);
            digitalWrite(led[i], HIGH);
        }
        else
        {
            message &= (~(1 << i));
            digitalWrite(led[i], LOW);
        }
    }

    // pub
    mqttClient.beginMessage(p_topic);
    mqttClient.println(message, BIN);
    mqttClient.endMessage();

    delay(1000);
}

// 조도 센서 set함수
void CDS_set()
{
    for(int i = 0; i < MAX_PARKING_SPACES; ++i){
        pinMode(CDS[i], INPUT);
    }
}

// LED set함수
void LED_set()
{
    for(int i = 0; i < MAX_LED; ++i){
        pinMode(led[i], OUTPUT);
    }
}