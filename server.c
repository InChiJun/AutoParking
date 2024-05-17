#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <mariadb/mysql.h> // MariaDB/MySQL 데이터베이스 라이브러리 포함
#include <time.h>
#include <string.h>

// 전역변수
MYSQL *con;

// 함수 전방 선언
void db_set();
void on_connect(struct mosquitto *mosq, void *obj, int reason_code);
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);

int main()
{
    db_set();

    struct mosquitto *mosq = NULL;

    // Mosquitto 라이브러리 초기화
    mosquitto_lib_init();

    // Mosquitto 클라이언트 생성
    mosq = mosquitto_new(NULL, true, NULL);
    if (mosq == NULL)
    {
        fprintf(stderr, "Error: Out of memory.\n"); // 메모리 할당 오류시 출력
        return 1;
    }

    // MQTT 브로커에 연결
    int rc = mosquitto_connect(mosq, "localhost", 1883, 60);
    if (rc != MOSQ_ERR_SUCCESS)
    {
        fprintf(stderr, "Error: Could not connect to MQTT broker.\n"); // MQtt 브로커 연결 오류시 출력
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    // 콜백 함수 설정
    mosquitto_connect_callback_set(mosq, on_connect);   // 연결
    mosquitto_message_callback_set(mosq, on_message);   // 메시지 수신

    if(mosquitto_connect(mosq, "localhost", 1883, 60))
    {
        fprintf(stderr, "Unable to connect.\n");
        exit(-1);
    }

    // 메시지 루프 시작
    rc = mosquitto_loop_forever(mosq, -1, 1);
    if (rc != MOSQ_ERR_SUCCESS)
    {
        fprintf(stderr, "Error: Could not start message loop.\n"); // 메시지 루프 시작 오류 출력
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    return 0;
}

// 함수 정의

void db_set()
{
    con = mysql_init(NULL);

    if (con == NULL)
    {
        fprintf(stderr, "%s\n", mysql_error(con));
        exit(1);
    }

    if (mysql_real_connect(con, "localhost", "root", "ubuntu",
                           NULL, 0, NULL, 0) == NULL)
    {
        fprintf(stderr, "%s\n", mysql_error(con));
        mysql_close(con);
        exit(1);
    }

    mysql_select_db(con, "raspi_db");
}

void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
    // "sensor/1/data" 구독 추가
    mosquitto_subscribe(mosq, NULL, "sensor/1/data", 0);
    // "sensor/2/data" 구독 추가
    mosquitto_subscribe(mosq, NULL, "sensor/2/data", 0);
}

// MQTT 메시지 수신 콜백 함수
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
    printf("Received message: %s\n", (char*)message->payload); // 수신된 MQTT 메세지 출력

    if (message->payloadlen)
    {
        // 메시지 파싱
        int parkingNum = 0; // 주차 공간 번호를 저장할 배열
        char status = 0; // 주차 공간 상태를 저장할 배열
        sscanf((char *)message->payload, "Parking Space %d: %c", &parkingNum, &status);

        // 현재 시간을 얻음
        time_t now = time(NULL); // 현재 시간은 time_t 타입으로 반환
        struct tm *t = localtime(&now); // 현지 시간대에 해당하는 시간으로 변환

        char dateTime[64]; // 시간 데이터 문자 배열 생성
        strftime(dateTime, sizeof(dateTime), "%Y-%m-%d %H:%M:%S", t); // struct tm 구조체를 지정된 형식의 문자열로 변환 -> "YYYY-MM-DD HH:MM:SS"

        // "sensor/2/data" 주제로 메시지가 들어오면 응답 데이터 전송
        if(strcmp(message->topic, "sensor/2/data") == 0){
            // 여기서 "response/data"는 응답으로 전송할 데이터의 주제
            // 쿼리 문자열 생성 및 실행
            char query[256];
            MYSQL_ROW row;
            sprintf(query, "SELECT parking_num FROM parking_data WHERE status = 'X' ORDER BY parking_num ASC LIMIT 1;");

            if (mysql_query(con, query) != 0) // 쿼리문이 실패했다면
            {
                fprintf(stderr, "Error: mysql_query() failed\n"); // 쿼리 실행 오류시 출력
            }
            else // 쿼리문이 성공했다면
            {
                MYSQL_RES *result = mysql_use_result(con);
                row = mysql_fetch_row(result);
                if(row) // 결과가 존재하면
                {
                    char min_parking_num[10] = {0}; // parking_num 값을 저장하기 위한 충분한 크기의 문자 배열 선언
                    strcpy(min_parking_num, row[0]); // row[0]의 값을 min_parking_num에 복사
                    // MQTT 주제로 전송
                    char payload[50];
                    sprintf(payload, "Parking Number: %s, Time: %s", min_parking_num, dateTime);
                    mosquitto_publish(mosq, NULL, "server/1/parking_data", strlen(payload), payload, 0, false);
                }
                else
                {
                    // MQTT 주제로 전송
                    char payload[50];
                    sprintf(payload, "Parking Number: -1, Time: %s", dateTime);
                    mosquitto_publish(mosq, NULL, "server/1/parking_data", strlen(payload), payload, 0, false);
                }
                mysql_free_result(result); // 결과 집합 해제
            }
        }
        else if(strcmp(message->topic, "sensor/1/data") == 0)
        {
            // 쿼리 문자열 생성 및 실행
            char query[256];
            MYSQL_ROW row;
            sprintf(query, "SELECT * FROM parking_data WHERE parking_num = '%d';", parkingNum);

            if (mysql_query(con, query) != 0) // 쿼리문이 실패했다면
            {
                fprintf(stderr, "Error: mysql_query() failed\n"); // 쿼리 실행 오류시 출력
            }
            else // 쿼리문이 성공했다면
            {
                MYSQL_RES *result = mysql_use_result(con);
                row = mysql_fetch_row(result);
                mysql_free_result(result); // 결과 집합 해제
                if(!row){ // 새로운 값을 넣는다면
                    sprintf(query, "INSERT INTO parking_data values ('%d','%c', '%s');", parkingNum, status, dateTime);
                    mysql_query(con, query);
                }
                else{ // 기존의 값을 수정한다면
                    sprintf(query, "UPDATE parking_data SET status='%c', collect_time='%s' WHERE parking_num='%d';", status, dateTime, parkingNum);
                    if (mysql_query(con, query))
                    {
                        fprintf(stderr, "Error: mysql_query() failed\n");
                        fprintf(stderr, "Error: %s\n", mysql_error(con));
                    }
                }
            }
        }
    }
    else
    {
        printf("%s (null)\n", message->topic);
    }
}