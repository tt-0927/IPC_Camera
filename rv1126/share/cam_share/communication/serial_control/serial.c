#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<termios.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>


#include<stdlib.h>
#include "serial.h"
//#include "nslog.h"

#if 1
#define nslog(nLevel, format, args...) \
printf(format,##args)
#endif
Uart_Param_t pUartCfg={
	.speed=9600,
	.stop=1,
	.bits=8,
	.event='N',
	.comport=2
};


Init_Param_t initCfg;
int g_serial_flag;
Serial_PanZoom_t g_position[4];

Serial_set_t g_serial_info;
static  pthread_mutex_t g_serial_mutex = PTHREAD_MUTEX_INITIALIZER;
#define POSITTION_TO_4BYTE(x) ((x >> 12) & 0x000f), ((x >> 8) & 0x000f), ((x >> 4) & 0x000f), (x & 0x000f)
#define ZOOMPOSITION_TO_4BYTE(x) ((x >> 12) & 0x000f), ((x >> 8) & 0x000f), ((x >> 4) & 0x000f), (x & 0x000f)

int serial_get_fd(Comport_Type_t type)
{
	int fd = -1;
	if (TTY0_PORT == type)
	{
		pthread_mutex_lock(&(initCfg.mutex_tty0));
		fd = initCfg.tty0Handel;
		pthread_mutex_unlock(&(initCfg.mutex_tty0));
	}
	else if (TTY1_PORT == type)
	{
		pthread_mutex_lock(&(initCfg.mutex_tty1));
		fd = initCfg.tty1Handel;
		pthread_mutex_unlock(&(initCfg.mutex_tty1));
	
	}
	else if (TTY2_PORT == type)
	{
		pthread_mutex_lock(&(initCfg.mutex_tty2));
		fd = initCfg.tty2Handel;
		pthread_mutex_unlock(&(initCfg.mutex_tty2));
	}
	else if (TTY3_PORT == type)
	{
		pthread_mutex_lock(&(initCfg.mutex_tty3));
		fd = initCfg.tty3Handel;
		pthread_mutex_unlock(&(initCfg.mutex_tty3));
	}
	else if (TTY0USB_PORT == type)
	{
		pthread_mutex_lock(&(initCfg.mutex_tty4));
		fd = initCfg.tty4Handel;
		pthread_mutex_unlock(&(initCfg.mutex_tty4));
	}
	
	
	return fd;
}

static int serial_set_fd(int fd, Comport_Type_t type)
{
	if (TTY0_PORT == type)
	{
		pthread_mutex_lock(&(initCfg.mutex_tty0));
		initCfg.tty0Handel = fd;
		pthread_mutex_unlock(&(initCfg.mutex_tty0));
	}
	else if (TTY1_PORT == type)
	{
		pthread_mutex_lock(&(initCfg.mutex_tty1));
		initCfg.tty1Handel = fd;
		pthread_mutex_unlock(&(initCfg.mutex_tty1));
	
	}
	else if (TTY2_PORT == type)
	{
		pthread_mutex_lock(&(initCfg.mutex_tty2));
		initCfg.tty2Handel = fd;
		pthread_mutex_unlock(&(initCfg.mutex_tty2));
	}
	else if (TTY3_PORT == type)
	{
		pthread_mutex_lock(&(initCfg.mutex_tty3));
		initCfg.tty3Handel = fd;
		pthread_mutex_unlock(&(initCfg.mutex_tty3));
	}
	else if (TTY0USB_PORT == type)
	{
		pthread_mutex_lock(&(initCfg.mutex_tty4));
		initCfg.tty4Handel = fd;
		pthread_mutex_unlock(&(initCfg.mutex_tty4));
	}
	
	return 0;
}

static void serial_set_panZoom(char *buff, int index, int type)
{
	printf("\n%02x %02x %02x %02x\n", buff[2],buff[3], buff[4], buff[5]);
	{
		pthread_mutex_lock(&(g_position[index].mutex));
		if (type == PAN)
		{
			g_position[index].posit_pan = ((buff[2] & 0x0f) << 12) + ((buff[3] & 0x0f) << 8) + ((buff[4] & 0x0f) << 4) + (buff[5] & 0x0f);
			g_position[index].posit_pan = ((g_position[index].posit_pan >> 15) & 1) ? g_position[index].posit_pan | ((-1 >> 16) << 16) : g_position[index].posit_pan & 0xffff;
		
			g_position[index].posit_tilt = ((buff[6] & 0x0f) << 12) + ((buff[7] & 0x0f) << 8) + ((buff[8] & 0x0f) << 4) + (buff[9] & 0x0f);
			g_position[index].posit_tilt = ((g_position[index].posit_tilt >> 15) & 1) ? g_position[index].posit_tilt | ((-1 >> 16) << 16) : g_position[index].posit_tilt & 0xffff;
		}
		else
		{
			g_position[index].coefficient = ((buff[2] & 0x0f) << 12) + ((buff[3] & 0x0f) << 8) + ((buff[4] & 0x0f) << 4) + (buff[5] & 0x0f);
			g_position[index].coefficient = ((g_position[index].coefficient >> 15) & 1) ? g_position[index].coefficient | ((-1 >> 16) << 16) : g_position[index].coefficient & 0xffff;

		}
		pthread_mutex_unlock(&(g_position[index].mutex));
	}
	return;
}


static void serial_get_pan(Serial_Position_t *position, int port)
{
	pthread_mutex_lock(&(g_position[port].mutex));
	position->posit_pan = g_position[port].posit_pan;
	position->posit_tilt = g_position[port].posit_tilt;
	position->coefficient = g_position[port].coefficient;
	pthread_mutex_unlock(&(g_position[port].mutex));
	return;
}

static int cmd_move_braocast(Comport_Type_t port)
{
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_do_home fd=[%d] is err!!!\n", fd);
		return -1;
	}

	char buffer[4] = {0x88, 0x30, 0x01, 0xFF};
	ret = write(fd, buffer, 4);
	if (4 != ret)
	{
		nslog(NS_ERROR, "cmd_do_home send data is err!!!\n");
		return -1;
	}

	return 0;
}
#if 0
static int cmd_auto_focus(Comport_Type_t port, char cmd)
{
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_do_home fd=[%d] is err!!!\n", fd);
		return -1;
	}
	
	char buffer[6] = {cmd, 0x01, 0x04, 0x38, 0x02, 0xFF};
	ret = write(fd, buffer, 6);
	if (6 != ret)
	{
		nslog(NS_ERROR, "cmd_auto_focus send data is err!!!\n");
		return -1;
	}

	return 0;
}
#endif

