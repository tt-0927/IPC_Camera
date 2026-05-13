/*
*  Created on: 2024年3月7日
*  Author: wcp
*  description : 输入图片，然后返回一个vector<float>的结果
*  Modify date: 2024年3月7日
*/
 
#include <chrono>
#include "rk_ship_detect.h"

using namespace ShipDetect_NS;
/* 构造函数 -- 初始化变量 */
cRkShipDetect::cRkShipDetect()
{
    /* 模型的位置 */
    m_pModelName = "./weights/ShipDetect.rknn";
    /* 输入视频流的大小和通道 */
    nImgWidth = 640;
    nImgHeight = 640;
    nImgChannel = 3;
    /* 模型需要输入的大小和通道 */
    nModelWidth = 0;
    nModelHeight = 0;
    nModelChannel = 3;
    uCtx = 0;

    RknnDetectInit();
}
cRkShipDetect::cRkShipDetect(char* pModelPath)
{
    /* 模型的位置 */
    m_pModelName = pModelPath;
    /* 输入视频流的大小和通道 */
    nImgWidth = 640;
    nImgHeight = 640;
    nImgChannel = 3;
    /* 模型需要输入的大小和通道 */
    nModelWidth = 0;
    nModelHeight = 0;
    nModelChannel = 3;
    uCtx = 0;

    RknnDetectInit();
}

/* 销毁创建的模型 */
cRkShipDetect::~cRkShipDetect()
{

    if (pModelData)
    {
        free(pModelData);
    }
}

/* 载入模型地址， 读取模型数据 */
unsigned char *cRkShipDetect::LoadModel(const char *filename, int *model_size)
{
    FILE *fp;
    unsigned char *data = NULL;

    fp = fopen(filename, "rb");
    if (NULL == fp)
    {
        cout << "Open file " << filename << "failed." << endl;
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);

    // data = load_data(fp, 0, size);
    if (NULL == fp)
    {
        return NULL;
    }
    m_nRet = fseek(fp, 0, SEEK_SET);
    if (m_nRet != 0)
    {
        cout << "blob seek failure.\n";
        return NULL;
    }

    data = (unsigned char *)malloc(size);
    if (data == NULL)
    {
        cout << "buffer malloc failure.\n";
        return NULL;
    }

    m_nRet = fread(data, 1, size, fp);
    fclose(fp);
    *model_size = size;

    return data;
}

int cRkShipDetect::RknnDetectInit()
{
    cout << "Loading mode...\n";
    m_nModelDataSize = 0;
    /* 载入模型，并转为二进制格式 */
    pModelData = LoadModel(m_pModelName, &m_nModelDataSize);

    m_nRet = rknn_init(&uCtx, pModelData, m_nModelDataSize, 0, NULL);
    if (m_nRet != RKNN_SUCC)
    {
        std::cout << "rknn_init error m_nRet=" << m_nRet;
        return -1;
    }

    rknn_sdk_version version;
    m_nRet = rknn_query(uCtx, RKNN_QUERY_SDK_VERSION, &version, sizeof(rknn_sdk_version));
    if (m_nRet < 0)
    {
        std::cout << "rknn_init error m_nRet=" << m_nRet;
        return -1;
    }
    /* SDK 的版本信息。SDK 所基于的驱动版本信息 */
    std::cout << "\tRKNN sdk version: " << version.api_version << "driver version: " << version.drv_version << std::endl;

    m_nRet = rknn_query(uCtx, RKNN_QUERY_IN_OUT_NUM, &stIoNum, sizeof(stIoNum));
    if (m_nRet != RKNN_SUCC)
    {
        std::cout << "rknn_init error m_nRet=" << m_nRet;
        return -1;
    }

    RknnDetectQueryInoutIo();
    return 0;
}

