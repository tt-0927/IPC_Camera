#include "share_socket.h"
#include "camera.h"

#include <pthread.h>

static int getRun_Status(Strategy_CamControl_t *cam)
{
	return cam->m_thread_run_flag;
}

static int getStart_Status(Strategy_CamControl_t *cam)
{
	return cam->m_flag_start;
}

static int recv_CameraInfo(Strategy_CamControl_t *cam, unsigned char* buffer)
{
    if (NULL == cam || NULL == buffer)
    {
        printf( "recv_CameraInfo parameters is null!");
        return -1;
    }

	int len = -1;

	if (-1 != cam->m_send_socket)
	{
    	len = recv(cam->m_send_socket, cam->m_buffer, sizeof(cam->m_buffer), 0);
    	if (len > 0)
    	{
        	memcpy(buffer, cam->m_buffer, len);
   		}
	}
	else
	{
		printf("cam->m_send_socket is invilad!!!\n");
		return -1;
	}

    return len;
}

static int output_recv_caminfo(int nLen, unsigned char *pchBuffer)
{
	int nIndex = 0;
	for(nIndex = 0;nIndex < nLen; nIndex++)
	{
		// printf("pchBuffer[%d]:%02x\n",nIndex, pchBuffer[nIndex]);
	}
	return 0;
}

static void set_CameraInfo_panTilt(Strategy_CamControl_t *cam, int posit_pan, int posit_tilt)
{
    if (NULL == cam)
    {
        // printf( "set_CameraInfo_panTilt!!!\n");
        return;
    }

    cam->m_posit_pan = posit_pan;
    cam->m_posit_tilt = posit_tilt;
}

static void set_CameraInfo_zoom(Strategy_CamControl_t *cam , int zoomValue)
{
    if (NULL == cam)
    {
        printf( "set_CameraInfo_zoom is err!\n");
        return;
    }

    cam->m_zoomValue = zoomValue;
}

static int start_connect_IpCam(Strategy_CamControl_t *cam, const char *addr, const int port)
{
    if (NULL == cam || NULL == addr || port < 0)
    {
    	printf( "startControl param is err!! cam = %p addr = %p port = %d\n",cam, addr, port);
        return -1;
    }


	printf( "start_connect_IpCam cam->ip=%s", addr);
    if (cam->m_thread_run_flag == 1)
    {
        if (cam->m_flag_start == 0)
        {
            cam->m_addr.sin_family = AF_INET;
            cam->m_addr.sin_addr.s_addr = inet_addr(addr);
            cam->m_addr.sin_port = htons(port);

            cam->m_send_socket = RH_Socket(__FILE__,(char*) __func__,AF_INET, SOCK_DGRAM, 0);
			if (cam->m_send_socket < 1)
			{
				printf( "the cam->m_send_socket socket is err!\n");
				return -1;
			}

            if (-1 == connect(cam->m_send_socket, ( struct sockaddr *)&(cam->m_addr), sizeof(cam->m_addr)))
            {
				printf( "start_connect_IpCam connect is err\n");
				return -1;
			}

            struct timeval Time;
            Time.tv_sec = 3;
            Time.tv_usec = 0;
            setsockopt(cam->m_send_socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&Time, sizeof(struct timeval));
            if (3 > cam->m_send_socket)
            {
                printf("errno=%d,<%s>!\n",errno,strerror(errno));
                return -1;
            }

            cam->m_flag_start = 1;
        }

        return 1;
    }
    else
    {
        return -1;
    }
}