static int cmd_move_home(Comport_Type_t port, char cmd)
{
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_do_home fd=[%d] is err!!!\n", fd);
		return -1;
	}
	
	char buffer[5] = {cmd, 0x01, 0x06, 0x04, 0xFF};
	ret = write(fd, buffer, 5);
	if (5 != ret)
	{
		nslog(NS_ERROR, "cmd_do_home send data is err!!!\n");
		return -1;
	}

	return 0;
}

static int cmd_move_far(Comport_Type_t port, int speed, char cmd)
{
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_do_home fd=[%d] is err!!!\n", fd);
		return -1;
	}

	if ((0 >= speed) || (speed > 7))
	{
		speed = 0x02;
	}
	else
	{
		speed = speed * 2;
	}
	
	char buffer[6] = {cmd, 0x01, 0x04, 0x07, speed, 0xFF};
	ret = write(fd, buffer, 6);
	if (6 != ret)
	{
		nslog(NS_ERROR, "cmd_do_far send data is err!!!\n");
		return -1;
	}

	return 0;
}

static int cmd_move_near(Comport_Type_t port, int speed, char cmd)
{
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_do_home fd=[%d] is err!!!\n", fd);
		return -1;
	}
	
	if ((0 >= speed) || (speed > 7))
	{
		speed = 0x03;
	}
	else
	{
		speed = speed * 3;
	}
	
	char buffer[6] = {cmd, 0x01, 0x04, 0x07, speed, 0xFF};
	ret = write(fd, buffer, 6);
	if (6 != ret)
	{
		nslog(NS_ERROR, "cmd_do_near send data is err!!!\n");
		return -1;
	}

	return 0;
}


static int cmd_move_stopZoom(Comport_Type_t port, char cmd)
{
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_do_home fd=[%d] is err!!!\n", fd);
		return -1;
	}
	

	char buffer[6] = {cmd, 0x01, 0x04, 0x07, 0x00, 0xFF};
	ret = write(fd, buffer, 6);
	if (6 != ret)
	{
		nslog(NS_ERROR, "cmd_do_stopZoom send data is err!!!\n");
		return -1;
	}

	return 0;
}



int cmd_move_direZoom(int zoomPosition, Comport_Type_t port, int num)
{
	if (zoomPosition < CLIENT_CAMERA_ZOOM_MIN)
	{
		zoomPosition = CLIENT_CAMERA_ZOOM_MIN;
	}
	if (zoomPosition > CLIENT_CAMERA_ZOOM_MAX)
	{
		zoomPosition = CLIENT_CAMERA_ZOOM_MAX;
	}

	char cmd = '0';
	cmd = 0x80 | num;
	
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_move_direZoom fd=[%d] is err!!!\n", fd);
		return -1;
	}
	
	char buffer[] = {cmd, 0x01, 0x04, 0x47, ZOOMPOSITION_TO_4BYTE(zoomPosition), 0xFF};
	ret = write(fd, buffer, sizeof(buffer));
	if (sizeof(buffer) != ret)
	{
		nslog(NS_ERROR, "cmd_do_direZoom send data is err!!!\n");
		return -1;
	}

	return 0;
}


static int cmd_move_up(Comport_Type_t port, int speed, char cmd)
{
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_move_up fd=[%d] is err!!!\n", fd);
		return -1;
	}

	if ((speed < 0) || (speed > 0x18))
	{
		speed = 0x08;
	}

	char buffer[] = {cmd, 0x01,0x06, 0x01, 0x00, speed, 0x03, 0x01, 0xFF};
	ret = write(fd, buffer, sizeof(buffer));
	if (sizeof(buffer) != ret)
	{
		nslog(NS_ERROR, "cmd_move_up send data is err!!!\n");
		return -1;
	}

	return 0;
}