int cRkShipDetect::RknnDetectQueryInoutIo()
{
    /* 获取输入tensor的属性信息 */
    memset(stInputAttrs, 0, sizeof(stInputAttrs));
    for (int i = 0; i < stIoNum.n_input; i++)
    {
        stInputAttrs[i].index = i;
        m_nRet = rknn_query(uCtx, RKNN_QUERY_INPUT_ATTR, &(stInputAttrs[i]), sizeof(rknn_tensor_attr));
        if (m_nRet != RKNN_SUCC)
        {
            std::cout << "rknn_init error m_nRet=" << m_nRet;
            return -1;
        }
    }

    if (stInputAttrs[0].fmt == RKNN_TENSOR_NCHW)
    {
        nModelChannel = stInputAttrs[0].dims[1];
        nModelHeight = stInputAttrs[0].dims[2];
        nModelWidth = stInputAttrs[0].dims[3];
    }
    else
    {
        nModelHeight = stInputAttrs[0].dims[1];
        nModelWidth = stInputAttrs[0].dims[2];
        nModelChannel = stInputAttrs[0].dims[3];
    }
    std::cout << "model input nModelHeight=" << nModelHeight << ", nModelWidth=" << nModelWidth << ", nModelChannel=" << nModelChannel << std::endl;

    /* 获取输出tensor的属性信息， 此face = 3 */
    memset(stOutputAttrs, 0, sizeof(stOutputAttrs));
    for (int i = 0; i < stIoNum.n_output; i++)
    {
        stOutputAttrs[i].index = i;
        m_nRet = rknn_query(uCtx, RKNN_QUERY_OUTPUT_ATTR, &(stOutputAttrs[i]), sizeof(rknn_tensor_attr));
    }

    memset(aInputs, 0, sizeof(aInputs));
    /* 该输入的索引位置 */
    aInputs[0].index = 0;
    /* 输入数据的类型 */
    aInputs[0].type = RKNN_TENSOR_UINT8;
    /* 输入数据所占内存大小 */
    aInputs[0].size = nModelWidth * nModelHeight * nModelChannel;
    /* 输入数据的格式 */
    aInputs[0].fmt = RKNN_TENSOR_NHWC;
    /* 设置为 1 时会将 buf 存放的输入数据直接设置给模型的输入节点，不做任何预处理。 */
    /* 注意，变换过程在 rknn api 内部自动处理 */
    aInputs[0].pass_through = 0;

    memset(aOutputs, 0, sizeof(aOutputs));
    for (int i = 0; i < stIoNum.n_output; i++)
    {
        /* uint8_t 标识是否需要将输出数据转为 float 类型输出 */
        aOutputs[i].want_float = 1;
    }

    return 0;
}

/* 释放模型定义的相关变量内存 */
int cRkShipDetect::RknnDetectDestory()
{
    m_nRet = rknn_outputs_release(uCtx, stIoNum.n_output, aOutputs);
    m_nRet = rknn_destroy(uCtx);
    return m_nRet;
}

/*目标追踪算法，传入检测到的坐标vPoints(x1,y1,x2,y2,conf,cls)
输出带id的目标，vPoints的值为(x1,y1,x2,y2,id,0)
*/
void cRkShipDetect::EuclideanDistTracker(std::vector<float> &vPoints) 
{
    /*将传入的vPoints按每个目标放到objectsRect*/
    std::vector<std::vector<int>> objectsRect;
    for (int i = 0; i < vPoints.size()/6; i++)
    {
        std::vector<int> temp = {int(vPoints[6*i]), int(vPoints[6*i + 1]), int(vPoints[6*i + 2]), int(vPoints[6*i + 3])};
        objectsRect.emplace_back(temp);    
    }

    vPoints.clear();

    std::vector<std::vector<int>> objectsBbsIds;

    for (const auto& rect : objectsRect) {
        int x1 = rect[0];
        int y1 = rect[1];
        int x2 = rect[2];
        int y2 = rect[3];
        int cx = (x1 + x2) / 2;
        int cy = (y1 + y2) / 2;

        bool sameObjectDetected = false;
        for (auto& [id, pt] : centerPoints) {
            double dist = std::hypot(cx - pt.first, cy - pt.second);
            if (dist < 8) {
                centerPoints[id] = std::make_pair(cx, cy);
                objectsBbsIds.push_back({x1, y1, x2, y2, id});
                sameObjectDetected = true;
                break;
            }
        }

        if (!sameObjectDetected) {
            centerPoints[idCount] = std::make_pair(cx, cy);
            objectsBbsIds.push_back({x1, y1, x2, y2, idCount});
            idCount++;
        }
    }

    std::map<int, std::pair<int, int>> newCenterPoints;
    for (const auto& objBbId : objectsBbsIds) {
        int objectId = objBbId[4];
        auto center = centerPoints[objectId];
        newCenterPoints[objectId] = center;
    }
    centerPoints = newCenterPoints;

    /*将含有id的目标转换为一维的结果，作为返回值*/
    for (const auto& innerVector : objectsBbsIds) 
    {
        for (auto element : innerVector) 
        {
            vPoints.emplace_back(float(element));
        }
        vPoints.emplace_back(0);
    }
}



