/*
 * @Author       : chenchl
 * @Date         : 2023-11-15 13:46:06
 * @LastEditors  : chenchl
 * @LastEditTime : 2023-11-15 15:40:38
 * @FilePath     : ai_face_recognition_interface.cpp
 * @Description  : 人脸识别的c调用c++接口
 */
#include "ai_face_recognition_interface.h"
#include "rk_face_feature.h"
#include "rk_faces_detect.h"
#include "rk_retinaface_facenet128.h"
#include <vector>
#include <list>
extern "C"{
#include "featureDB/include/ctrl_db.h"
#include <dirent.h>
}


/* 一组数据的大小 */
#define FACE_DATA_GROUP_SIZE 1


/* 按顺序存储分析完毕的数据 */
std::list<std::vector<PEOPLEFEATURES>> g_listFacePos;
/*人脸检测*/ 
RK_FACES_DETECT faceDetect("/opt/bl/model/MB_FaceDetector1920x1024.rknn");
/*人脸特征提取*/ 
RK_FACE_FEATURE featureNet("/opt/bl/model/MB_facenet160x160.rknn");
/*人脸识别辅助类*/ 
RETINAFACE_FACENET *g_pRetinafaceFacenet = NULL;
/* 人脸库提取的数据 */
std::vector<PEOPLEFEATURES> vDBAllFeature;
/*存放人脸特征位置*/ 
std::vector<int> vTarget;
/* 人脸相识度 */
float g_fFacial_recognition = 0.75;

/*人脸识别分析初始化*/
int face_recognition_init()
{
        g_pRetinafaceFacenet = new RETINAFACE_FACENET();
        if (g_listFacePos.size() > 0)
        {
            g_listFacePos.clear();
        }

    return 0;
}

/*人脸识别分析反初始化*/
int face_recognition_uninit()
{
    if (g_pRetinafaceFacenet)
    {
        delete g_pRetinafaceFacenet;
        g_pRetinafaceFacenet = NULL;
        if (g_listFacePos.size() > 0)
        {
            g_listFacePos.clear();
        }
    }
    return 0;
}

int face_recognition_get_pos(AiFaceFeaturesePos_S **pFacePos, int *pTotal)
{

    if (g_listFacePos.size() > 0)
    {
        /* 获取首个元素数据 */
        auto vec = g_listFacePos.front();
        /* 移除链表 */
        g_listFacePos.pop_front();

        /* TODO:转化操作 */
        int nVecSize = vec.size() / FACE_DATA_GROUP_SIZE;
        printf("FACE_DATA_GROUP_SIZE========vec.size()=====%ld %d\n",vec.size(),nVecSize);

        int nMallocSize = sizeof(AiFaceFeaturesePos_S) * (nVecSize);
        *pFacePos = (AiFaceFeaturesePos_S *)malloc(nMallocSize);
        memset(*pFacePos, 0, nMallocSize);
        int nIndex = 0;
        for (nIndex = 0; nIndex < nVecSize; nIndex++)
        {

            if (vec.size() > (nIndex * FACE_DATA_GROUP_SIZE) )
            {
                memcpy((*pFacePos)[nIndex].fPos, vec[(nIndex * FACE_DATA_GROUP_SIZE)].fBoxs, sizeof(vec[(nIndex * FACE_DATA_GROUP_SIZE)].fBoxs) );
                memcpy((*pFacePos)[nIndex].fFeatures, vec[(nIndex * FACE_DATA_GROUP_SIZE)].fFeatures, sizeof(vec[(nIndex * FACE_DATA_GROUP_SIZE)].fFeatures));
                memcpy((*pFacePos)[nIndex].achName, vec[(nIndex * FACE_DATA_GROUP_SIZE)].achName,sizeof(vec[(nIndex * FACE_DATA_GROUP_SIZE)].achName));

                (*pFacePos)[nIndex].nResult = vec[nIndex * FACE_DATA_GROUP_SIZE].nResult;
                printf("ec[face_recognition_get_pos]==---------%d----=%s   %f\n",nIndex,
                    vec[nIndex * FACE_DATA_GROUP_SIZE].achName,vec[nIndex * FACE_DATA_GROUP_SIZE].fFeatures[0]);
            }
        }
        if(pTotal)
        {
            *pTotal = nVecSize;
        }
        /* 返回已获取的坐标组个数 */
        return nIndex;
    }
    return 0;
}