static int cmd_move_down(Comport_Type_t port, int speed, char cmd)
{
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_move_down fd=[%d] is err!!!\n", fd);
		return -1;
	}

	if ((speed < 0) || (speed > 0x18))
	{
		speed = 0x08;
	}

	char buffer[] = {cmd, 0x01,0x06, 0x01, 0x00, speed, 0x03, 0x02, 0xFF};
	ret = write(fd, buffer, sizeof(buffer));
	if (sizeof(buffer) != ret)
	{
		nslog(NS_ERROR, "cmd_move_up send data is err!!!\n");
		return -1;
	}

	return 0;
}

static int cmd_move_left(Comport_Type_t port, int speed, char cmd)
{
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_move_left fd=[%d] is err!!!\n", fd);
		return -1;
	}

	if ((speed < 0) || (speed > 0x14))
	{
		speed = 0x08;
	}
	
	char buffer[] = {cmd, 0x01,0x06, 0x01, speed, 0x00, 0x01, 0x03, 0xFF};
	ret = write(fd, buffer, sizeof(buffer));
	if (sizeof(buffer) != ret)
	{
		nslog(NS_ERROR, "cmd_move_left send data is err!!!\n");
		return -1;
	}


	return 0;
}

static int cmd_move_right(Comport_Type_t port, int speed, char cmd)
{
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_move_right fd=[%d] is err!!!\n", fd);
		return -1;
	}

	if ((speed < 0) || (speed > 0x14))
	{
		speed = 0x08;
	}
	
	char buffer[] = {cmd, 0x01,0x06, 0x01, speed, 0x00, 0x02, 0x03, 0xFF};
	ret = write(fd, buffer, sizeof(buffer));
	if (sizeof(buffer) != ret)
	{
		nslog(NS_ERROR, "cmd_move_left send data is err!!!\n");
		return -1;
	}
	printf("0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x 0x%02x cam move right!\r\n", buffer[0],buffer[1],buffer[2],buffer[3],\
			buffer[4],buffer[5],buffer[6],buffer[7],buffer[8]);//debug by kg 20180123
	return 0;
}

int cmd_move_Direpo(int position_pan, int position_tilt, int move_speed_pan, int move_speed_tilt, Comport_Type_t port, int num)
{
	if (position_pan < CLIENT_CAMERA_POSITTOIN_PAN_MIN)
	{
		position_pan = CLIENT_CAMERA_POSITTOIN_PAN_MIN;
	}
	if (position_pan > CLIENT_CAMERA_POSITTOIN_PAN_MAX)
	{
		position_pan = CLIENT_CAMERA_POSITTOIN_PAN_MAX;
	}
	if (position_tilt < CLIENT_CAMERA_POSITTOIN_TILT_MIN)
	{
		position_tilt = CLIENT_CAMERA_POSITTOIN_TILT_MIN;
	}
	if (position_tilt > CLIENT_CAMERA_POSITTOIN_TILT_MAX)
	{
		position_tilt = CLIENT_CAMERA_POSITTOIN_TILT_MAX;
	}

	if (move_speed_pan < SPEED_PAN_MIN)
	{
		move_speed_pan = SPEED_PAN_MIN;
	}
	if (move_speed_pan > SPEED_PAN_MAX)
	{
		move_speed_pan = SPEED_PAN_MAX;
	}

	
	if (move_speed_tilt < SPEED_TILT_MIN)
	{
		move_speed_tilt = SPEED_TILT_MIN;
	}
	if (move_speed_tilt > SPEED_TILT_MAX)
	{
		move_speed_tilt = SPEED_TILT_MAX;
	}

	char cmd = '0';
	cmd = 0x80 | num;
	
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_move_Direpo fd=[%d] is err!!!\n", fd);
		return -1;
	}

	if ((0 == move_speed_pan) || (0 == move_speed_tilt))
	{
		move_speed_pan = 0x18;
		move_speed_tilt = 0x14;
	}

	nslog(NS_INFO, "&&&&&&&&&&&&&&&&&&&&the speed is %02x %02x\n", move_speed_pan, move_speed_tilt);
	char buffer[] = {cmd, 0x01, 0x06, 0x02, move_speed_pan, move_speed_tilt, POSITTION_TO_4BYTE(position_pan), POSITTION_TO_4BYTE(position_tilt), 0xFF };
	ret = write(fd, buffer, sizeof(buffer));
	if (sizeof(buffer) != ret)
	{
		nslog(NS_ERROR, "cmd_move_left send data is err!!!\n");
		return -1;
	}

	return 0;
}

static int cmd_move_stopTurn(Comport_Type_t port, char cmd)
{
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_move_stopTurn fd=[%d] is err!!!\n", fd);
		return -1;
	}

	char buffer[] = {cmd, 0x01,0x06, 0x01, 0x08, 0x08, 0x03, 0x03, 0xFF};
	ret = write(fd, buffer, sizeof(buffer));
	if (sizeof(buffer) != ret)
	{
		nslog(NS_ERROR, "cmd_move_left send data is err!!!\n");
	}
	
	return 0;
}

static int cmd_set_presite(int index, Comport_Type_t port, int num)
{
	if (index > 254 || index < 0)
	{
		nslog(NS_ERROR, "cmd_set_position is err!!\n");
		return -1;
	}

	char cmd = '0';
	cmd = 0x80 | num;
	
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_set_position fd=[%d] is err!!!\n", fd);
		return -1;
	}

	nslog(NS_ERROR, "the cmd_set_presite ***********cmd=%c\n", cmd);
	char buffer[] = {cmd, 0x01,0x04, 0x3F, 0x01, index, 0xFF};
	ret = write(fd, buffer, sizeof(buffer));
	if (sizeof(buffer) != ret)
	{
		nslog(NS_ERROR, "cmd_move_left send data is err!!!\n");
		return -1;
	}

	return 0;
}

