/**
 * @FilePath     : svp_ai_detect.c
 * @Author       : zhouzirui
 * @Date         : 2025-04-28 19:22:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-13 10:33:35
 * @Description  : 海思ai检测模块封装
 */

#include "svp_ai_detect.h"
#include "mpi_common.h"

static td_char gs_aClassTypes[OT_AIDETECT_CLASS_BUTT][SVP_AIDETECT_BUFFER_LEN] = {
    "人脸",
    "人形",
    "机动车",
    "宠物(主要是猫狗)",
    "垃圾(主要是垃圾袋)",
    "包裹(快递包裹、书包)",
    "钱包",
    "手机"
};

static td_char gs_aTrackStatus[OT_AIDETECT_TRACK_STATUS_BUTT][SVP_AIDETECT_BUFFER_LEN] = {
    "单目标首次跟踪",
    "已跟踪上的目标状态更新",
    "当前目标断开跟踪",
    "未开启跟踪"
};

/**
 * @brief       : 读取模型文件到内存
 * @author      : zhouzirui
 * @param        {td_char} *path：模型文件路径
 * @param        {td_u32} *len：读取后模型的数据长度
 * @return       {*}空：失败 非空：成功
 */
static td_char *svpAiDetect_readModelFile(const td_char *path, td_u32 *len)
{
    FILE *fd = TD_NULL;
    td_char *data = TD_NULL;

    fd = fopen(path, "rb");
    if (fd == TD_NULL)
    {
        return TD_NULL;
    }

    (td_void) fseek(fd, 0, SEEK_END);
    *len = (td_u32)ftell(fd);
    (td_void) fseek(fd, 0, SEEK_SET);
    data = (td_char *)malloc(*len);
    if (data == TD_NULL)
    {
        (td_void) fclose(fd);
        return TD_NULL;
    }
    (td_void) memset(data, 0, *len);
    (td_void) fread(data, *len, 1, fd);
    (td_void) fclose(fd);
    fd = TD_NULL;

    return data;
}

/**
 * @brief       : 初始化结果结构体
 * @author      : zhouzirui
 * @param        {ot_aidetect_result_array} *pResult：指向AI检测结果结构体
 * @param        {ot_aidetect_model_info} stModelInfo：模型信息
 */
static void svpAiDetect_initResult(ot_aidetect_result_array *pResult, ot_aidetect_model_info stModelInfo)
{
    memset(pResult, 0, sizeof(ot_aidetect_result_array));
    pResult->class_num = (stModelInfo.class_num > OT_AIDETECT_CLASS_BUTT ? OT_AIDETECT_CLASS_BUTT : stModelInfo.class_num);
    for (size_t i = 0; i < pResult->class_num; ++i)
    {
        pResult->object_class[i].class_type = stModelInfo.classes[i];
        pResult->object_class[i].object_capacity = SVP_AIDETECT_MAX_OUTPUT_RECT_NUM;
        pResult->object_class[i].objects =
            (ot_aidetect_object *)malloc(sizeof(ot_aidetect_object) * pResult->object_class[i].object_capacity);
        if (pResult->object_class[i].objects == TD_NULL)
        {
            continue;
        }
        memset(pResult->object_class[i].objects, 0, sizeof(ot_aidetect_object) * pResult->object_class[i].object_capacity);
    }
}

/**
 * @brief       : 释放结果结构体
 * @author      : zhouzirui
 * @param        {ot_aidetect_result_array} *pResult：指向AI检测结果结构体
 */
static void svpAiDetect_freeResult(ot_aidetect_result_array *pResult)
{
    if (pResult == TD_NULL)
        return;
    for (size_t i = 0; i < pResult->class_num; ++i)
    {
        if (pResult->object_class[i].objects != TD_NULL)
        {
            free(pResult->object_class[i].objects);
            pResult->object_class[i].objects = TD_NULL;
        }
    }
}

/**
 * @brief       : 清除ai检测结果
 * @author      : zhouzirui
 * @param        {ot_aidetect_result_array} *pResult：ai检测结果
 */
