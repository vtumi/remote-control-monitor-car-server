/*
 * server.c为服务器端代码
 */

#include "server.h"


int main()
{
	setup();

	pthread_t recv_tid, send_tid;

	/*声明服务器地址和客户链接地址*/
	struct sockaddr_in servaddr, cliaddr;

	/*声明服务器监听套接字和客户端链接套接字*/
	int	listenfd, connfd;
	pid_t	childpid;

	socklen_t clilen;

	/*(1) 初始化监听套接字listenfd*/
	if ( (listenfd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
	{
		perror( "socket error" );
		exit( 1 );
	}

	int on = 1;
	setsockopt( listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );

	/*(2) 设置服务器sockaddr_in结构*/
	bzero( &servaddr, sizeof(servaddr) );

	servaddr.sin_family		= AF_INET;
	servaddr.sin_addr.s_addr	= htonl( INADDR_ANY ); /* 表明可接受任意IP地址 */
	servaddr.sin_port		= htons( PORT );

	/*(3) 绑定套接字和端口*/
	if ( bind( listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr) ) < 0 )
	{
		perror( "bind error" );
		exit( 1 );
	}

	/*(4) 监听客户请求*/
	if ( listen( listenfd, LISTENQ ) < 0 )
	{
		perror( "listen error" );
		exit( 1 );
	}

	/*(5) 接受客户请求*/
	for (;; )
	{
		clilen = sizeof(cliaddr);
		if ( (connfd = accept( listenfd, (struct sockaddr *) &cliaddr, &clilen ) ) < 0 )
		{
			perror( "accept error" );
			exit( 1 );
		}

		int flags = fcntl( connfd, F_GETFL, 0 );
		fcntl( connfd, F_SETFL, flags | O_NONBLOCK );

		pthread_cancel( recv_tid );
		pthread_join( recv_tid, NULL );
		pthread_cancel( send_tid );
		pthread_join( send_tid, NULL );
		/*创建子线程处理该客户链接接收消息*/
		if ( pthread_create( &recv_tid, NULL, recv_data, &connfd ) == -1 )
		{
			perror( "pthread create error" );
			exit( 1 );
		}
		/*创建子线程处理该客户链接发送消息*/
		if ( pthread_create( &send_tid, NULL, send_data, &connfd ) == -1 )
		{
			perror( "pthread create error" );
			exit( 1 );
		}
	}

	/*(6) 关闭监听套接字*/
	stop();
	close( listenfd );
}


/*处理接收客户端消息函数*/
void *recv_data( void *fd )
{
	int sockfd = *(int *) fd;
	/*接收超时*/
	struct timeval ti = { 1, 0 };
	setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO, &ti, sizeof(ti) );

	bool	isStop = true;
	int	n;
	char	buf[MAX_LINE];
	while ( (n = recv( sockfd, buf, MAX_LINE, 0 ) ) != 0 )
	{
		if ( n > 0 )
		{
			byte type = buf[0];
			switch ( type )
			{
			case 0x1:
			{
				short	x	= (short) ( (buf[1] << 8) + buf[2]);
				short	y	= (short) ( (buf[3] << 8) + buf[4]);
				short	r	= (short) ( (buf[5] << 8) + buf[6]);
				bool	isRun	= control( x, y, r );
				if ( isRun && isStop )
					isStop = false;
				else if ( !isRun && !isStop )
					isStop = true;
			}
			break;
			case 0x2:
			{
				short shift = (short) ( (buf[1] << 8) + buf[2]);
				ring( shift );
			}
			break;
			case 0xFF:
			{
				cvReleaseCapture( &capture );
				close( sockfd );
				exit( 0 );
			}
			break;
			}
		} else if ( !(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) && !isStop )
		{
			isStop = true;
			stop();
		}
	}

	if ( !isStop )
		stop();
	close( sockfd );
	return(NULL);
}


