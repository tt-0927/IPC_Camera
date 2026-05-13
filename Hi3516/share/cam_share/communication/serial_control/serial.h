#ifndef _SERIAL_H_
#define _SERIAL_H_

#ifdef __cplusplus
extern "C" {
#endif 

#include <pthread.h>
#define CLIENT_CAMERA_ZOOM_MIN 0x0000
#define CLIENT_CAMERA_ZOOM_MAX 0x4000

#define CLIENT_CAMERA_POSITTOIN_PAN_MIN -0x0990
#define CLIENT_CAMERA_POSITTOIN_PAN_MAX 0x0990
#define CLIENT_CAMERA_POSITTOIN_TILT_MIN -0x01B0
#define CLIENT_CAMERA_POSITTOIN_TILT_MAX 0x0510

#define SPEED_PAN_MIN 0x01
#define SPEED_PAN_MAX 0x18
#define SPEED_TILT_MIN 0x01
#define SPEED_TILT_MAX 0x14

#define SERIAL_CONFIG "/opt/hisi/.config/design_data/serial_control.xml"
#define CAM_NUM 5

typedef struct UartParam_t
{
	int speed;
	int stop;
	int bits;
	char event;
	int comport;
    void *(*serial_process_thr)(void *arg);
	void *(*serial_heart_thr)(void *arg);
} Uart_Param_t;

typedef struct init_param
{
	int flag_tty0;
	pthread_mutex_t mutex_tty0;
	int tty0Handel;
	int flag_tty1;
	pthread_mutex_t mutex_tty1;
	int tty1Handel;
	int flag_tty2;
	pthread_mutex_t mutex_tty2;
	int flag_tty3;
	int tty2Handel;
	pthread_mutex_t mutex_tty3;
	int tty3Handel;
	int flag_tty4;
	pthread_mutex_t mutex_tty4;
	int tty4Handel;
}Init_Param_t;

typedef enum
{
	GOHOME = 0,
	GONEAR = 1,
	GOFAR = 2,
	STOPZ = 3,
	GOUP = 4,
	GODOWN = 5,
	GOLEFT = 6,
	GORIGHT = 7,
	STOPTURN = 8
}Move_Type_t;

typedef enum
{
	TTY0_PORT = 0,
	TTY1_PORT = 1,
	TTY2_PORT = 2,
	TTY3_PORT =3,
	TTY0USB_PORT = 4,
}Comport_Type_t;

typedef enum
{
	PAN = 0,
	ZOOM = 1,
}Sposition_Type_t;


typedef enum
{
	SET = 0,
	DEL = 1,
	CALL = 2	
}Presion_Type_t;

typedef enum 
{
	CAM_MOVE,
	CAM_SITE,
	CAM_GDIR
}Track_type_t;

typedef enum 
{
	CAM_TEA = 0,
	CAM_STU = 3,
	CAM_PTEA = 1,
	CAM_PSTU = 4,
	CAM_BLB = 2,
}Cam_Type_t;

typedef enum 
{
	CAM_UDP = 0,
	CAM_UART = 1,
}Communication_Type_t;

typedef struct Pos_t
{
	int posit_pan;
	int posit_tilt;
	int coefficient;
	Cam_Type_t cam_type;
} Serial_Position_t;

typedef struct Param_t
{
	Move_Type_t move_type;
	Cam_Type_t cam_type;
	Presion_Type_t site_type;
	int speed;
	int index;
	Track_type_t type;
	Serial_Position_t serial_position;
} Serial_Param_t;

typedef struct PanZoom_t
{
	int posit_pan;
	int posit_tilt;
	int coefficient;
	pthread_mutex_t mutex;
} Serial_PanZoom_t;

typedef struct Serial_t
{
	Communication_Type_t com_type;
	Comport_Type_t port[8];//kj b
	int addr[8];
	Comport_Type_t keyboard_port;
	Comport_Type_t external_control_port;
	Comport_Type_t keyboard_KG;
	Comport_Type_t keyboard_B;
	Comport_Type_t keyboard_B2;
}Serial_set_t;

typedef struct 
{
	int port;//第几个串口
	int index;//第几个相机
	
}Read_Param_t;

/*@云台控制命令 
 *@控制类型   相机类型
 *@失败返回非0值
 */

int serial_move_cmd(Move_Type_t type, Comport_Type_t port, int speed, int num);

/*@拉伸到指定位置
 *@ 相机类型
 *@失败返回非0值
 */

int cmd_move_direZoom(int zoomPosition, Comport_Type_t port, int num);



/*@移动到指定位置    
 *@水平方向位置，垂直方向位置，水平移动速度，垂直移动速度，  相机类型
 *@失败返回非0值
 */
 
int cmd_move_Direpo(int position_pan, int position_tilt, int move_speed_pan, int move_speed_tilt, Comport_Type_t port, int num);


/*@设置预置位
 *@设置类型 设置位置    相机类型
 *@失败返回非0值
 */

int get_serial_xml(Serial_set_t *serial_info);

int serial_control_presite(Presion_Type_t type, int index, Comport_Type_t port, int num);

int cmd_get_msg(Comport_Type_t port);

int serial_get_cmd(Serial_Position_t *param,int port, int num);

int serial_get_fd(Comport_Type_t type);

//ydl's serial
void *serial_read(void *arg);

/**
 * @name       serial_write
 * @brief      common serial write 
 * @param[IN]  port:UartParam_t.comport
 * @param[IN]   len:the lenght of buf
 * @param[IN]   buf:buffer to write
 * @param[OUT]  none
 * @return	   
 */
int serial_write(Comport_Type_t port,int len,unsigned char* buf);

int serial_init(Uart_Param_t *arg);

int serial_deinit();

int set_serial_info(Serial_set_t serial_info);
int get_serial_info(Serial_set_t *serial_info);
int convertToAscii(const char *string,char *result,int maxsize);

#ifdef __cplusplus
}
#endif 

#endif  /* end of file */