static void svpAiDetect_clearResult(ot_aidetect_result_array *pResult)
{
    /*清除结果*/
    for (size_t i = 0; i < pResult->class_num; ++i)
    {
        pResult->object_class[i].object_num = 0;
        memset(pResult->object_class[i].objects, 0, sizeof(ot_aidetect_object) * pResult->object_class[i].object_capacity);
    }
}

/**
 * @brief       : 打印ai检测结果
 * @author      : zhouzirui
 * @param        {ot_aidetect_result_array} *pResult：ai检测结果
 */
static void svpAiDetect_printResult(ot_aidetect_result_array *pResult)
{
    uint32_t i = 0, j = 0;
    // mpi_ai_detect_log("class_num:%d",pResult->class_num);
    for (i = 0; i < pResult->class_num; ++i)
    {
        // mpi_ai_detect_log("object_num:%d",pResult->object_class[i].object_num);
        for (j = 0; j < pResult->object_class[i].object_num; j++)
        {
            /*当前目标断开跟踪*/
            if (pResult->object_class[i].objects[j].track_status == OT_AIDETECT_TRACK_STATUS_DIE)
            {
                mpi_ai_detect_log("检测类型: %s, 跟踪ID :%u 断开跟踪,坐标:[%u,%u,%u,%u]",
                        gs_aClassTypes[pResult->object_class[i].class_type], pResult->object_class[i].objects[j].track_id,
                        pResult->object_class[i].objects[j].detect_rect.x, pResult->object_class[i].objects[j].detect_rect.y,
                        pResult->object_class[i].objects[j].detect_rect.width,
                        pResult->object_class[i].objects[j].detect_rect.height);
                continue;
            }
            mpi_ai_detect_log("{检测类型: %s, 坐标[%u,%u,%u,%u], 跟踪ID: %u,跟踪状态: %s[%d], 置信度(0,1): %f}",
                    gs_aClassTypes[pResult->object_class[i].class_type], pResult->object_class[i].objects[j].detect_rect.x,
                    pResult->object_class[i].objects[j].detect_rect.y, pResult->object_class[i].objects[j].detect_rect.width,
                    pResult->object_class[i].objects[j].detect_rect.height, pResult->object_class[i].objects[j].track_id,
                    gs_aTrackStatus[pResult->object_class[i].objects[j].track_status],
                    pResult->object_class[i].objects[j].track_status, pResult->object_class[i].objects[j].detect_confidence);
        }
    }
}