static void* recv_IpCam_thread(void *argv)
{
	Strategy_CamControl_t * pCamHandle = (Strategy_CamControl_t*)argv;
	int len = 0;
	unsigned char buffer[256] = {0};
	int posit_pan = 0;
	int posit_tilt = 0;
	int zoomValue = 0;
	while (getRun_Status(pCamHandle) == 1)
	{
		if (getStart_Status(pCamHandle) == 1)
		{
			len = recv_CameraInfo(pCamHandle, buffer);
			if ((len >= CRADLE_MSG_HEAD_LEN) && (buffer[1] == CRADLE_MSG_HEAD_IDF))
			{
				if (len > CRADLE_MSG_HEAD_LEN)
				{
					posit_pan = ((buffer[2] & 0x0f) << 12) + ((buffer[3] & 0x0f) << 8) + ((buffer[4] & 0x0f) << 4) + (buffer[5] & 0x0f);
					posit_pan = ((posit_pan >> 15) & 1) ? posit_pan | ((-1 >> 16) << 16) : posit_pan & 0xffff;

					posit_tilt = ((buffer[6] & 0x0f) << 12) + ((buffer[7] & 0x0f) << 8) + ((buffer[8] & 0x0f) << 4) + (buffer[9] & 0x0f);
					posit_tilt = ((posit_tilt >> 15) & 1) ? posit_tilt | ((-1 >> 16) << 16) : posit_tilt & 0xffff;
					//printf( "recv_CameraInfo %d %d\n", posit_pan , posit_tilt);
					pthread_mutex_lock(&pCamHandle->mutex1);
					set_CameraInfo_panTilt(pCamHandle,posit_pan, posit_tilt);
					pthread_cond_signal(&pCamHandle->cond1);
					pthread_mutex_unlock(&pCamHandle->mutex1);
				}
				else if (len == CRADLE_MSG_HEAD_LEN)
				{
					zoomValue = ((buffer[2] & 0x0f) << 12) + ((buffer[3] & 0x0f) << 8) + ((buffer[4] & 0x0f) << 4) + (buffer[5] & 0x0f);
					zoomValue = ((zoomValue >> 15) & 1) ? zoomValue | ((-1 >> 16) << 16) : zoomValue & 0xffff;
					//printf( "recv_CameraInfo cccc=%d\n", zoomValue);
					pthread_mutex_lock(&pCamHandle->mutex2);
					set_CameraInfo_zoom(pCamHandle,zoomValue);
					pthread_cond_signal(&pCamHandle->cond2);
					pthread_mutex_unlock(&pCamHandle->mutex2);
				}
				//else
				{
					output_recv_caminfo(len, buffer);
				}

			}
			else if(len > 0)
			{
				output_recv_caminfo(len, buffer);
			}
		}

		usleep(10 * 1000);
	}
	return NULL;
}

static int init_track_IpCam(Strategy_CamControl_t *cam)
{
	cam->m_flag_start = 0;
	memset(&(cam->m_addr), 0, sizeof(cam->m_addr));
	memset(&(cam->m_buffer), 0, sizeof(cam->m_buffer));
	cam->m_addr_len = sizeof(struct sockaddr_in);
	cam->m_send_socket = -1;

	cam->move_speed_pan = CLIENT_CAMERA_SPEED_PAN_MAX;
	cam->move_speed_tilt = CLIENT_CAMERA_SPEED_TILT_MAX;

	pthread_mutex_init(&(cam->mutex1),NULL);
	pthread_mutex_init(&(cam->mutex2), NULL);
	pthread_cond_init(&(cam->cond1),NULL);
	pthread_cond_init(&(cam->cond2),NULL);
	cam->m_thread_run_flag = 1;
	int ret = pthread_create(&(cam->heart_tid), NULL, recv_IpCam_thread, (void *)(cam));
	if (ret)
	{
		printf("pthread_create recv_IpCam_thread is fail\n");
	}

	return ret;
}


static int send_net_cmd(Strategy_CamControl_t *cam, unsigned char *cmd, int len)
{
	if (NULL==cam || NULL== cmd)
	{
		printf("send_net_cmd param is err!!\n");
		return -1;
	}

	int ret = -1;
	if (cam->m_send_socket > 0)
	{
		ret = send(cam->m_send_socket, cmd, len, 0);
		if (ret != len)
		{
			printf( "send_net_cmd send is err!!! len=[%d] ret=[%d]\n", len, ret);
			return -1;
		}
	}

	return 0;
}