static int cmd_reset_presite(int index, Comport_Type_t port, int num)
{
	if (index > 254 || index < 0)
	{
		nslog(NS_ERROR, "cmd_set_position is err!!\n");
		return -1;
	}

	char cmd = '0';
	cmd = 0x80 | num;
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_set_position fd=[%d] is err!!!\n", fd);
		return -1;
	}
	
	char buffer[] = {cmd, 0x01,0x04, 0x3F, 0x00, index, 0xFF};
	ret = write(fd, buffer, sizeof(buffer));
	if (sizeof(buffer) != ret)
	{
		nslog(NS_ERROR, "cmd_move_left send data is err!!!\n");
		return -1;
	}

	return 0;
}

static int cmd_recall_presite(int index, Comport_Type_t port, int num)
{
	if (index > 254 || index < 0)
	{
		nslog(NS_ERROR, "cmd_set_position is err!!\n");
		return -1;
	}

	char cmd = '0';
	cmd = 0x80 | num;
	
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_set_position fd=[%d] is err!!!\n", fd);
		return -1;
	}
	
	char buffer[] = {cmd, 0x01,0x04, 0x3F, 0x02, index, 0xFF};
	ret = write(fd, buffer, sizeof(buffer));
	if (sizeof(buffer) != ret)
	{
		nslog(NS_ERROR, "cmd_move_left send data is err!!!\n");
		return -1;
	}

	return 0;
}

int serial_move_cmd(Move_Type_t type, Comport_Type_t port, int speed, int num)
{
	int ret  = 0;

	char cmd = '0';
	cmd = 0x80 | num;
	
	switch (type)
	{
		case GOHOME:
			ret = cmd_move_home(port, cmd);
			break;
		case GONEAR:
			ret = cmd_move_far(port, speed, cmd);
			break;
		case GOFAR:
			ret = cmd_move_near(port, speed, cmd);
			break;
		case STOPZ:
			ret = cmd_move_stopZoom(port, cmd);
			break;
		case GOUP:
			ret = cmd_move_up(port, speed, cmd);
			break;
		case GODOWN:
			ret = cmd_move_down(port, speed, cmd);
			break;
		case GOLEFT:
			ret = cmd_move_left(port, speed, cmd);
			break;
		case GORIGHT:
			ret = cmd_move_right(port, speed, cmd);
			break;
		case STOPTURN:
			ret = cmd_move_stopTurn(port, cmd);
			break;
			
		default:
			ret = cmd_move_braocast(port);
			break;

	}
	
	return ret;
}


int serial_get_cmd(Serial_Position_t *param, int port, int num)
{
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_set_position fd=[%d] is err!!!\n", fd);
		return -1;
	}

	
	char cmd = '0';
	cmd = 0x80 | num;
	printf("the cmd is %02x\n", cmd);
	
	char buffer[5] = {cmd, 0x09, 0x06, 0x12, 0xFF};
	ret = write(fd, buffer, sizeof(buffer));
	if (sizeof(buffer) != ret)
	{
		nslog(NS_ERROR, "serial_get_cmd send data is err!!!\n");
		return -1;
	}
	
	char buffer1[5] = {cmd, 0x09, 0x04, 0x47, 0xFF};
	ret = write(fd, buffer1, sizeof(buffer1));
	if (sizeof(buffer1) != ret)
	{
		nslog(NS_ERROR, "serial_get_cmd send data is err!!!\n");
		return -1;
	}
	
	usleep(100000);
	serial_get_pan(param, port);
	nslog(NS_ERROR, "\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^the param is %d %d %d\n", param->posit_pan, param->posit_tilt, param->coefficient);
	return 0;
	
}


int serial_control_presite(Presion_Type_t type, int index, Comport_Type_t port, int num)
{
	nslog(NS_ERROR, "serial_control_presite type=%d index=%d port=%d num=%d\n",type, index, port, num);
	int ret  = 0;
	
	switch (type)
	{
		case SET:
			ret = cmd_set_presite(index, port, num);
			break;
		case DEL:
			ret = cmd_reset_presite(index, port, num);
			break;
		case CALL:
			ret = cmd_recall_presite(index, port, num);
			break;
		default:
			ret = -1;
			break;

	}
	
	return ret;
}


