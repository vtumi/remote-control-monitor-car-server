/*
 * server.h 包含该tcp/ip套接字编程所需要的基本头文件
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>

#include <wiringPi.h>
#include <softPwm.h>
#include <opencv2/highgui/highgui_c.h>
#include <opencv/cxcore.h>
#include <opencv/cv.h>

#include "jpegutil.h"

typedef enum {false=0,true=1} bool ;
typedef unsigned char		byte;

const int	PORT		= 2222;
const int	LISTENQ		= 5;
const int	MAX_LINE	= 7;
const int	BLOCK_SIZE	= 4096;

const int	OUT1	= 22; //左前轮
const int	OUT2	= 23; //左后轮
const int	OUT3	= 24; //右前轮
const int	OUT4	= 25; //右后轮
const int	OUT5	= 28; //超声波测距发送
const int	OUT6	= 4; //蜂鸣器高电平
const int	OUT7	= 5; //蜂鸣器低电平

const int	IN1	= 7;  //左边红外线避障接收
const int	IN2	= 27; //右边红外线避障接收
const int	IN3	= 29; //超声波测距接收

const int	ENA	= 2; //左边马达速度控制
const int	ENB	= 3; //右边马达速度控制

CvCapture *capture;


void *recv_data( void *fd );


void *send_data( void *fd );


void setup();


void restart();


float distance();


void ring(int shift);


bool control( short x, short y, short r );


void stop();


void front();


void back();


void left();


void right();