int goback_home(Strategy_CamControl_t *cam)
//返回源点
{
    if (NULL == cam)
    {
        printf("goback_home!!!!!\n");
        return -1;
    }

    unsigned char instruct[] = { 0x81, 0x01, 0x06, 0x04, 0xFF };
    if (send_net_cmd(cam, instruct, sizeof(instruct)) != 0)
    {
    	printf( "send_net_cmd is err!!!!\n");
        return -1;
    }
    else
    {
    	printf( "goback_home is right\n");
        return 0;
    }
}

static int cmd_move_home(Strategy_CamControl_t *cam)
{

	unsigned char instruct[] = {0x81, 0x01, 0x06, 0x04, 0xFF};
	if (send_net_cmd(cam, instruct, sizeof(instruct)) != 0)
	{
		printf("send_net_cmd to stu cam is err!!!\n");
		return -1;
	}
	else
	{
		return 0;
	}
}

static int cmd_move_far(Strategy_CamControl_t *cam, int speed)
{
	if ((0 >= speed) || (speed > 7))
	{
		speed = 0x02;
	}
	else
	{
		speed = speed * 2;
	}
	printf( "go far speed = %d\n", speed);
	unsigned char buffer[6] = {0x81, 0x01, 0x04, 0x07, speed, 0xFF};
	if (send_net_cmd(cam, buffer, sizeof(buffer)) != 0)
	{
		printf("send_net_cmd to stu cam is err!!!\n");
		return -1;
	}
	else
	{
		return 0;
	}

}

static int cmd_move_near(Strategy_CamControl_t *cam, int speed)
{
	if ((0 >= speed) || (speed > 7))
	{
		speed = 0x03;
	}
	else
	{
		speed = speed * 3;
	}
	printf( "go far speed = %d\n", speed);
	unsigned char buffer[6] = {0x81, 0x01, 0x04, 0x07, speed, 0xFF};
	if (send_net_cmd(cam, buffer, sizeof(buffer)) != 0)
	{
		printf("send_net_cmd to stu cam is err!!!\n");
		return -1;
	}
	else
	{
		return 0;
	}

	return 0;
}

static int cmd_move_stopZoom(Strategy_CamControl_t *cam)
{
	unsigned char buffer[6] = {0x81, 0x01, 0x04, 0x07, 0x00, 0xFF};
	if (send_net_cmd(cam, buffer, sizeof(buffer)) != 0)
	{
		printf("send_net_cmd to stu cam is err!!!\n");
		return -1;
	}
	else
	{
		return 0;
	}


	return 0;
}

static int cmd_move_up(Strategy_CamControl_t *cam, int speed)
{
	if ((speed < 0) || (speed > 0x18))
	{
		speed = 0x08;
	}

	printf( "cmd_move_up speed =%d\n", speed);
	unsigned char buffer[] = {0x81, 0x01,0x06, 0x01, 0x00, speed, 0x03, 0x01, 0xFF};
	if (send_net_cmd(cam, buffer, sizeof(buffer)) != 0)
	{
		printf("send_net_cmd to stu cam is err!!!\n");
		return -1;
	}
	else
	{
		return 0;
	}

	return 0;
}



static int cmd_move_down(Strategy_CamControl_t *cam, int speed)
{

	if ((speed < 0) || (speed > 0x18))
	{
		speed = 0x08;
	}
	printf( "cmd_move_down speed =%d\n", speed);
	unsigned char buffer[] = {0x81, 0x01,0x06, 0x01, 0x00, speed, 0x03, 0x02, 0xFF};
	if (send_net_cmd(cam, buffer, sizeof(buffer)) != 0)
	{
		printf("send_net_cmd to stu cam is err!!!\n");
		return -1;
	}
	else
	{
		return 0;
	}

	return 0;
}

static int cmd_move_left(Strategy_CamControl_t *cam, int speed)
{
	if ((speed < 0) || (speed > 0x14))
	{
		speed = 0x08;
	}
	printf( "cmd_move_left speed =%d\n", speed);
	unsigned char buffer[] = {0x81, 0x01,0x06, 0x01, speed, 0x00, 0x01, 0x03, 0xFF};
	if (send_net_cmd(cam, buffer, sizeof(buffer)) != 0)
	{
		printf("send_net_cmd to stu is err!!!\n");
		return -1;
	}
	else
	{
		return 0;
	}

}