/*处理发送客户端消息函数*/
void *send_data( void *fd )
{
	int sockfd = *(int *) fd;

	while ( 1 )
	{
		if ( capture )
		{
			IplImage		*frame;
			byte			*outBuffer;
			long unsigned int	outLen;
			if ( (frame = cvQueryFrame( capture ) ) != NULL )
			{
				ipl2Jpeg( frame, &outBuffer, &outLen );
				int packet =
					outLen % BLOCK_SIZE == 0 ?
					outLen / BLOCK_SIZE : outLen / BLOCK_SIZE + 1;
				/* printf("total packet is:%d\n", packet); */
				int i;
				for ( i = 0; i < packet; i++ )
				{
					byte *index = outBuffer + i * BLOCK_SIZE;
					/* printf("current index is :%d\n", i * BLOCK_SIZE); */
					if ( i < packet - 1 )
					{
						send( sockfd, index, BLOCK_SIZE, 0 );
						/* printf("send a block\n"); */
					} else {
						send( sockfd, index, outLen - i * BLOCK_SIZE, 0 );
						/* printf("send last packet\n"); */
					}
				}

				if ( NULL != outBuffer )
				{
					free( outBuffer );
					outBuffer = NULL;
				}
			}
		}
	}
}


/*初始化*/
void setup()
{
	if ( wiringPiSetup() == -1 )
	{
		perror( "wiringPi setup error" );
		exit( 1 );
	}
	pinMode( OUT1, OUTPUT );
	pinMode( OUT2, OUTPUT );
	pinMode( OUT3, OUTPUT );
	pinMode( OUT4, OUTPUT );
	pinMode( OUT5, OUTPUT );
	pinMode( OUT6, OUTPUT );
	pinMode( OUT7, OUTPUT );
	pinMode( IN1, INPUT );
	pinMode( IN2, INPUT );
	pinMode( IN3, INPUT );
	softPwmCreate( ENA, 0, 100 );
	softPwmCreate( ENB, 0, 100 );

	digitalWrite( OUT1, LOW );
	digitalWrite( OUT2, LOW );
	digitalWrite( OUT3, LOW );
	digitalWrite( OUT4, LOW );
	digitalWrite( OUT5, LOW );
	digitalWrite( OUT6, LOW );
	digitalWrite( OUT7, LOW );

	capture = cvCreateCameraCapture( -1 );
	cvSetCaptureProperty( capture, CV_CAP_PROP_FRAME_WIDTH, 320 );
	cvSetCaptureProperty( capture, CV_CAP_PROP_FRAME_HEIGHT, 240 );
}


/*超声波测距*/
float distance()
{
	struct timeval	tv1;
	struct timeval	tv2;
	long		start, stop;
	float		dist;
	int		wait;

	digitalWrite( OUT5, HIGH );
	delayMicroseconds( 15 );
	digitalWrite( OUT5, LOW );

	wait = 0;
	while ( digitalRead( IN3 ) == LOW )
	{
		wait++;
		if ( wait > 10000 )
			break;
		delayMicroseconds( 1 );
	}
	gettimeofday( &tv1, NULL );

	wait = 0;
	while ( digitalRead( IN3 ) == HIGH )
	{
		wait++;
		if ( wait > 10000 )
			break;
		delayMicroseconds( 1 );
	}
	gettimeofday( &tv2, NULL );

	start	= tv1.tv_sec * 1000000 + tv1.tv_usec;
	stop	= tv2.tv_sec * 1000000 + tv2.tv_usec;

	dist = (float) (stop - start) / 1000000 * 34000 / 2;

	return(dist);
}


/*蜂鸣器*/
void ring( int shift )
{
	digitalWrite( OUT6, shift );
}