int set_terminal(int fd,int nSpeed, int nStop,int nBits,char nEvent)
{
	struct termios newtio,oldtio;
	if ((0 != tcgetattr(fd,&oldtio)))
	{
		perror("SetUp Serial 1 !");
		return -1;
	}

	bzero(&newtio, sizeof(newtio));

	newtio.c_cflag |= (CREAD | CLOCAL);
	newtio.c_cflag &= ~CSIZE;

	switch(nBits)
	{
		case 7:
			newtio.c_cflag |= CS7;
			break;
		case 8:
			newtio.c_cflag |= CS8;
			break;
		default :
			break;
	}

	switch(nSpeed)
	{
		case 2400:
			cfsetispeed(&newtio,B2400);
			cfsetospeed(&newtio,B2400);  
			break;   
		case 4800:
			cfsetispeed(&newtio,B4800);
			cfsetospeed(&newtio,B4800);  
			break;   
		case 9600:
			cfsetispeed(&newtio,B9600);
			cfsetospeed(&newtio,B9600);
			break;   
		case 115200:
			cfsetispeed(&newtio,B115200);
			cfsetospeed(&newtio,B115200);
			break;  
		default:
			cfsetispeed(&newtio,B9600);
			cfsetospeed(&newtio,B9600); 
			break; 
	}
 

	switch(nEvent)
	{
		case 'O':
			newtio.c_cflag |= PARENB;
			newtio.c_cflag |= PARODD;
			newtio.c_iflag |= (INPCK | ISTRIP);
			break;
		case 'E':
			newtio.c_cflag |= PARENB;
			newtio.c_cflag &= ~PARODD;
			newtio.c_iflag |= (INPCK | ISTRIP);
			break;
		case 'N':
			newtio.c_cflag &= ~PARENB;
			break;
	}

	if (1 == nStop)
	{
		newtio.c_cflag &= ~CSTOPB;

	}
	else if (nStop == 2)
	{
		newtio.c_cflag |= ~CSTOPB;
	}
  
	newtio.c_cc[VTIME] = 5;
	newtio.c_cc[VMIN] = 0;

	tcflush(fd, TCIFLUSH);

	if ((tcsetattr(fd, TCSANOW, &newtio))!= 0)
	{
		perror("com set error !");
		return -1;
	} 

	nslog(NS_INFO, "Set Done !\n");
	return 0;
}

int open_port(int comport)
{
	//char *dev[] = {"/dev/ttyO0","/dev/ttyO1","/dev/ttyO2"};//DM8168
	//char *dev[] = {"/dev/ttyAMA0","/dev/ttyAMA1","/dev/ttyAMA2","/dev/ttyAMA3"};//HISI
	char *dev[] = {"/dev/ttyS3","/dev/ttyS9","/dev/ttyUSB0"};//rk3568
	int fd = 0; 
	if (comport == 0)
	{
		fd = open(dev[0],O_RDWR|O_NOCTTY|O_NDELAY);
		if (-1 == fd)
		{
	  		nslog(NS_ERROR, "Can't open serial");
			return (-1);
		}
	} 
	else if (comport == 1)
	{
		fd = open(dev[1],O_RDWR|O_NOCTTY|O_NDELAY);
		if (-1 == fd)
		{
	  		nslog(NS_ERROR, "Can't open serial");
			return (-1);
		}
	}
	else if (comport == 2)
	{
		fd = open(dev[2],O_RDWR|O_NOCTTY|O_NDELAY);
		if (-1 == fd)
		{
	  		nslog(NS_ERROR, "Can't open serial");
			return (-1);
		}

	}
	else if (comport == 3)
	{
		fd = open(dev[3],O_RDWR|O_NOCTTY|O_NDELAY);
		if (-1 == fd)
		{
			nslog(NS_ERROR, "Can't open serial");
			return (-1);
		}

	}

	if (fcntl(fd,F_SETFL,0)<0)
	{
		nslog(NS_ERROR, "fcntl failed \n");
	}
	else
	{
		nslog(NS_ERROR, "fcntl is sucess!!!!\n");
	}

	if ((isatty(STDIN_FILENO)) == 0)
	{
		nslog(NS_ERROR, "standard input is not a terminal \n");
	}
	else
	{
		nslog(NS_INFO, "isatty success");
	}
	
	nslog(NS_INFO, "fd-open=%d type=%d\n", fd, comport);
	return fd;
  
}


void *serial_read(void *arg)
{
	if (NULL == arg)
	{
		nslog(NS_ERROR, "serial_read pthread param is err!!!!\n");
		return NULL;
	}

	Read_Param_t *read_param = (Read_Param_t *)arg;
	fd_set rd;
	int fd = serial_get_fd(read_param->port);

	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_set_position fd=[%d] is err!!!\n", fd);
		return NULL;
	}
	
	int nread = 0;
	char buff[512] = {0};
	int i = 0;
	FD_ZERO(&rd);
	FD_SET(fd, &rd);
	while(FD_ISSET(fd, &rd))
	{
    	if (select(fd + 1, &rd, NULL, NULL, NULL) < 0)
    	{
			nslog(NS_INFO,"select error!\n");
    	}
    	else
    	{
			while((nread = read(fd, buff, 512)) > 0)
			{
				for (i = 0; i < nread; i ++)
				{
					nslog(NS_INFO, "0x%02x ", buff[i]);
				}
				nslog(NS_INFO, "nread=%d\n", nread);
				if (nread < 7)//不是我们要的值
				{
					break;
				}

				if (nread == 11 && 0xFF == buff[10])
				{
					serial_set_panZoom(buff, read_param->port, PAN);
					memset(buff, 0, sizeof(buff));
				}

				
				if (nread == 7  && 0xFF == buff[6])
				{
					serial_set_panZoom(buff, read_param->port, ZOOM);
					memset(buff, 0, sizeof(buff));
				}
				
			}
		}
	}

	return NULL;
}

