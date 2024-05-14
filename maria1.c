#include <stdio.h>          // 표준 입출력 라이브러리
#include <stdlib.h>         // 표준 라이브러리
#include <string.h>         // 문자열 처리 라이브러리
#include <unistd.h>         // 유닉스 표준 함수
#include <fcntl.h>          // 파일 제어 라이브러리
#include <termios.h>        // 터미널 제어 라이브러리
#include <pthread.h>        // POSIX 스레드 라이브러리
#include <stdbool.h>        // 불리언 타입 라이브러리
#include <mariadb/mysql.h>  // MySQL 데이터베이스 라이브러리
#include <time.h>           // 시간 처리 라이브러리

#define SERIAL_PORT "/dev/ttyACM0"    // 시리얼 포트 경로
#define BAUD_RATE B115200              // 시리얼 포트 보드레이트
#define DB_HOST "localhost"            // 데이터베이스 호스트 주소
#define DB_USER "root"                 // 데이터베이스 사용자 이름
#define DB_PASS "password"             // 데이터베이스 비밀번호
#define DB_NAME "database_name"        // 데이터베이스 이름
#define DB_TABLE "sensor_data"         // 데이터베이스 테이블 이름

int serial_fd;  // 시리얼 포트 파일 디스크립터

// 시리얼에서 데이터를 읽고 데이터베이스에 저장하는 스레드 함수
void *read_serial(void *arg) {
    time_t previous_time = time(NULL);  // 이전 시간
    while (true) {
        usleep(1000 * 1000);    // 1초 대기
        char buffer[3] = {0};   // 데이터 버퍼
        int ret = read(serial_fd, buffer, sizeof(buffer) - 1); // 시리얼에서 데이터 읽기

        // 데이터 읽기 성공 및 이전 읽기 후 3초 경과
        if ((ret > 0) && (time(NULL) - previous_time > 3)) {
            previous_time = time(NULL);    // 현재 시간 저장
            char date[20];  // 날짜 문자열 버퍼
            time_t now = time(NULL);   // 현재 시간 가져오기
            strftime(date, 20, "%Y-%m-%d %H:%M:%S", localtime(&now)); // 형식화된 시간 문자열 생성

            char sqlstring[256];    // SQL 쿼리 문자열
            sprintf(sqlstring, "INSERT INTO %s (value, timestamp) VALUES('%s','%s')", DB_TABLE, buffer, date); // SQL 쿼리 생성
            printf("%s\n", sqlstring); // 생성된 쿼리 출력

            usleep(1000 * 50);  // 50밀리초 대기

            MYSQL *con = mysql_init(NULL); // MySQL 연결 초기화

            // MySQL 연결 오류 처리
            if (con == NULL) {
                fprintf(stderr, "%s\n", mysql_error(con)); // 오류 메시지 출력
                continue;   // 다음 반복으로 진행
            }

            // MySQL 서버 연결 시도
            if (mysql_real_connect(con, DB_HOST, DB_USER, DB_PASS, DB_NAME, 0, NULL, 0) == NULL) {
                fprintf(stderr, "%s\n", mysql_error(con)); // 오류 메시지 출력
                mysql_close(con);   // MySQL 연결 종료
                continue;   // 다음 반복으로 진행
            }

            // SQL 쿼리 실행
            if (mysql_query(con, sqlstring)) {
                fprintf(stderr, "%s\n", mysql_error(con)); // 오류 메시지 출력
                mysql_close(con);   // MySQL 연결 종료
                continue;   // 다음 반복으로 진행
            }

            mysql_close(con);   // MySQL 연결 종료
        }
    }
}

// 시리얼 포트 설정 함수
void setup_serial_port() {
    serial_fd = open(SERIAL_PORT, O_RDWR); // 시리얼 포트 열기
    if (serial_fd < 0) {    // 시리얼 포트 열기 실패 시
        printf("error\n");  // 에러 메시지 출력
        return; // 함수 종료
    }

    struct termios new_termios; // 시리얼 포트 설정 구조체
    memset(&new_termios, 0, sizeof(new_termios)); // 구조체 초기화

    // 시리얼 포트 설정: 보드레이트, 8비트 데이터, 로컬 모드, 읽기 허용
    new_termios.c_cflag = BAUD_RATE | CS8 | CLOCAL | CREAD;
    new_termios.c_iflag = 0;    // 입력 플래그 초기화
    new_termios.c_oflag = 0;    // 출력 플래그 초기화
    new_termios.c_lflag = 0;    // 로컬 플래그 초기화 (비캐노니컬 모드)
    new_termios.c_cc[VTIME] = 0;   // 읽기 대기 시간 (0으로 설정하여 무한 대기)
    new_termios.c_cc[VMIN] = 1;    // 최소 읽기 바이트 수 설정

    tcflush(serial_fd, TCIOFLUSH); // 시리얼 버퍼 비우기
    tcsetattr(serial_fd, TCSANOW, &new_termios); // 시리얼 포트 설정 적용
    fcntl(serial_fd, F_SETFL, O_NONBLOCK); // 시리얼 포트를 논블록 모드로 설정
}

// 메인 함수
int main() {
    setup_serial_port();    // 시리얼 포트 설정 함수 호출
    pthread_t thread_idr;   // 읽기 스레드 ID

    pthread_create(&thread_idr, NULL, read_serial, NULL);   // 읽기 스레드 생성

    while (true) {
        usleep(1000 * 1000);    // 1초 대기
    }

    close(serial_fd);   // 시리얼 포트 닫기
    return 0;   // 프로그램 종료
}