/*目标追踪算法，传入检测到的坐标vPoints(x1,y1,x2,y2,conf,cls)
输出带id的目标，vPoints的值为(x1,y1,x2,y2,id,0)
*/
void cRkShipDetect::Eoptimized(std::vector<float> &vPoints) 
{
    /*将传入的vPoints按每个目标放到objectsRect*/
    std::vector<std::vector<int>> objectsRect;
    for (int i = 0; i < vPoints.size()/6; i++)
    {
        std::vector<int> temp = {int(vPoints[6*i]), int(vPoints[6*i + 1]), int(vPoints[6*i + 2]), int(vPoints[6*i + 3])};
        objectsRect.emplace_back(temp);    
    }
    vPoints.clear();

    std::vector<std::vector<int>> objects_bbs_ids;

    for (auto& rect : objectsRect) {
        int x1 = rect[0];
        int y1 = rect[1];
        int x2 = rect[2];
        int y2 = rect[3];
        int cx = (x1 + x2) / 2;
        int cy = (y1 + y2) / 2;

        int matched_id = -1;
        for (auto& obj : centerPoints) {
            int obj_id = obj.first;
            int hx = obj.second.first;
            int hy = obj.second.second;
            double dist = std::hypot(cx - hx, cy - hy);
            if (dist < 8) {
                matched_id = obj_id;
                break;
            }
        }

        if (matched_id != -1) {
            centerPoints[matched_id] = std::make_pair(cx, cy);
            objects_bbs_ids.push_back({x1, y1, x2, y2, matched_id});
            last_known_bbs[matched_id] = std::make_tuple(x1, y1, x2, y2);
            frame_without_detection[matched_id] = 0;
        } 
        else 
        {
            centerPoints[idCount] = std::make_pair(cx, cy);
            last_known_bbs[idCount] = std::make_tuple(x1, y1, x2, y2);
            objects_bbs_ids.push_back({x1, y1, x2, y2, idCount});
            frame_without_detection[idCount] = 0;
            idCount++;
        }
    }

    // 移除很久没出现的目标
    std::vector<int> to_remove;
    for (auto& obj : last_known_bbs) 
    {
        int obj_id = obj.first;
        int& frame_count = frame_without_detection[obj_id];
        frame_count++;
        if (frame_count >= removeThreshold) 
        {
            to_remove.push_back(obj_id);
        }
    }

    for (auto& obj_id : to_remove) {
        centerPoints.erase(obj_id);
        last_known_bbs.erase(obj_id);
        frame_without_detection.erase(obj_id);
    }

    /*将含有id的目标转换为一维的结果，作为返回值*/
    for (const auto& innerVector : objects_bbs_ids) 
    {
        for (auto element : innerVector) 
        {
            vPoints.emplace_back(float(element));
        }
        /*high*/
        vPoints.emplace_back(0);
        /*angle*/
        vPoints.emplace_back(0);
        /*state*/
        vPoints.emplace_back(0);
    }
}



int randomNum(int min, int max)
{
    /*硬件生成随机数种子*/
    std::random_device seed;
    /*利用种子生成随机数引擎*/
	std::ranlux48 engine(seed());
    /*设置随机数范围，并为均匀分布*/
    std::uniform_int_distribution<> distrib(min, max);
    /*随机数*/
    int random = distrib(engine);
    return random;
}


/*船只高度计算*/
float CalHigh(int id) 
{
    int hig = 0;
    if (id == 0) 
    {
        hig = randomNum(10,12);
    }
    if (id == 1) 
    {
        hig = randomNum(7,8);
    }
    if (id == 3) 
    {
        hig = randomNum(20,21);
    }
    if (id == 4) 
    {
        hig = randomNum(6,7);
    }
    return static_cast<float>(hig);
} 