/*将新的人脸数据封装成JSON格式插入总库*/
int getJson(std::vector<PEOPLEFEATURES> vOneFeature)
{
    /*用随机数为人脸特征库的随机ID*/
    int nSendCode =rand() % 100;

    /* 组装json报文 */
    cJSON *pRoot = cJSON_CreateObject();
    cJSON *pArryRoot = cJSON_CreateArray();
    cJSON *precvRoot = cJSON_CreateObject();
    cJSON_AddNumberToObject(precvRoot, "featuresID", nSendCode);
    cJSON_AddStringToObject(precvRoot, "Name", vOneFeature.data()[0].achName);

    cJSON_AddNumberToObject(precvRoot, "PosX1", vOneFeature.data()[0].fBoxs[0]);
    cJSON_AddNumberToObject(precvRoot, "PosY1", vOneFeature.data()[0].fBoxs[1]);
    cJSON_AddNumberToObject(precvRoot, "PosX2", vOneFeature.data()[0].fBoxs[2]);
    cJSON_AddNumberToObject(precvRoot, "PosY2", vOneFeature.data()[0].fBoxs[3]);
    cJSON_AddItemToArray(pArryRoot, precvRoot);

    char* pConcatenated = (char*)calloc(2048, sizeof(char));
    if (pConcatenated == NULL)
    {
        fprintf(stderr, "Failed to allocate memory\n");
        return NULL;
    }
    int nSize =sizeof(vOneFeature.data()[0].fFeatures)/sizeof(vOneFeature.data()[0].fFeatures[0]);
    /*拼接 float128 数据*/ 
    for (int nIndex = 0; nIndex < nSize; nIndex++)
     {
        char achStr[2048];
        snprintf(achStr, sizeof(achStr), "%f/", vOneFeature.data()[0].fFeatures[nIndex]);
        strcat(pConcatenated, achStr);
    }
    cJSON_AddStringToObject(precvRoot, "Data", pConcatenated);

    cJSON_AddItemToObject(pRoot, "recvvalue", pArryRoot);

    /*将人脸特征信息写入数据库*/
    int nRet = insert_db(DB_FEATURE_INFO,pArryRoot);
    if (nRet < 0)
    {
        printf("写入数据库失败\n");
    }

    char *pJsonStr = cJSON_Print(pRoot);
    // printf("pJsonStr=======================%s\n", pJsonStr);
    if (pJsonStr)
    {
        free(pJsonStr);
        pJsonStr = NULL;
    }

    cJSON_Delete(pRoot);
    return 0;
}
int getSplitString(char *path, char *pAchName)
{
    char *pAchtoken;
    char *pAchpath = path;
    pAchtoken = strsep(&pAchpath, ".");
    strcpy(pAchName, pAchtoken);
    return 0;
}