static int cmd_move_right(Strategy_CamControl_t *cam, int speed)
{
	if ((speed < 0) || (speed > 0x14))
	{
		speed = 0x08;
	}
	printf( "cmd_move_right speed =%d\n", speed);
	unsigned char buffer[] = {0x81, 0x01,0x06, 0x01, speed, 0x00, 0x02, 0x03, 0xFF};
	if (send_net_cmd(cam, buffer, sizeof(buffer)) != 0)
	{
		printf("send_net_cmd to stu is err!!!\n");
		return -1;
	}
	else
	{
		return 0;
	}

}

static int cmd_move_stopTurn(Strategy_CamControl_t *cam)
{
	unsigned char buffer[] = {0x81, 0x01,0x06, 0x01, 0x08, 0x08, 0x03, 0x03, 0xFF};

	if (send_net_cmd(cam, buffer, sizeof(buffer)) != 0)
	{
		printf("send_net_cmd to stu is err!!!\n");
		return -1;
	}
	else
	{
		return 0;
	}

}

//nXpos:水平位置范围 538560 - 1077120
//nYpos:垂直位置范围 153600 - 307200
int cmd_move_pos(Strategy_CamControl_t *cam,int speed, int nXpos,int nYpos)
{
	unsigned char buffer[] = {0x81, 0x01,0x06, 0x02, speed, speed, 
			(nXpos>>24)&0xFF, (nXpos>>16)&0xFF, (nXpos>>8)&0xFF,(nXpos)&0xFF,
			(nYpos>>24)&0xFF, (nYpos>>16)&0xFF, (nYpos>>8)&0xFF,(nYpos)&0xFF,0xFF};
	for(int i=0;i<sizeof(buffer);i++)
	{	
		if(i<=5)continue;
		printf("%x ",buffer[i]);
	}
	printf("\n");
	if (send_net_cmd(cam, buffer, sizeof(buffer)) != 0)
	{
		printf("send_net_cmd to stu is err!!!\n");
		return -1;
	}
	else
	{
		return 0;
	}

}