/**
 * @brief       : ai检测初始化
 * @author      : zhouzirui
 * @param        {HiAiDetect_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int svpAiDetect_init(HiAiDetect_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    HiAiDetectNeedParam_S *pNeedParam = &pHandle->stNeedParam;

    td_char *pModelData = TD_NULL;
    td_u32 u32ModelLen = 0;
    /*输入模型信息*/
    ot_aidetect_input_model stInputModel;
    /*通道属性信息*/
    ot_aidetect_chn_attr stChnAttr;
    memset(&stInputModel, 0, sizeof(ot_aidetect_input_model));
    /*判断模型加载方式*/
    if (OT_AIDETECT_MODEL_LOAD_FROM_PATH == pNeedParam->enModelLoadMode)
    {
        stInputModel.model_load_mode = OT_AIDETECT_MODEL_LOAD_FROM_PATH; //模型加载方式 从路径加载模型
        stInputModel.model = (td_void *)pNeedParam->aModelPath; //模型数据，路径字符串
        stInputModel.size = (td_u32)strlen(pNeedParam->aModelPath); //路径字符串长度
    }
    else if (OT_AIDETECT_MODEL_LOAD_FROM_MEMORY == pNeedParam->enModelLoadMode)
    {
        pModelData = svpAiDetect_readModelFile(pNeedParam->aModelPath, &u32ModelLen);
        if (TD_NULL == pModelData)
        {
            mpi_ai_detect_log("读取模型 [%s] 数据失败", pNeedParam->aModelPath);
            return TD_FAILURE;
        }
        stInputModel.model_load_mode = OT_AIDETECT_MODEL_LOAD_FROM_MEMORY; //模型加载方式 从内存加载模型
        stInputModel.model = pModelData; //模型数据，内存字符串
        stInputModel.size = u32ModelLen; //内存大小
    }

    memset(&stChnAttr, 0, sizeof(ot_aidetect_chn_attr));
    //step 创建检测通道并设置通道的属性参数，通道的属性包含是否开启跟踪
    CHECK_API_RETURN(ss_mpi_aidetect_create_chn(pNeedParam->nChn, &stInputModel, &stChnAttr));

    /*创建检测通道后释放模型数据*/
    if (pModelData != TD_NULL)
    {
        free(pModelData);
        pModelData = TD_NULL;
    }

    /*检测模型信息*/
    ot_aidetect_model_info stModelInfo;
    memset(&stModelInfo, 0, sizeof(ot_aidetect_model_info));
    //step 获取模型相关信息，包括当前通道模型输入分辨率以及模型支持的检测类型
    CHECK_API_RETURN(ss_mpi_aidetect_get_model_info(pNeedParam->nChn, &stModelInfo));

    mpi_ai_detect_log("输入图像 w:%u,h:%u, 检测模型支持的目标个数: %u", stModelInfo.size.width, stModelInfo.size.height, stModelInfo.class_num);

    memset(&stChnAttr, 0, sizeof(ot_aidetect_chn_attr));
    stChnAttr.track_class_num = stModelInfo.class_num; //跟踪的类别数量
    for (size_t i = 0; i < stModelInfo.class_num; i++)
    {
        mpi_ai_detect_log("检测模型支持的目标信息:%s[%d]", gs_aClassTypes[stModelInfo.classes[i]],
                (td_s32)stModelInfo.classes[i]);
        stChnAttr.track_class_attr[i].class_type = stModelInfo.classes[i]; //目标分类类型
        stChnAttr.track_class_attr[i].track_en = TD_TRUE; //当前分类是否开启跟踪，默认不开启
    }
    //step 设置通道属性，包括通道对应模型的检测分类是否开启跟踪
    if(TD_SUCCESS != ss_mpi_aidetect_set_chn_attr(pNeedParam->nChn, &stChnAttr))
    {
        //step 销毁当前通道
        CHECK_API_RETURN(ss_mpi_aidetect_destroy_chn(pNeedParam->nChn));
        return TD_FAILURE;
    }

    /*通道参数*/
    ot_aidetect_chn_param chn_param;
    memset(&chn_param, 0, sizeof(ot_aidetect_chn_param));
    //step 获取通道参数，包括当前通道对应模型的阈值，模型执行的优先级，打印出来
    CHECK_API_RETURN(ss_mpi_aidetect_get_chn_param(pNeedParam->nChn, &chn_param));

    for (size_t i = 0; i < chn_param.detect_threshold_num; i++)
    {
        mpi_ai_detect_log("目标类型:%s,目标的阈值:%f(0,1), 目标断开跟踪的帧数:%u,模型优先级:[%d,%u,%u,%u]",
                gs_aClassTypes[chn_param.detect_threshold[i].class_type], chn_param.detect_threshold[i].detect_threshold,
                chn_param.detect_threshold[i].track_miss_frame_num,
                chn_param.model_priority.preemp_en,                // 是否使能可抢占低优先级任务，默认开启抢占
                chn_param.model_priority.priority,                 // 模型执行优先级，高优先级任务会优先调度，范围：[0,7]，默认值为3。数字越小，优先级越高
                chn_param.model_priority.priority_up_step_timeout, // 模型执行优先级逐步提升超时时间(单位毫秒)，范围：0或[15，600000），为0代表不开启优先级逐步提升
                chn_param.model_priority.priority_up_top_timeout   // 模型执行优先级最高提升超时时间(单位毫秒)，范围：0或[15，600000，为0代表不开启优先级最高提升
        );
    }

    /*初始化结果结构体*/
    svpAiDetect_initResult(&pHandle->stResult, stModelInfo);

    return TD_SUCCESS;
}

