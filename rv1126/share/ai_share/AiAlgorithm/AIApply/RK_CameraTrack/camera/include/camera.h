#ifndef _CAMERA_H
#define _CAMERA_H

#include <sys/socket.h>
#include <netinet/in.h>


#define CLIENT_CAMERA_SPEED_PAN_MIN 0
#define CLIENT_CAMERA_SPEED_PAN_MAX 24
#define CLIENT_CAMERA_SPEED_TILT_MIN 0
#define CLIENT_CAMERA_SPEED_TILT_MAX 20

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef BOOL
#define BOOL int
#endif
/* 头长度 */
#define CRADLE_MSG_HEAD_LEN 7	
 /* 头合法标志 */
#define CRADLE_MSG_HEAD_IDF 0x50  
///CMD控制定义

#define PANTILT_CTRL_PTZ_GOTO_PRESET     	0x4001//运行到预置位
#define PANTILT_CTRL_PTZ_SET_PRESET      	0x4002//设置预置位
#define PANTILT_CTRL_PTZ_CLE_PRESET      	0x4003//清除预置位

#define CLIENT_CAMERA_POSITTOIN_PAN_MIN -0x0990
#define CLIENT_CAMERA_POSITTOIN_PAN_MAX 0x0990
#define CLIENT_CAMERA_POSITTOIN_TILT_MIN -0x01B0
#define CLIENT_CAMERA_POSITTOIN_TILT_MAX 0x0510
#define POSITTION_TO_4BYTE(x) ((x >> 12) & 0x000f), ((x >> 8) & 0x000f), ((x >> 4) & 0x000f), (x & 0x000f)

#define CLIENT_CAMERA_ZOOM_MIN 0x0000
#define CLIENT_CAMERA_ZOOM_MAX 0x4000
#define ZOOMPOSITION_TO_4BYTE(x) ((x >> 12) & 0x000f), ((x >> 8) & 0x000f), ((x >> 4) & 0x000f), (x & 0x000f)


typedef enum
{
	MOVE_HOME = 0,	//归位
	MOVE_FAR,		//缩小
	MOVE_NEAR,		//放大
	STOP_ZOOM,		//停止放大缩小
	MOVE_UP,		//向上
	MOVE_DOWN,		//向下
	MOVE_LEFT,		//向左
	MOVE_RIGHT,		//向右
	STOP_TURN,		//停止

}CamType_E;


/* 控制摄像头参数 */
typedef struct _CameraControl_
{
    int m_flag_start;
    struct sockaddr_in m_addr;
    unsigned char m_buffer[2048];
    int m_addr_len;
    int m_send_socket;

    int move_speed_pan;	 //水平范围0x0-0x18
    int move_speed_tilt; //倾斜范围0x0-0x14

    int m_thread_run_flag;
    pthread_t heart_tid;
    int m_posit_pan;
    int m_posit_tilt;
    int m_zoomValue;

    pthread_mutex_t mutex1;
    pthread_mutex_t mutex2;
    pthread_cond_t cond1; // init cond
    pthread_cond_t cond2; // init cond
    char connect_ip[16];  //输入参数
    int control_port;	  //输入参数
    int is_init;		  //代表是否初始化 避免重复初始化
#ifdef SUPPORT_SONGXIA_CAM
    StrategyType_E nStrategyType;
#endif
} Strategy_CamControl_t;

typedef void* CAM_CONTROL_HANDLE;

/*返回原位*/
int goback_home(Strategy_CamControl_t *cam);

int udp_move_cmd(int type, int speed, Strategy_CamControl_t *cam);

//nXpos:水平位置范围 538560 - 1077120
//nYpos:垂直位置范围 153600 - 307200
int cmd_move_pos(Strategy_CamControl_t *cam,int speed, int nXpos,int nYpos);

Strategy_CamControl_t* init_udp_cam(char *ip, int nControlPort);

#endif