static int serial_read_pthread(Read_Param_t *param,Uart_Param_t* pdev)
{
	if (NULL == param)
	{
		nslog(NS_ERROR, "serial_read_pthread param is err!!!\n");
		return -1;
	}

	Read_Param_t *read_param = (Read_Param_t *)malloc(sizeof(Read_Param_t));
	if (NULL == read_param)
	{
		nslog(NS_ERROR, "serial_read_pthread malloc is err!!!\n");
		return -1;
	}
	memcpy(read_param, param, sizeof(Read_Param_t));
	
	pthread_t tid;
	int serial_val = -1;
	pthread_attr_t attr_serial;
	pthread_attr_init(&attr_serial);
	pthread_attr_setscope(&attr_serial, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&attr_serial, PTHREAD_CREATE_JOINABLE);
	serial_val = pthread_create(&tid, &attr_serial, pdev->serial_process_thr, (void*)(read_param));
	if (serial_val)
	{
		nslog(NS_ERROR, "pthread_create is fail\n");
	}
	else
	{
		nslog(NS_DEBUG, "pthread_create %d is suceess\n", read_param->port);
	}

	pthread_attr_destroy(&attr_serial);
	return 0;

}


static int serial_heart_pthread(Read_Param_t *param,Uart_Param_t* pdev)
{
	if ((NULL == param) || (NULL == pdev) || (NULL == pdev->serial_heart_thr))
	{
		nslog(NS_ERROR, "serial_heart_pthread param is err!!!\n");
		return -1;
	}

	Read_Param_t *read_param = (Read_Param_t *)malloc(sizeof(Read_Param_t));
	if (NULL == read_param)
	{
		nslog(NS_ERROR, "serial_read_pthread malloc is err!!!\n");
		return -1;
	}
	memcpy(read_param, param, sizeof(Read_Param_t));
	
	pthread_t tid;
	int serial_val = -1;
	pthread_attr_t attr_serial;
	pthread_attr_init(&attr_serial);
	pthread_attr_setscope(&attr_serial, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&attr_serial, PTHREAD_CREATE_JOINABLE);
	serial_val = pthread_create(&tid, &attr_serial, pdev->serial_heart_thr, (void*)(read_param));
	if (serial_val)
	{
		nslog(NS_ERROR, "pthread_create is fail\n");
	}
	else
	{
		nslog(NS_DEBUG, "pthread_create %d is suceess\n", read_param->port);
	}

	pthread_attr_destroy(&attr_serial);
	return 0;

}


int serial_init(Uart_Param_t *arg)
{
	int fd;
	int comport;
	int speed;
	int stop;
	int bits;
	char event;
	int ret;

	if (NULL != arg)
	{
		comport	= arg->comport;
		speed	= arg->speed;
		stop	= arg->stop;
		bits	= arg->bits;
		event	= arg->event;
	}
	else
	{
		Uart_Param_t *uartCfg = (Uart_Param_t *)&pUartCfg;
		comport	= uartCfg->comport;
		speed	= uartCfg->speed;
		stop	= uartCfg->stop;
		bits	= uartCfg->bits;
		event	= uartCfg->event;
	}
	printf("serial_init start!!\n");
    if ( arg->serial_process_thr == NULL )
    {
        nslog(NS_ERROR, "serial_process_thr is NULL\n");
        return -1;
    }
	
	if ((1 != initCfg.flag_tty0) && (TTY0_PORT == comport))
	{
		pthread_mutex_init(&(initCfg.mutex_tty0), NULL);
		if ((fd = open_port(comport)) < 0)
		{
			nslog(NS_ERROR, "open_port error\n");
			return -1;
		}

		serial_set_fd(fd, TTY0_PORT);
  		nslog(NS_INFO, "Uart configure:( port %d;speed %d;stop %d; bits %d; event %c )\n", comport, speed, stop, bits, event);
		if ((ret = set_terminal(fd, speed, stop, bits, event)) < 0)
		{
			nslog(NS_ERROR, "set_terminal error");
			return -1;
		}

		pthread_mutex_init(&(g_position[TTY0_PORT].mutex), NULL);
		Read_Param_t read_param = {0};
		read_param.port = comport;
		serial_read_pthread(&read_param,arg);
		serial_heart_pthread(&read_param,arg);
		
		//cmd_auto_focus(TTY0_PORT, char cmd)
	}

	if ((1 != initCfg.flag_tty1) && (TTY1_PORT == comport))
	{
		pthread_mutex_init(&(initCfg.mutex_tty1), NULL);
		if ((fd = open_port(comport)) < 0)
		{
			nslog(NS_ERROR, "open_port error\n");
			return -1;
		}

		serial_set_fd(fd, TTY1_PORT);
  		nslog(NS_INFO, "Uart configure:( port %d;speed %d;stop %d; bits %d; event %c )\n", comport, speed, stop, bits, event);
		if ((ret = set_terminal(fd, speed, stop, bits, event)) < 0)
		{
			nslog(NS_ERROR, "set_terminal error");
			return -1;
		}

		pthread_mutex_init(&(g_position[TTY1_PORT].mutex), NULL);
		Read_Param_t read_param = {0};
		read_param.port = comport;
		serial_heart_pthread(&read_param,arg);
		serial_read_pthread(&read_param,arg);
	}

	if ((1 != initCfg.flag_tty2) && (TTY2_PORT == comport))
	{
		pthread_mutex_init(&(initCfg.mutex_tty2), NULL);
		if ((fd = open_port(comport)) < 0)
		{
			nslog(NS_ERROR, "open_port error\n");
			return -1;
		}

		serial_set_fd(fd, TTY2_PORT);
  		nslog(NS_INFO, "Uart configure:( port %d;speed %d;stop %d; bits %d; event %c )\n", comport, speed, stop, bits, event);
		if ((ret = set_terminal(fd, speed, stop, bits, event)) < 0)
		{
			nslog(NS_ERROR, "set_terminal error");
			return -1;
		}

		pthread_mutex_init(&(g_position[TTY2_PORT].mutex), NULL);
		Read_Param_t read_param = {0};
		read_param.port = comport;
		serial_heart_pthread(&read_param,arg);
		serial_read_pthread(&read_param,arg);
	}

	if ((1 != initCfg.flag_tty3) && (TTY3_PORT == comport))
	{
		pthread_mutex_init(&(initCfg.mutex_tty3), NULL);
		if ((fd = open_port(comport)) < 0)
		{
			nslog(NS_ERROR, "open_port error\n");
			return -1;
		}

		serial_set_fd(fd, TTY3_PORT);
		nslog(NS_INFO, "Uart configure:( port %d;speed %d;stop %d; bits %d; event %c )\n", comport, speed, stop, bits, event);
		if ((ret = set_terminal(fd, speed, stop, bits, event)) < 0)
		{
			nslog(NS_ERROR, "set_terminal error");
			return -1;
		}

		pthread_mutex_init(&(g_position[TTY3_PORT].mutex), NULL);
		Read_Param_t read_param = {0};
		read_param.port = comport;
		serial_heart_pthread(&read_param,arg);
		serial_read_pthread(&read_param,arg);
	}
	
	if ((1 != initCfg.flag_tty4) && (TTY0USB_PORT == comport))
	{
		pthread_mutex_init(&(initCfg.mutex_tty4), NULL);
		if ((fd = open_port(comport)) < 0)
		{
			nslog(NS_ERROR, "open_port error\n");
			return -1;
		}

		serial_set_fd(fd, TTY0USB_PORT);
		nslog(NS_INFO, "Uart configure:( port %d;speed %d;stop %d; bits %d; event %c )\n", comport, speed, stop, bits, event);
		if ((ret = set_terminal(fd, speed, stop, bits, event)) < 0)
		{
			nslog(NS_ERROR, "set_terminal error");
			return -1;
		}

		pthread_mutex_init(&(g_position[TTY0USB_PORT].mutex), NULL);
		Read_Param_t read_param = {0};
		read_param.port = comport;
		serial_heart_pthread(&read_param,arg);
		serial_read_pthread(&read_param,arg);
	}


	return 0;

}