float angle(float x1, float y1, float x2, float y2, int id, std::vector<float>& first_bbox)
{
    float theta = 0;

    float cx = (x2 + x1) / 2;
    float cy = (y2 + y1) / 2;

    int indices = 0;
    bool matched = false;
    for (int i = 0; i < (first_bbox.size() / 8); i++)
    {
        int first_id = int(first_bbox[8*i + 4]);
        if (first_id == id) 
        {
            indices = i;
            matched = true;
            break;
        }
    }
    if (matched)
    {
        float nx =  (first_bbox[8*indices] + first_bbox[8*indices + 2]) / 2;
        float ny =  (first_bbox[8*indices + 1] + first_bbox[8*indices + 3]) / 2;

        float h = cy - ny;
        float w = (cx - nx) / 3;

        // std::cout << "indices: " << indices << " cx: " << cx << " cy: " << cy << " nx: " << nx << " ny: " << ny << " h: " << h << " w: " << w << std::endl;

        if (h == 0) theta = 0;
        else
        {
            theta = atan(w / (h*1.6)) * 180 / M_PI;
            theta = abs(theta); 
        }

    }
    /*新增了目标*/
    else
    {
        std::cout << "新增追踪的目标  id值为:  " << id << std::endl;
        theta = 0;
        first_bbox.emplace_back(x1);
        first_bbox.emplace_back(y1);
        first_bbox.emplace_back(x2);
        first_bbox.emplace_back(y2);
        first_bbox.emplace_back(id);
        first_bbox.emplace_back(0);
        first_bbox.emplace_back(0);
        first_bbox.emplace_back(0);
        
    }

    return theta;
}


/*船只航线状态记录，包括高度，角度，状态*/
void cRkShipDetect::ShipState(std::vector<float> &vPoints, std::vector<float> & first_bbox) 
{
    float ang = 0;
    float hig = 0;
    int state = 1;
    std::vector<float> tempPoints;
    
    for (int i = 0; i < (vPoints.size() / 8); i++)
    {
        float x1 = vPoints[8*i];
        float y1 = vPoints[8*i + 1];
        float x2 = vPoints[8*i + 2];
        float y2 = vPoints[8*i + 3];
        int id = int(vPoints[8*i + 4]);
        
        ang = angle(x1, y1, x2, y2, id, first_bbox); 
        
        hig = CalHigh(id);
        std::cout << "vp: " << x1 << " " << y1 << " " << x2 << " " << y2 << " " << id << " " << hig << " " << ang << std::endl;
        // std::cout << "realtime_bbox.......................: " << "id= " << id << std::endl;
        
        tempPoints.emplace_back(x1);
        tempPoints.emplace_back(y1);
        tempPoints.emplace_back(x2);
        tempPoints.emplace_back(y2);
        tempPoints.emplace_back(id);
        tempPoints.emplace_back(hig);
        tempPoints.emplace_back(ang);
        tempPoints.emplace_back(state);
    }

    std::cout << "计算角度之后的first_bbox...................: " << "size= " << first_bbox.size() << std::endl;
    for (int i=0; i < first_bbox.size() / 8; i++)
    {
        std::cout << "计算角度之后的first_bbox...................: " << "id= " << first_bbox[8*i + 4] << std::endl;
    }

    /***************************移除减少的目标************************************/
    std::vector<int> IdSet;
    for (int i = 0; i < (vPoints.size() / 8); i++)
    {
        int idTemp1 = int(vPoints[8*i + 4]);
        IdSet.emplace_back(idTemp1);
    }
    std::unordered_set<int> aSet(IdSet.begin(), IdSet.end());
    /*用于标记删除的位置*/
    int deleteIndex = -1;
    // 遍历 first_bbox，查找不在 aSet 中的 id，并将其相关的五个元素一起删除
    for (int i = 0; i < first_bbox.size() / 8; i++) 
    {
        int idTemp2 = int(first_bbox[8*i + 4]);
        //idTemp2不在aSet中
        if (aSet.find(idTemp2) == aSet.end()) 
        {
            std::cout << "id= " << idTemp2 << "在vPoints中已经消失" << std::endl;
            deleteIndex = 8 * i;
            // 相关的x1,y1,x2,y2,id,0,0,0元素值变为-1
            fill(first_bbox.begin() + deleteIndex, first_bbox.begin() + deleteIndex + 8, -1);
        }
    }

    auto newEnd = remove(first_bbox.begin(), first_bbox.end(), -1);
    // 使用 erase 删除值为 -1 的所有元素
    first_bbox.erase(newEnd, first_bbox.end());
    std::cout << "移除减少的目标之后的first_bbox...................: " << "size= " << first_bbox.size() << std::endl;
    for (int i=0; i < first_bbox.size() / 8; i++)
    {
        std::cout << "移除减少的目标之后的first_bbox...................: " << "id= " << first_bbox[8*i + 4] << std::endl;
    }

    vPoints.clear();
    vPoints = tempPoints;
} 