/*保存特征数据库*/
void AISaveFeature()
{
    /*文件夹路径*/     
    char* pAchFolderPath = "/opt/bl/pic";
    DIR *dir;
    struct dirent *entry;

    /*打开目录*/ 
    dir = opendir(pAchFolderPath);
    if (dir == NULL) {
        printf("Failed to open directory: %s\n", pAchFolderPath);
        return;
    }
    /*遍历目录中的所有文件*/ 
    while ((entry = readdir(dir)) != NULL) 
    {
        /*检查文件是否是图像文件*/ 
        int len = strlen(entry->d_name);
        if (len >= 4 && strcmp(entry->d_name + len - 4, ".jpg") == 0 ||
            len >= 4 && strcmp(entry->d_name + len - 4, ".png") == 0)
        {
            /*创建图像文件的完整路径*/ 
            char imagePath[256];
            strcpy(imagePath, pAchFolderPath);
            strcat(imagePath, "/");

            strcat(imagePath, entry->d_name);
            /*读取图像文件*/ 
            cv::Mat image = cv::imread(imagePath);

            if (image.empty()) {
                /*图像为空的处理逻辑*/ 
                std::cout << "输入图像为空" << std::endl;
            }
            if (image.cols <= 0 || image.rows <= 0) {
                /*图像尺寸不符合条件的处理逻辑*/ 
                std::cout << "输入图像尺寸不符合条件" << std::endl;
            }


            /*在这里可以对图像进行处理或分析*/ 
            cv::resize(image, image, cv::Size(1920,1024));

            /* AI算法-在这里可以对图像进行处理或分析 */
            std::vector<PEOPLEFEATURES> vPicFeature;
            char achImageName[256];
            getSplitString(entry->d_name,achImageName);

            g_pRetinafaceFacenet->RetinafaceFacenetBgr((char*)image.data, faceDetect, featureNet ,vPicFeature);
            /*将新的人脸数据插入总库*/ 
            if(vPicFeature.size() == 1)
            {
                /* 数据库写入 
                @ vPicFeature 这个是每个人图片生成的数据，格式为 PEOPLEFEATURES（包含人脸坐标和人脸特征）
                @ 将vPicFeature写入数据库
                        float fBoxs[4];
                        float fFeatures[128];
                        char achName[128];
                        int nResult;
                */
                strcpy(vPicFeature.data()[0].achName, achImageName);
                getJson(vPicFeature);

            }
    	    else
            {
                std::cout <<imagePath<<" 图片有多人或者没有。 "<<std::endl;
            }
            std::cout << "vPicFeature.size()^^^^^^人脸个数^^^^^^^^^^^^^^^^^^" <<vPicFeature.size()<<" \n^^^^^^^^^^imagePath……图片路径^^^^^^^^^^^^^\n"<<imagePath<<std::endl;
            
            
            
        }
    }
    // 关闭目录
    closedir(dir);
}

/* 读取数据库的人脸特征 */
void GetAllFeature()
{
    vDBAllFeature.clear();
    cJSON * pCson = query_all_db(DB_FEATURE_INFO);
    // printf("pJsonStr=======================%s\n", cJSON_Print(pCson));
    int nArray_size = cJSON_GetArraySize(pCson);
    // printf( "nArray_size=============%d\n", nArray_size );
    for(int nNum = 0;nNum < nArray_size;nNum++)
    {
        /* 读取一个人的信息 */
        PEOPLEFEATURES vOne;
        char achID[30]={0};
        char achPosX1[30] ={0};
        char achPosY1[30] ={0};
        char achPosX2[30] ={0};
        char achPosY2[30] ={0};
        char achName[256] ={0};
        char achdata[2048]={0};
        cJSON *item = cJSON_GetArrayItem(pCson, nNum);
        cJSON *cfeaturesID = cJSON_GetObjectItem(item,"featuresID");
        cJSON *PosX1 = cJSON_GetObjectItem(item,"PosX1");
        cJSON *PosY1 = cJSON_GetObjectItem(item,"PosY1");
        cJSON *PosX2 = cJSON_GetObjectItem(item,"PosX2");
        cJSON *PosY2 = cJSON_GetObjectItem(item,"PosY2");
        cJSON *cName= cJSON_GetObjectItem(item,"Name");
        cJSON *cdata= cJSON_GetObjectItem(item,"data");

        memcpy(achID, cfeaturesID->valuestring, strlen(cfeaturesID->valuestring));
        memcpy(achPosX1, PosX1->valuestring, strlen(PosX1->valuestring));
        memcpy(achPosY1, PosY1->valuestring, strlen(PosY1->valuestring));
        memcpy(achPosX2, PosX2->valuestring, strlen(PosX2->valuestring));
        memcpy(achPosY2, PosY2->valuestring, strlen(PosY2->valuestring));
        memcpy(achName, cName->valuestring, strlen(cName->valuestring));
        memcpy(achdata, cdata->valuestring, strlen(cdata->valuestring));
        int nID = atoi(achID);
        // printf("nArray_size======nID=======%d  len== %ld   \n",nID ,strlen(cdata->valuestring));
        vOne.fBoxs[0] = atof(achPosX1);
        vOne.fBoxs[1] = atof(achPosY1);
        vOne.fBoxs[2] = atof(achPosX2);
        vOne.fBoxs[3] = atof(achPosX2);
        strcpy(vOne.achName,achName);
        char delims[] = "/";
        char *pResult = NULL;
        pResult = strtok( achdata, delims );
        int nFeaNum = 0;
        while(pResult!= NULL)
        {
            // printf( "pResult=============%s\n", pResult );
            vOne.fFeatures[nFeaNum] = atof(pResult);
            nFeaNum++;
            if(nFeaNum ==128)
                nFeaNum = 0;
            pResult = strtok( NULL, delims );
        }

        vDBAllFeature.push_back(vOne);
        // printf( "vDBAllFeature=====ssss========%ld\n", vDBAllFeature.size() );

    }
    
}