/**
 * @brief       : ai检测去初始化
 * @author      : zhouzirui
 * @param        {HiAiDetect_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int svpAiDetect_uninit(HiAiDetect_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    /*释放结果结构体*/
    svpAiDetect_freeResult(&pHandle->stResult);

    //step 销毁当前通道
    CHECK_API_RETURN(ss_mpi_aidetect_destroy_chn(pHandle->stNeedParam.nChn));

    return TD_SUCCESS;
}

/**
 * @brief       : 送帧给ai进行检测处理
 * @author      : zhouzirui
 * @param        {HiAiDetect_S} *pHandle：句柄
 * @param        {ot_video_frame} *pFrame：帧数据指针
 * @param        {ot_aidetect_result_array} *pResult：ai检测结果
 * @return       {*}成功返回0,失败返回-1
 */
static int svpAiDetect_sendFrame(HiAiDetect_S *pHandle, ot_video_frame *pFrame)
{
    if (NULL == pHandle || NULL == pFrame)
    {
        return TD_FAILURE;
    }

    /*当前通道状态信息*/
    // ot_aidetect_chn_status stChStatus;
    // struct timeval start, end;

    // //step 获取当前通道状态，包括当前通道帧率、接收总帧数等信息
    // if (0 == (u32IndexNum % SVP_AIDETECT_INDEX_NUM)) {
    //     CHECK_API_RETURN(ss_mpi_aidetect_query_status(pHandle->stNeedParam.nChn, &stChStatus));
    //     // mpi_ai_detect_log("接收的总帧数:%u,平均帧率:%u, 当前帧率:%u",
    //     //         stChStatus.recv_frames, stChStatus.avg_frame_rate, stChStatus.frame_rate);
    // }

    //step 算法分析处理，输入通道、帧信息，输出检测、跟踪结果信息
    // gettimeofday(&start, TD_NULL);
    CHECK_API_RETURN(ss_mpi_aidetect_process(pHandle->stNeedParam.nChn, pFrame, &pHandle->stResult));
    // gettimeofday(&end, TD_NULL);

    // td_u64 timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
    // mpi_ai_detect_log("帧数: %u, 算法分析处理成功 使用的时间: %f ms.", u32IndexNum, timeuse / 1000.f);
    // svpAiDetect_printResult(pResult, u32IndexNum);

    return TD_SUCCESS;
}

HiAiDetect_S *svpAiDetect_alloc(HiAiDetectNeedParam_S stNeedParam)
{
    HiAiDetect_S *pHandle = (HiAiDetect_S *)malloc(sizeof(HiAiDetect_S));
    memset(pHandle, 0, sizeof(HiAiDetect_S));

    //info /**********************必需参数***************************/
    pHandle->stNeedParam.nChn                   = stNeedParam.nChn;
    pHandle->stNeedParam.enModelLoadMode        = stNeedParam.enModelLoadMode;
    snprintf(pHandle->stNeedParam.aModelPath, sizeof(pHandle->stNeedParam.aModelPath), "%s", stNeedParam.aModelPath);
    
    //info /**********************功能参数***************************/

    //info /**********************函数列表***************************/
    pHandle->svpAiDetect_init                   = svpAiDetect_init;
    pHandle->svpAiDetect_uninit                 = svpAiDetect_uninit;
    pHandle->svpAiDetect_sendFrame              = svpAiDetect_sendFrame;
    pHandle->svpAiDetect_initResult             = svpAiDetect_initResult;
    pHandle->svpAiDetect_freeResult             = svpAiDetect_freeResult;
    pHandle->svpAiDetect_clearResult            = svpAiDetect_clearResult;
    pHandle->svpAiDetect_printResult            = svpAiDetect_printResult;

    return pHandle;
}

void svpAiDetect_release(HiAiDetect_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
}