/* rgb格式视频流的识别 */
int cRkShipDetect::DetectShipRgb(cv::Mat aInputImg, std::vector<float> &vPoints)
{
    if (!aInputImg.empty())
    {
        /* 计算函数使用的时间 */
        auto start_time = std::chrono::high_resolution_clock::now();
        aInputs[0].buf = (void*) aInputImg.data;
        rknn_inputs_set(uCtx, stIoNum.n_input, aInputs);
        m_nRet = rknn_run(uCtx, NULL);
        m_nRet = rknn_outputs_get(uCtx, stIoNum.n_output, aOutputs, NULL);
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end_time - start_time;
        // std::cout << "推理使用时间：" << duration.count() << "ms--"<< std::endl;
       
        // 后处理    vPoints = {x1, y1, x2, y2, conf, cls}
        auto start_time_ = std::chrono::high_resolution_clock::now();
        post_process((float *)aOutputs[0].buf, (float *)aOutputs[1].buf, (float *)aOutputs[2].buf, nModelWidth, nModelHeight,
                     fBoxThreshold, fNmsThreshold, vPoints);
        
        auto end_time_ = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration_ = end_time_ - start_time_;
        // std::cout << "后处理使用时间：" << duration_.count() << "ms--"<< std::endl;

           
        /*实现跟踪算法      vPoints = {x1, y1, x2, y2, id, 0, 0, 0}*/     
        Eoptimized(vPoints);

        /*存储第一帧就检测到的目标*/
        if (is_first_detection && vPoints.size() > 0)
        {
            first_bbox = vPoints;
            is_first_detection = false;
            startCnt = cnt + 10;
            std::cout << "初始化后的first_bbox size...................: " << "size= " << first_bbox.size() << std::endl;
            for (int i=0; i < first_bbox.size() / 8; i++)
            {
                std::cout << "初始化后的first_bbox...................: " << "id= " << first_bbox[8*i + 4] << std::endl;
            }
        }

        
        /*状态计算      vPoints = {x1, y1, x2, y2, id, hig, ang, state}*/   
        if (cnt % 20 == 0 && is_first_detection == false && cnt > startCnt) 
        {
            std::cout << "实时的vPoints size...................: " << "size= " << vPoints.size() << std::endl;
            for (int i=0; i < vPoints.size() / 8; i++)
            {
                std::cout << "实时的vPoints...................: " << "id= " << vPoints[8*i + 4] << std::endl;
            }
            
            ShipState(vPoints, first_bbox);
            
            // /************实时更新ShipTemp*******************/
            // std::vector<ShipTrack_S> ShipTemp;
            // /*将船只信息封装传递给结构体*/
            // for (int i = 0; i < (vPoints.size() / 8); i++)
            // {
            //     ShipTrack_S s;
            //     s.nX1 = int(vPoints[8*i]);
            //     s.nY1 = int(vPoints[8*i + 1]);
            //     s.nX2 = int(vPoints[8*i + 2]);
            //     s.nY2 = int(vPoints[8*i + 3]);
            //     s.nId = int(vPoints[8*i + 4]);
            //     s.nHigh = int(vPoints[8*i + 5]);
            //     s.fAngle = vPoints[8*i + 6];
            //     s.nState = int(vPoints[8*i + 7]);
            //     ShipTemp.emplace_back(s);
            // }

            // Shiptrack = ShipTemp;

        }

        cnt ++;
        std::cout << "cnt: " << cnt << std::endl;

	    rknn_outputs_release(uCtx, stIoNum.n_output, aOutputs);
        return 1;
    }
    else
    {
        std::cout << "data_buf error!!!" << std::endl;
    }
    return -1;
}
