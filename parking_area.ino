// wifi-mqtt 설정
#include "WiFi.h"
#include "ArduinoMqttClient.h"

char ssid[] = "IOTA_24G";
char pass[] = "kosta90009";
char broker[] = "10.10.10.7";    // host 주소 IP 10.10.10.7
int port = 1883;

char p_topic[] = "sensor/1/data";

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

// 주차공간 확인 센서 설정
#define MAX_PARKING_SPACES 2
const int CDS[MAX_PARKING_SPACES] = {A0, A1};

#define MAX_LED 2
const int led[MAX_LED] = {2, 3};

// 주차장 전역 변수
enum ParkingSpaceStatus {
  EMPTY,
  OCCUPIED
};

ParkingSpaceStatus lastParkingSpaces[MAX_PARKING_SPACES];

// 함수 전방 선언
void CDS_set();
void LED_set();
void Parking_set();

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

    // 주차장 초기 설정
    Parking_set();
}

void loop()
{
    // O : 주차된 차량 있음, X : 주차된 차량 없음
    for(int i = 0; i < MAX_PARKING_SPACES; ++i)
    {
         // 현재 조도 센서 값에 따라 주차 공간의 상태를 결정
        ParkingSpaceStatus currentStatus = analogRead(CDS[i]) < 10000 ? OCCUPIED : EMPTY;

        // 현재 상태와 이전 상태가 다르면, 상태 변경 처리
        if(currentStatus != lastParkingSpaces[i])
        {
            // LED 상태 변경
            digitalWrite(led[i], currentStatus == OCCUPIED ? HIGH : LOW);

            // MQTT 메시지 전송
            String message = "Parking Space " + String(i) + ":" + (currentStatus == OCCUPIED ? "O" : "X");
            mqttClient.beginMessage(p_topic);
            mqttClient.print(message);
            mqttClient.endMessage();

            // 이전 상태 업데이트
            lastParkingSpaces[i] = currentStatus;
        }
    }

    delay(1000);
}

// 조도 센서 set함수
void CDS_set()
{
    for(int i = 0; i < MAX_PARKING_SPACES; ++i) {
        pinMode(CDS[i], INPUT);
    }
}

// LED set함수
void LED_set()
{
    for(int i = 0; i < MAX_LED; ++i) {
        pinMode(led[i], OUTPUT);
    }
}

// Parking_set함수
void Parking_set()
{
    for(int i = 0; i < MAX_PARKING_SPACES; ++i) {
        lastParkingSpaces[i] = EMPTY; // 마지막 상태도 EMPTY로 초기화
    }
}