int serial_deinit(Comport_Type_t port)
{
	#if 0
	if (TTY0_PORT == port)
	{

		pthread_mutex_init(&(initCfg.mutex_tty2), NULL);
		if ((fd = open_port(comport)) < 0)
		{
			nslog(NS_ERROR, "open_port error\n");
			return -1;
		}

		serial_set_fd(fd, TTY2_PORT);
  		nslog(NS_INFO, "Uart configure:( port %d;speed %d;stop %d; bits %d; event %c )\n", comport, speed, stop, bits, event);
		if ((ret = set_terminal(fd, speed, stop, bits, event)) < 0)
		{
			nslog(NS_ERROR, "set_terminal error");
			return -1;
		}

		pthread_mutex_init(&(g_position[TTY2_PORT].mutex), NULL);
		Read_Param_t read_param = {0};
		read_param.port = comport;
		serial_read_pthread(&read_param);
	#endif
	return 0;
}




/**
 * @name       serial_write
 * @brief      common serial write 
 * @param[IN]   len:the lenght of buf
 * @param[IN]   buf:buffer to write
 * @param[OUT]  none
 * @return	   
 */
int serial_write(Comport_Type_t port,int len,unsigned char* buf)
{
	int fd = serial_get_fd(port);
	int ret = 0;
	if (fd < 3)
	{
		nslog(NS_ERROR, "cmd_do_home fd=[%d] is err!!!\n", fd);
		return -1;
	}

	ret = write(fd, buf, len);
	if (len != ret)
	{
		nslog(NS_ERROR, "cmd_do_home send data is err!!!\n");
		return -1;
	}
	return 0;
}
int get_serial_info(Serial_set_t *serial_info)
{

	pthread_mutex_lock(&g_serial_mutex);
	memcpy( serial_info,&g_serial_info, sizeof(Serial_set_t));
	pthread_mutex_unlock(&g_serial_mutex);
	return 0;
}
int set_serial_info(Serial_set_t serial_info)
{

	pthread_mutex_lock(&g_serial_mutex);
	memcpy( &g_serial_info,&serial_info, sizeof(Serial_set_t));
	pthread_mutex_unlock(&g_serial_mutex);
	return 0;
}

// int get_serial_xml(Serial_set_t *serial_info)
// {
// //	printf("in...................................\n");
// 	char comport_nodename[128] = {0};
// 	char addr_nodename[128] = {0};
// 	int get_fileerror = 0;
// 	if (NULL == serial_info)
// 	{
// 		nslog(NS_ERROR,"get_serial_xml param is err!\n");
// 		return -1;
// 	}
// 	int i = 0;

// 	/*获取控制类型，0-UDP，1-串口UART*/
// 	if (xml_get_intNode2("/root/MsgBody/com_type/", &(serial_info->com_type), SERIAL_CONFIG) != TRUE)
// 	{
// 		get_fileerror = 1;
// 	}
// 	else
// 	{
// 		if (xml_get_intNode2("/root/MsgBody/keyboard_port/", &(serial_info->keyboard_port), SERIAL_CONFIG) != TRUE)
// 		{
// 			get_fileerror = 1;
// 		}
// 		if (xml_get_intNode2("/root/MsgBody/keyboard_B/", &(serial_info->keyboard_B), SERIAL_CONFIG) != TRUE)
// 		{
// 			get_fileerror = 1;
// 		}
// 		if (xml_get_intNode2("/root/MsgBody/keyboard_KG/", &(serial_info->keyboard_KG), SERIAL_CONFIG) != TRUE)
// 		{
// 			get_fileerror = 1;
// 		}
// 		if (xml_get_intNode2("/root/MsgBody/external_control_port/", &(serial_info->external_control_port), SERIAL_CONFIG) != TRUE)
// 		{
// 			get_fileerror = 1;
// 		}
// 		for(i = 0; i < sizeof(serial_info->port) / sizeof(serial_info->port[0]); i++ )
// 		{
// 			sprintf(comport_nodename, "/root/MsgBody/comport%d/",i );
// 			if(xml_get_intNode2(comport_nodename, (int *)&(serial_info->port[i]), SERIAL_CONFIG) != TRUE)
// 			{
// 				get_fileerror = 1;
// 				break;
// 			}

// 			sprintf(addr_nodename, "/root/MsgBody/addr%d/",i );
// 			if(xml_get_intNode2(addr_nodename, (int *)&(serial_info->addr[i]), SERIAL_CONFIG) != TRUE)
// 			{
// 				get_fileerror = 1;
// 				break;
// 			}
// 		}
// 	}
// 	if(get_fileerror == 1)
// 	{
// 		serial_info->com_type = 0;
// 		serial_info->keyboard_port = 0;
// 		serial_info->external_control_port = 2;	//串口2作为中控接口
// 		serial_info->keyboard_KG = 0;
// 		serial_info->keyboard_B = 0;
// 		xml_set_intNode2("/root/MsgBody/com_type/", (serial_info->com_type), SERIAL_CONFIG);
// 		xml_set_intNode2("/root/MsgBody/keyboard_port/", (serial_info->keyboard_port), SERIAL_CONFIG);
// 		xml_set_intNode2("/root/MsgBody/external_control_port/", (serial_info->external_control_port), SERIAL_CONFIG);
// 		xml_set_intNode2("/root/MsgBody/keyboard_KG/",(serial_info->keyboard_KG),SERIAL_CONFIG);
// 		xml_set_intNode2("/root/MsgBody/keyboard_B/",(serial_info->keyboard_B),SERIAL_CONFIG);

// 		for(i = 0; i < sizeof(serial_info->port) / sizeof(serial_info->port[0]); i++ )
// 		{
// 			serial_info->port[i] = 2;
// 			serial_info->addr[i] = i;
// 			sprintf(comport_nodename, "/root/MsgBody/comport%d/",i );
// 			xml_set_intNode2(comport_nodename, serial_info->port[i], SERIAL_CONFIG);
// 			sprintf(addr_nodename, "/root/MsgBody/addr%d/",i );
// 			xml_set_intNode2(addr_nodename, serial_info->addr[i], SERIAL_CONFIG);

// 		}
// 	}
// 	memcpy(&g_serial_info, serial_info, sizeof(Serial_set_t));
// //	printf("out...................................\n");
// 	return 0;
// }


int ascii_isspace(char c)
{
    char comp[]={' ','\t','\n','\r','\v','\f'};
    int i=0;
    int len=sizeof(comp)-1;
    for(i=0;i<len;i++){
        if(c==comp[i]){
            return 1;
        }
    }
    return 0;
}

int convertHexToInt(const char * nptr,int count)
{
    const char *s;
    char c;
    int acc=0;
    int base=16;
    s=nptr;

    do {
        c = *s++;
    } while (ascii_isspace(c));

    int i=0;
    for (i=0 ;i<count; c = *s++,i++) {
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'A' && c <= 'Z')
            c -= 'A' - 10;
        else if (c >= 'a' && c <= 'z')
            c -= 'a' - 10;
        else
            break;
        if (c >= base)
            break;

        acc *= base;
        acc += c;
    }
    return acc;
}

//说明： 将 FF FF FF带空格的字符串转为ascii码， 放入最多 maxsize个数到result里,返回值为转存的个数
int convertToAscii(const char *string,char *result,int maxsize)
{
    if(maxsize<=0 || string==NULL || result==NULL)
        return -1;
    const char *s=string;
    char *r=result;
    char c;
    int count=0;
    int rCount=0;
    int strlength=strlen(s);

    if(strlength<=0){
    	return 0;
    }

    int ret=0;
    int charCount=0;
    while(1){
        while(ascii_isspace(*s)){
            s++;
            count++;
        }
        if(ascii_isspace(*(s+1))){
            charCount=1;
        }else{
            charCount=2;
        }
        ret=convertHexToInt(s,charCount);

        rCount++;
        if(rCount>=maxsize){
        	rCount=maxsize;
            break;
        }
        sprintf(r,"%c",ret);
        r++;


        count+=charCount;
        if(count>=strlength){
            break;
        }
        s+=2;
    }
    return rCount;
}









