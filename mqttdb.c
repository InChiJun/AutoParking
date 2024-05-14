#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <mariadb/mysql.h> // MariaDB/MySQL 데이터베이스 라이브러리 포함

// MQTT 메시지 수신 콜백 함수
void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
    printf("Received message: %s\n", (char*)message->payload); // 수신된 MQTT 메세지 출력

    // MariaDB에 데이터 저장
    MYSQL *conn;
    conn = mysql_init(NULL);

    // 데이터베이스 초기화 오류시
    if (conn == NULL) 
    {
        fprintf(stderr, "Error: mysql_init() failed\n");
        return;
    }

    // 데이터베이스 연결
    if (mysql_real_connect(conn, "localhost", "username", "password", "database", 0, NULL, 0) == NULL)
    {
        fprintf(stderr, "Error: mysql_real_connect() failed\n"); // 연결 오류시 출력
        mysql_close(conn);
        return;
    }

    // 쿼리 문자열 생성 및 실행
    char query[100];
    sprintf(query, "INSERT INTO sensor_data (value) VALUES ('%s')", (char*)message->payload);
    if (mysql_query(conn, query) != 0)
    {
        fprintf(stderr, "Error: mysql_query() failed\n"); // 쿼리 실행 오류시 출력
    }

    mysql_close(conn); // 데이터베이스 연결 종료
}

int main()
{
    struct mosquitto *mosq = NULL;

    // Mosquitto 라이브러리 초기화
    mosquitto_lib_init();

    // Mosquitto 클라이언트 생성
    mosq = mosquitto_new(NULL, true, NULL);
    if (!mosq)
    {
        fprintf(stderr, "Error: Out of memory.\n"); // 메모리 할당 오류시 출력
        return 1;
    }

    // MQTT 브로커에 연결
    int rc = mosquitto_connect(mosq, "10.10.10.7", 1883, 60);
    if (rc != MOSQ_ERR_SUCCESS)
    {
        fprintf(stderr, "Error: Could not connect to MQTT broker.\n"); // MQtt 브로커 연결 오류시 출력
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    // 메시지 수신 콜백 함수 설정
    mosquitto_message_callback_set(mosq, on_message);

    // 특정 토픽 구독
    rc = mosquitto_subscribe(mosq, NULL, "sensor/1/data", 0);
    if (rc != MOSQ_ERR_SUCCESS)
    {
        fprintf(stderr, "Error: Could not subscribe to topic.\n");
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
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
