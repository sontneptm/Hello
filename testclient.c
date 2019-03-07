#include <wiringPi.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <wiringPi.h>
#include <softTone.h>
#include <errno.h>
#include <signal.h>

#define TOTAL  15                                /* 학교종의 전체 계이름의 수 */
#define TCP_PORT  5000
#define MAXBUF 80
#define SPKR 11    /* GPIO25 */
#define LED 5

void* socket_init(void*);
void* socket_read(void*);
void* musicPlay(void*);
void* doAlarm(void*);
void* doFcm(void*);

int notes[] = {
 659, 659, 0, 659, 0, 523, 659, 0, 784, 0,0,0, 392, 0,0,0, 523, 0,0, 392, 0,0,330
};
char buf[MAXBUF];
char dummy[MAXBUF];
int read_len;
struct sockaddr_in server_addr;
int ssock;
int alarmTag=0;
int isAlarming = 0;
int isFcming = 0;
int callSign = 0;

pthread_t read_thread, musthr, alarm_thread, fcm_thread;
pthread_t init_thread;

int main(int argc, char* argv) {

	pthread_create(&init_thread, NULL, socket_init, NULL);
	pthread_join(init_thread, NULL);

	wiringPiSetup();

	pthread_create(&read_thread, NULL, socket_read, NULL);
	pthread_create(&musthr, NULL, musicPlay, NULL);
	pthread_create(&alarm_thread, NULL, doAlarm, NULL);
	pthread_create(&fcm_thread, NULL, doFcm, NULL);

	pthread_join(read_thread, NULL);
	pthread_join(musthr, NULL);
	pthread_join(alarm_thread, NULL);
	pthread_join(fcm_thread, NULL);

	pthread_cancel(read_thread);
	pthread_cancel(musthr);
	pthread_cancel(alarm_thread);
	pthread_cancel(fcm_thread);

	return 0;
}

void* socket_init(void* args) {
	int clen, i;

	if ((ssock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket( )");
	}

	//소켓이 접속할 주소 지정
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("192.168.1.4");
	server_addr.sin_port = htons(TCP_PORT);

	clen = sizeof(server_addr);

	//지정한 주소로 접속
	if (connect(ssock, (struct sockaddr *)&server_addr, clen) < 0) {
		perror("connect( )");
	}
}

void* socket_read(void* args) {
	while (1) {
		while(isAlarming==1 || isFcming==1){
		}

		callSign = 0;

		memset(buf, 0x00, MAXBUF);
		read_len = read(ssock, buf, MAXBUF);
		printf("buf = %s\n", buf);
		if(atoi(buf)==1){
			alarmTag = 1;
			callSign = 1;
			//ㅊ처루
		}
		if (atoi(buf) == 2) {
			alarmTag = 1;
			callSign = 2;
			//수빈
		}
		if (atoi(buf) == 3) {
			alarmTag = 1;
			callSign = 3;
			//승민
		}
		delay(100);
	}
}

void* musicPlay(void* args)
{
	int i;

	softToneCreate(SPKR);                     /* 톤 출력을 위한 GPIO 설정 */

	for (i = 0; i < TOTAL; ++i) {
		softToneWrite(SPKR, notes[i]);        /* 톤 출력 : 학교종 연주 */
		delay(120);                                       /* 음의 전체 길이만큼 출력되도록 대기 */
	}
}

void* doFcm(void* args){
	while (1) {
		while (alarmTag == 0) {
		}

		isFcming = 1;

		if (callSign == 1) {
			system("java -jar notificationCcw.jar");
		}
		else if (callSign == 2) {
			system("java -jar notificationKsb.jar");
		}
		else if (callSign == 3) {
			system("java -jar notificationJsm.jar");
		}
		else {
			break;
		}

		isFcming = 0;

		if (isAlarming == 0) {
			alarmTag = 0;
		}
	}
	delay(100);
}

void* doAlarm(void* args) {
	pinMode(LED, OUTPUT);             /* Pin의 출력 설정 */

	while (1) {
		while (alarmTag == 0) {
		}

		isAlarming = 1;

		if (callSign == 1) {
			system("omxplayer ccw_call.mp3");
		}
		else if (callSign == 2) {
			system("omxplayer ksb_call.mp3");
		}
		else if (callSign == 3) {
			system("omxplayer jsm_call.mp3");
		}
		else {
			break;
		}
	
		for (i = 0; i < 10; i++) {
			digitalWrite(LED, HIGH);           /* HIGH(1) 값을 출력 : LED 켜기 */
			delay(80);                               /* 1초(1000ms) 동안 대기 */
			digitalWrite(LED, LOW);             /* LOW(0) 값을 출력 : LED 끄기 */
			delay(80);
		}

		isAlarming = 0;

		if (isFcming == 0) {
			alarmTag = 0;
		}
	}
	delay(100);
}
 