int udp_move_cmd(int type, int speed, Strategy_CamControl_t *cam)
{
	int ret  = 0;
	if (NULL == cam)
	{
		ret = -1;
		printf( "udp_move_cmd param is err!!!\n");
		return ret;
	}

#ifdef SUPPORT_SONGXIA_CAM

//	strcpy(cam->connect_ip, "172.16.8.200");
	int nZoom, nPanPos, nTiltPos;
	if(cam->control_port == 9999)
	{
		switch (type)
		{
			case 0:
				ret = sonny_cmd_move_home(cam);
				break;
			case 1:
				if(cam->nStrategyType == KEYBOARD_0650KJ)
				{
					printf("KEYBOARD_0650KJ KEYBOARD_0650KJ KEYBOARD_0650KJ\n");
					ret = sonny_cmd_move_zoomIn(cam, nZoom);
				}
				else
				{
					printf("Diector Diector Diector\n");
					ret = sonny_cmd_move_zoomIn(cam, nZoom);
					usleep(160*1000);
					ret = sonny_cmd_move_stopZoom(cam);
				}
				break;
			case 2:

				if(cam->nStrategyType == KEYBOARD_0650KJ)
				{
					printf("KEYBOARD_0650KJ KEYBOARD_0650KJ KEYBOARD_0650KJ\n");
					ret = sonny_cmd_move_zoomOut(cam, nZoom);
				}
				else
				{
					printf("Diector Diector Diector\n");
					ret = sonny_cmd_move_zoomOut(cam, nZoom);
					usleep(160*1000);
					ret = sonny_cmd_move_stopZoom(cam);
				}
				break;
			case 3:
				ret = sonny_cmd_move_stopZoom(cam);
				usleep(160*1000);
				ret = sonny_cmd_move_stopZoom(cam);
				break;
			case 4:
				if(cam->nStrategyType == KEYBOARD_0650KJ)
				{
					ret = sonny_cmd_move_up(cam, speed, 1, nPanPos, nTiltPos);
				}
				else
				{
					ret = sonny_cmd_move_up(cam, speed, 1, nPanPos, nTiltPos);
					usleep(160*1000);
					ret = sonny_cmd_move_stop(cam);
				}


				break;
			case 5:
				if(cam->nStrategyType == KEYBOARD_0650KJ)
				{
					ret = sonny_cmd_move_down(cam,  speed, 1, nPanPos, nTiltPos);
				}
				else
				{
					ret = sonny_cmd_move_down(cam,  speed, 1, nPanPos, nTiltPos);
					usleep(160*1000);
					ret = sonny_cmd_move_stop(cam);
				}

				break;
			case 6:

				if(cam->nStrategyType == KEYBOARD_0650KJ)
				{
					ret = sonny_cmd_move_left(cam,  speed, 1, nPanPos, nTiltPos);
				}
				else
				{
					ret = sonny_cmd_move_left(cam,  speed, 1, nPanPos, nTiltPos);
					usleep(160*1000);
					ret = sonny_cmd_move_stop(cam);
				}
				break;
			case 7:
				if(cam->nStrategyType == KEYBOARD_0650KJ)
				{
					ret = sonny_cmd_move_right(cam,  speed, 1, nPanPos, nTiltPos);
				}
				else
				{
					ret = sonny_cmd_move_right(cam,  speed, 1, nPanPos, nTiltPos);
					usleep(160*1000);
					ret = sonny_cmd_move_stop(cam);
				}
				break;

			case 8:
				ret = sonny_cmd_move_stop(cam);
				break;
			default:
				printf( "udp we have no sunch cmd");
				break;

		}
	}



#endif

	switch (type)
	{
		case 0:
			ret = cmd_move_home(cam);
			break;
		case 1:
			ret = cmd_move_far(cam, speed);
			break;
		case 2:
			ret = cmd_move_near(cam, speed);
			break;
		case 3:
			ret = cmd_move_stopZoom(cam);
			break;
		case 4:
			ret = cmd_move_up(cam, speed);
			break;
		case 5:
			ret = cmd_move_down(cam, speed);
			break;
		case 6:
			ret = cmd_move_left(cam, speed);
			break;
		case 7:
			ret = cmd_move_right(cam, speed);
			break;
		case 8:
			ret = cmd_move_stopTurn(cam);
			break;

		default:
			printf( "udp we have no sunch cmd");
			break;

	}

	return ret;
}



Strategy_CamControl_t* init_udp_cam(char *ip, int nControlPort)
{
	Strategy_CamControl_t *stratey_cam = NULL;
	printf("IP[%s] controlPort[%d] --",ip,nControlPort);
	if (ip == NULL || strcmp(ip, "0.0.0.0") == 0 || 0 >= nControlPort)
	{
		printf( "it is empty cam ip=%s or port=%d\n", ip, nControlPort);
		return stratey_cam;
	}
	stratey_cam = malloc(sizeof(Strategy_CamControl_t));
	if(stratey_cam == NULL)
	{
		printf( " malloc it is empty cam ip=%s or port=%d\n", ip, nControlPort);
		return stratey_cam;
	}
	strcpy(stratey_cam->connect_ip, ip);
	stratey_cam->control_port = nControlPort;
	printf( "we start connect the ip=%s conPort=%d\n",  stratey_cam->connect_ip, stratey_cam->control_port);

	init_track_IpCam(stratey_cam);
	int ret = start_connect_IpCam(stratey_cam, stratey_cam->connect_ip, stratey_cam->control_port);
	if (ret == -1)
	{
		printf("init_udp_cam start_connect_IpCam ip[i]= %s conPort[i]=%d failed!\n", stratey_cam->connect_ip, stratey_cam->control_port);
		return stratey_cam;
	}
	return stratey_cam;
}