/*人脸特征库初始化*/
int shareDBInit()
{
    /*初始化数据库*/
    char achDbName[64] = {0};
    snprintf(achDbName,sizeof(achDbName),"%s/%s",DB_PATH,DB_NAME);
    db_init(achDbName,DB_FEATURE_INFO);
    return 0;

}

/*送图像数据进行分析*/
int face_recognition_send_image(char *pImageData)
{
    if (g_pRetinafaceFacenet)
    {
        /* 人脸库提取的数据 */
        std::vector<PEOPLEFEATURES> vOneFeature;
        /* 送分析 */
        g_pRetinafaceFacenet->RetinafaceFacenetBgr(pImageData, faceDetect, featureNet ,vOneFeature);

        printf("vOneFeature.sise()=============%ld  vDBAllFeature====%ld\n",vOneFeature.size(),vDBAllFeature.size());
		/* 识别到的人脸与人脸数据库做余弦相似度，取最大值 */
		vTarget.clear();
        /*捕捉到有人才进去匹配*/
        if(vOneFeature.size() > 0 && vDBAllFeature.size()> 0)
        {
            for(int nIndex = 0;nIndex < vDBAllFeature.size();nIndex++)
            {
                vTarget.push_back(-1);
                float fS = -1;
                for(int nNum = 0 ;nNum < vOneFeature.size();nNum++)
                {
                    float fSimilarity = g_pRetinafaceFacenet->CosineSimilarity(vDBAllFeature[nIndex].fFeatures,vOneFeature[nNum].fFeatures,128);
                    std::cout <<"nIndex=="<<nIndex<< "==========fSimilarity=======" <<fSimilarity<<std::endl;
                    if(fSimilarity > fS)
                    {
                        fS = fSimilarity;
                        vTarget[nIndex] = nNum;
                    }
                }
                if(fS < g_fFacial_recognition)
                {
                    vTarget[nIndex] = -1;

                }
                else
                {
                    /*人脸坐标赋值*/ 
                    for(int nNum = 0;nNum < 4 ; nNum++)
                    {
                        vDBAllFeature[nIndex].fBoxs[nNum] = vOneFeature[vTarget[nIndex]].fBoxs[nNum];
                    }
                    vOneFeature[vTarget[nIndex]].nResult = 1;
                    strcpy(vOneFeature[vTarget[nIndex]].achName,vDBAllFeature[nIndex].achName);

                }
            }
        }

        /*用之前清理掉*/
        g_listFacePos.clear();

        /* 存储分析完毕的数据 */
        g_listFacePos.push_back(vOneFeature);
        return 0;
    }
    return -1;
}