/*控制程序*/
bool control( short x, short y, short r )
{
	/* printf( "x:%d y:%d r:%d\n", x, y, r ); */
	if ( r == 0 )
	{
		stop();
		return(false);
	}
	int d, angle;
	d	= (int) sqrt( x * x + y * y );
	angle	= atan2( y, x ) * 180 / M_PI;
	if ( d > r )
		d = r;
	/* printf( "d:%d\n", d ); */
	int speed1, speed2, pwm1, pwm2;
	speed1 = (int) (d * 100 / r);
	if ( speed1 < 40 )
		speed1 = 40;

	if ( x < 0 )
	{
		if ( abs( angle ) - 90 > 45 )
		{
			left();
			return(true);
		}
		speed2	= (int) (speed1 * (180 - abs( angle ) ) / 90);
		pwm2	= speed1;
		pwm1	= speed2;
	} else if ( x > 0 )
	{
		if ( abs( angle ) < 45 )
		{
			right();
			return(true);
		}
		speed2	= (int) (speed1 * abs( angle ) / 90);
		pwm1	= speed1;
		pwm2	= speed2;
	}

	if ( y > 0 )
	{
		float dist = distance();
		/* printf("distance = %0.2f cm\n",dist); */
		int	block1	= digitalRead( IN1 );
		int	block2	= digitalRead( IN2 );
		/* printf( "block1:%d block2:%d\n", block1, block2); */
		if ( dist > 0 && dist < 50 )
		{
			if ( block1 == 0 && block2 == 0 )
			{
				left();
				return(false);
			} else if ( block1 == 0 )
			{
				right();
				return(true);
			} else if ( block2 == 0 )
			{
				left();
				return(true);
			} else{
				left();
				return(true);
			}
		}else if ( block1 == 0 )
		{
			right();
			return(true);
		} else if ( block2 == 0 )
		{
			left();
			return(true);
		}

		front( pwm1, pwm2 );
	} else if ( y < 0 )
		back( pwm1, pwm2 );
	else {
		stop();
		return(false);
	}
	return(true);
}


/*停止*/
void stop()
{
	digitalWrite( OUT1, LOW );
	digitalWrite( OUT2, LOW );
	digitalWrite( OUT3, LOW );
	digitalWrite( OUT4, LOW );
	softPwmWrite( ENA, 0 );
	softPwmWrite( ENB, 0 );
	delay( 10 );
	/* printf( "stop\n" ); */
}


/*前进*/
void front( int pwm1, int pwm2 )
{
	digitalWrite( OUT1, LOW );
	digitalWrite( OUT2, HIGH );
	digitalWrite( OUT3, LOW );
	digitalWrite( OUT4, HIGH );
	softPwmWrite( ENA, pwm1 );
	softPwmWrite( ENB, pwm2 );
	delay( 10 );
	/* printf( "front pwm1:%d pwm2:%d\n", pwm1, pwm2 ); */
}


/*后退*/
void back( int pwm1, int pwm2 )
{
	digitalWrite( OUT2, LOW );
	digitalWrite( OUT1, HIGH );
	digitalWrite( OUT4, LOW );
	digitalWrite( OUT3, HIGH );
	softPwmWrite( ENA, pwm1 );
	softPwmWrite( ENB, pwm2 );
	delay( 10 );
	/* printf( "back pwm1:%d pwm2:%d\n", pwm1, pwm2 ); */
}


/*向左*/
void left()
{
	digitalWrite( OUT1, LOW );
	digitalWrite( OUT2, HIGH );
	digitalWrite( OUT4, LOW );
	digitalWrite( OUT3, HIGH );
	softPwmWrite( ENA, 100 );
	softPwmWrite( ENB, 100 );
	delay( 10 );
	/* printf( "left\n" ); */
}


/*向右*/
void right()
{
	digitalWrite( OUT2, LOW );
	digitalWrite( OUT1, HIGH );
	digitalWrite( OUT3, LOW );
	digitalWrite( OUT4, HIGH );
	softPwmWrite( ENA, 100 );
	softPwmWrite( ENB, 100 );
	delay( 10 );
	/* printf( "right\n" ); */
}


