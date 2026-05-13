/**
 * @file ModelOpt.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-03-06
 *
 * @brief Molchip模型操作
 */
#include "ModelOpt.hpp"

#include <sys/stat.h>
#include <cstring>
#include <sstream>
#include <fstream>
#define CHECK_RETURN(nRet, msg)                                                            \
    do                                                                                     \
    {                                                                                      \
        int32_t __ret_code = (nRet);                                                       \
        if (__ret_code != 0)                                                               \
        {                                                                                  \
            printf("[ERROR] %s:%d - %s failed with error code: 0x%x (%d) -------------\n", \
                   __FUNCTION__, __LINE__, msg, __ret_code, __ret_code);                   \
        }                                                                                  \
    } while (0)

Inference_NS::CModelOpt::CModelOpt(std::string strModelPath)
    : m_strModelPath(strModelPath)
{
}

Inference_NS::CModelOpt::~CModelOpt()
{
    unInit();
}

/* 设置模型路径 */
void Inference_NS::CModelOpt::setModelPath(std::string strModelPath)
{
    m_strModelPath = strModelPath;
}

/* 分配内存 */
int Inference_NS::CModelOpt::allocMmzMemory(T_FY_Mem *pMem, uint32_t nSize, uint32_t nAlign, E_FY_MemAllocType stType)
{
    int nRet = 0;
    switch (stType)
    {
    case E_FY_MEM_VMM_NO_CACHED:
        nRet = FY_MPI_SYS_MmzAlloc(&pMem->phyAddr, reinterpret_cast<void **>(&pMem->virAddr), "NPU", "anonymous", nSize);
        break;
    case E_FY_MEM_VMM_CACHED:
        nRet = FY_MPI_SYS_MmzAlloc_Cached(&pMem->phyAddr, reinterpret_cast<void **>(&pMem->virAddr), "NPU_CACHED", "anonymous", nSize);
        if (nRet == 0)
        {
            /* 刷新模型 MMZ Cache */
            FY_MPI_SYS_MmzFlushCache(pMem->phyAddr, reinterpret_cast<void *>(pMem->virAddr), pMem->size);
        }
        break;
    case E_FY_MEM_MALLOC:
        nRet = posix_memalign(reinterpret_cast<void **>(&pMem->virAddr), nAlign, nSize);
        pMem->phyAddr = reinterpret_cast<uint64_t>(pMem->virAddr);
        break;
    default:
        pMem->phyAddr = 0;
        pMem->virAddr = 0;
        nRet = -1;
        break;
    }
    pMem->size = (nRet == 0) ? nSize : 0;
    return nRet;
}

/* 定义的所有内存分段 */
int Inference_NS::CModelOpt::allocMemSegment(T_FY_MemSegmentInfo *self)
{
    int nRet = 0;
    for (int i = 0; i < self->segNum; i++)
    {
        nRet = allocMmzMemory(&self->memInfo[i].mem, self->memInfo[i].allocInfo.size,
                              self->memInfo[i].allocInfo.alignByteSize, self->memInfo[i].allocInfo.allocType);
        CHECK_RETURN(nRet, "allocMmzMemory");
    }

    return nRet;
}

/* 初始化模型 */
bool Inference_NS::CModelOpt::init()
{

    if (m_bInitialized)
    {
        /* 模型已被初始化 */
        return true;
    }
    bool bRet = false;
    int nRet = 0;

    int nModelDataSize = 0;
    int32_t nMajorVersion = 0; /* 软件主版号 */
    int32_t nMinorVersion = 0; /* 软件辅版本号 */
    int32_t nPatchVersion = 0; /* 软件补丁版本号 */

    /* 初始化 NPU 系统 */
    nRet = FY_NPU_SysInit();
    // if (nRet != 0)
    // {
    //     printf("Molchip 初始化失败 nRet=[%d]\n", nRet);
    //     bRet = false;
    //     goto EXIT;
    // }
    /* 设置日志等级 */
    FY_SDK_SetLogLevel(4);

    /* 载入模型，加入模型的虚拟内存 */
    loadModel(m_strModelPath, nModelDataSize);
    if (nModelDataSize == 0)
    {
        printf("载入模型[%s]，并转为二进制格式失败\n",m_strModelPath.c_str());
        bRet = false;
        goto EXIT;
    }

    /* 创建 NPU 模型 */
    T_FY_ModelCfgParam stModelConfig;

    nRet = FY_NPU_CreateModelFromPhyMem(&m_stModel, &stModelConfig, nullptr, &m_stModelDesc, &m_pModelHandle);
    CHECK_RETURN(nRet, "FY_NPU_CreateModelFromPhyMem");
    if (nRet != 0)
    {
        printf("创建 NPU 模型失败 [%d]\n", nRet);
        bRet = false;
        goto EXIT;
    }

    /* 计算任务内存需求 */
    T_FY_TaskCfgParam stTaskConfig;
    nRet = FY_NPU_GetTaskMemSize(m_pModelHandle, &stTaskConfig, &m_stTaskMem);
    CHECK_RETURN(nRet, "FY_NPU_GetTaskMemSize");
    if (nRet != 0)
    {
        printf("计算任务内存需求失败 [%d]\n", nRet);
        bRet = false;
        goto EXIT;
    }

    /* 分配任务内存 */
    nRet = allocMemSegment(&m_stTaskMem);
    CHECK_RETURN(nRet, "allocMemSegment (m_stTaskMem)");
    if (nRet != 0)
    {
        printf("分配任务内存失败 [%d]\n", nRet);
        bRet = false;
        goto EXIT;
    }

    /* 创建 NPU 任务 */
    nRet = FY_NPU_CreateTask(m_pModelHandle, &stTaskConfig, &m_stTaskMem, &m_pTaskHandle);
    CHECK_RETURN(nRet, "FY_NPU_CreateTask");
    if (nRet != 0)
    {
        printf("创建 NPU 任务失败 [%d]\n", nRet);
        bRet = false;
        goto EXIT;
    }

    /* 初始化成功 */
    m_bInitialized = true;

    /* SDK 的版本信息。SDK 所基于的驱动版本信息 */
    nRet = FY_SDK_GetVersion(&nMajorVersion, &nMinorVersion, &nPatchVersion);
    printf("SDK版本信息: %d.%d.%d\n", nMajorVersion, nMinorVersion, nPatchVersion);
    /* 获取NPU版本号 */
    nRet = FY_NPU_GetVersion(&nMajorVersion, &nMinorVersion, &nPatchVersion);
    printf("NPU版本信息: %d.%d.%d\n", nMajorVersion, nMinorVersion, nPatchVersion);

    /* 获取模型的输入和输出的数量 */
    printf("模型的输入数量[%d], 输出的数量[%d]\n",
           m_stModelDesc.ioDesc.inputNum,
           m_stModelDesc.ioDesc.outputNum);

    bRet = true;
EXIT:
    if (!bRet)
    {
        if (m_bInitialized)
        {
            if (m_pTaskHandle)
            {
                FY_NPU_ReleaseTask(m_pTaskHandle);
            }
            if (m_pModelHandle)
            {
                FY_NPU_ReleaseModel(m_pModelHandle);
            }
            // FY_NPU_SysExit();

            for (int i = 0; i < m_stTaskMem.segNum; i++)
            {
                FY_MPI_SYS_MmzFree(m_stTaskMem.memInfo[i].mem.phyAddr, reinterpret_cast<void *>(m_stTaskMem.memInfo[i].mem.virAddr));
            }
            for (int i = 0; i < m_stModel.segNum; i++)
            {
                FY_MPI_SYS_MmzFree(m_stModel.memInfo[i].mem.phyAddr, reinterpret_cast<void *>(m_stModel.memInfo[i].mem.virAddr));
            }

            m_bInitialized = false;
        }
    }

    return bRet;
}

/* 反初始化模型 */
bool Inference_NS::CModelOpt::unInit()
{

    if (m_bInitialized)
    {
        if (m_pTaskHandle)
        {
            FY_NPU_ReleaseTask(m_pTaskHandle);
        }
        if (m_pModelHandle)
        {
            FY_NPU_ReleaseModel(m_pModelHandle);
        }
        // FY_NPU_SysExit();

        for (int i = 0; i < m_stTaskMem.segNum; i++)
        {
            FY_MPI_SYS_MmzFree(m_stTaskMem.memInfo[i].mem.phyAddr, reinterpret_cast<void *>(m_stTaskMem.memInfo[i].mem.virAddr));
        }
        for (int i = 0; i < m_stModel.segNum; i++)
        {
            FY_MPI_SYS_MmzFree(m_stModel.memInfo[i].mem.phyAddr, reinterpret_cast<void *>(m_stModel.memInfo[i].mem.virAddr));
        }

        m_bInitialized = false;

        return true;
    }
    return false;
}

const char *dataTypeToString(int nDt)
{
    switch (nDt)
    {
    case E_FY_DT_UNDEFINED:
        return "UNDEFINED";
    case E_FY_FLOAT:
        return "FLOAT";
    case E_FY_FLOAT16:
        return "FLOAT16";
    case E_FY_INT8:
        return "INT8";
    case E_FY_INT32:
        return "INT32";
    case E_FY_UINT8:
        return "UINT8";
    case E_FY_INT16:
        return "INT16";
    case E_FY_UINT16:
        return "UINT16";
    case E_FY_UINT32:
        return "UINT32";
    case E_FY_INT64:
        return "INT64";
    case E_FY_UINT64:
        return "UINT64";
    default:
        return "Unknown";
    }
}
/* 打印模型算子信息 */
bool Inference_NS::CModelOpt::showModelOpt()
{
    int32_t nOpNum = 0;
    /* 获取NPU模型fusion-op算子数目 */
    FY_NPU_GetOpNum(m_pModelHandle, &nOpNum);
    /* 获取NPU模型fusion-op算子信息 */
    T_FY_OpDesc stOpDescList[nOpNum]; /* 外部申请空间 */
    FY_NPU_GetOpListInfo(
        m_pModelHandle,
        nOpNum,
        stOpDescList);
    printf("----------------------[模型算子信息]-------------------------\n");
    printf("%-3s\t %-40s\t %-10s\t %-40s\t %-10s\t %-40s\t %-10s\t %-10s\n",
           "id", "Name", "numInputs", "inputDesc", "numOutputs", "outputDesc", "weightInfo", "quaInfo");
    for (int i = 0; i < nOpNum; i++)
    {
        T_FY_OpDesc *stOneOp = &stOpDescList[i];
        int32_t nId = stOneOp->id;
        char *pName = stOneOp->name;
        int32_t nNumInputs = stOneOp->numInputs;
        /* 输入维度信息 */
        T_FY_BlobDesc *pInputDesc = stOneOp->inputDesc;
        std::ostringstream strInputDims;
        strInputDims << "[" << dataTypeToString(pInputDesc->tensor.dataType) << "][";
        for (int nDi = 0; nDi < pInputDesc->tensor.numDims; nDi++)
        {
            strInputDims << pInputDesc->tensor.dims[nDi];
            if (nDi < (pInputDesc->tensor.numDims - 1))
            {
                strInputDims << "x";
            }
            else
            {
                strInputDims << "]";
            }
        }
        int32_t nNumOutputs = stOneOp->numOutputs;
        /* 输出维度信息 */
        T_FY_BlobDesc *pOutputDesc = stOneOp->outputDesc;
        std::ostringstream strOutputDims;
        strOutputDims << "[" << dataTypeToString(pOutputDesc->tensor.dataType) << "][";
        for (int nDi = 0; nDi < pOutputDesc->tensor.numDims; nDi++)
        {
            strOutputDims << pInputDesc->tensor.dims[nDi];
            if (nDi < (pOutputDesc->tensor.numDims - 1))
            {
                strOutputDims << "x";
            }
            else
            {
                strOutputDims << "]";
            }
        }

        int32_t nWeightInfo = stOneOp->weightInfo.reserved; /* op 参数信息 */
        int32_t nQuaInfo = stOneOp->quaInfo.reserved;       /* op 量化信息 */

        printf("%-3d\t %-40s\t %-10d\t %-40s\t %-10d\t %-40s\t %-10d\t %-10d\n",
               nId, pName, nNumInputs, strInputDims.str().c_str(), nNumOutputs, strOutputDims.str().c_str(), nWeightInfo, nQuaInfo);
    }
    printf("--------------------------------------------------------\n");

    return true;
}

bool Inference_NS::CModelOpt::showModelNpu()
{
    printf("----------------------[NPU信息]-------------------------\n");
    /* 查询NPU能力信息 */
    T_FY_NpuCapacityInfo stCapacity;
    FY_NPU_QueryCapacity(E_FY_NPU_ID_0, &stCapacity);
    printf("nu信息: 算力[Tops]:%f\t 频率变化[Mhz]:%d/%d\t 数据格式:%s\n", stCapacity.hardware.nuTops, stCapacity.hardware.nuCurrFreq, stCapacity.hardware.nuMaxFreq, stCapacity.hardware.nuDataFmt);
    printf("vu信息: 算力[Tops]:%f\t 频率变化[Mhz]:%d/%d\t 数据格式:%s\n", stCapacity.hardware.vuTops, stCapacity.hardware.vuCurrFreq, stCapacity.hardware.vuMaxFreq, stCapacity.hardware.vuDataFmt);
    /* 查询NPU负载信息 */
    T_FY_NpuPayLoadInfo stLoad;
    FY_NPU_QueryLoad(E_FY_NPU_ID_0, &stLoad);
    printf("Task总数目[%d]  等待执行数目[%d]  剩余可用数目[%d]  负载百分比[%f]\n",
           stLoad.allTaskCnt, stLoad.waitTaskCnt, stLoad.resTaskCnt, stLoad.rate);
    printf("--------------------------------------------------------\n");

    return true;
}

/* 获取模型输入输出参数 */
bool Inference_NS::CModelOpt::getModelDesc(T_FY_ModelDesc &stModelDesc)
{
    if (m_bInitialized)
    {
        stModelDesc = m_stModelDesc;
        return true;
    }
    return false;
}

/* 释放输入参数 */
bool Inference_NS::CModelOpt::releaseInputs(std::vector<T_FY_TaskInput> &vTaskInputs)
{
    if (m_bInitialized)
    {
        for (int i = 0; i < m_stModelDesc.ioDesc.inputNum; i++)
        {
            FY_MPI_SYS_MmzFree(vTaskInputs[i].dataIn.phyAddr, reinterpret_cast<void *>(vTaskInputs[i].dataIn.virAddr));
            vTaskInputs[i].dataIn.size = 0;
        }
        return true;
    }
    return false;
}

/* 释放输出参数 */
bool Inference_NS::CModelOpt::releaseOutputs(std::vector<T_FY_TaskOutput> &vTaskOutputs)
{
    if (m_bInitialized)
    {
        for (int i = 0; i < m_stModelDesc.ioDesc.outputNum; i++)
        {
            if (vTaskOutputs[i].dataOut.size > 0)
            {
                FY_MPI_SYS_MmzFree(vTaskOutputs[i].dataOut.phyAddr, reinterpret_cast<void *>(vTaskOutputs[i].dataOut.virAddr));
                vTaskOutputs[i].dataOut.size = 0;
            }
        }
        return true;
    }
    return false;
}

/* 运行模型 */
bool Inference_NS::CModelOpt::run(
    std::vector<T_FY_TaskInput> &vTaskInputs,
    std::vector<T_FY_TaskOutput> &vTaskOutputs)
{
    int nRet = 0;

    if (m_bInitialized)
    {
        /* 刷新输入 MMZ Cache */
        for (int nIn = 0; nIn < m_stModelDesc.ioDesc.inputNum; nIn++)
        {
            nRet = FY_MPI_SYS_MmzFlushCache(
                vTaskInputs[nIn].dataIn.phyAddr,
                reinterpret_cast<void *>(vTaskInputs[0].dataIn.virAddr),
                vTaskInputs[nIn].dataIn.size);
            if (nRet != 0)
            {
                CHECK_RETURN(nRet, "flush_mmz_memory (input)");
                return false;
            }
        }

        /* 执行推理 */
        nRet = FY_NPU_Forward(m_pTaskHandle, E_FY_NPU_ID_0, m_stModelDesc.ioDesc.inputNum, vTaskInputs.data(),
                              m_stModelDesc.ioDesc.outputNum, vTaskOutputs.data());
        if (nRet != 0)
        {
            CHECK_RETURN(nRet, "FY_NPU_Forward");
            return false;
        }
        for (int nOut = 0; nOut < m_stModelDesc.ioDesc.outputNum; nOut++)
        {
            /* 刷新输出 MMZ Cache */
            nRet = FY_MPI_SYS_MmzFlushCache(
                vTaskOutputs[nOut].dataOut.phyAddr,
                reinterpret_cast<void *>(vTaskOutputs[nOut].dataOut.virAddr),
                vTaskOutputs[nOut].dataOut.size);
            if (nRet != 0)
            {
                CHECK_RETURN(nRet, "flush_mmz_memory (output)");
                return false;
            }
        }

        return true;
    }

    return false;
}

/* 载入模型 */
void Inference_NS::CModelOpt::loadModel(std::string strFileName, int &nModelSize)
{
    int nRet = 0;
    FILE *pFp = nullptr;
    int nSize = 0;

    pFp = fopen(strFileName.c_str(), "rb");
    if (nullptr == pFp)
    {
        printf("打开模型失败 [%s]\n", strFileName.c_str());
        goto EXIT;
    }

    fseek(pFp, 0, SEEK_END);
    nSize = ftell(pFp);

    nRet = fseek(pFp, 0, SEEK_SET);
    if (nRet != 0)
    {
        printf("将文件句柄设置到文件头失败\n");
        goto EXIT;
    }
    /* 恢复文件指针位置到文件开头 */
    rewind(pFp);

    /* 分配模型内存 */
    m_stModel.segNum = 1;
    m_stModel.memInfo[0].allocInfo.alignByteSize = 128;
    m_stModel.memInfo[0].allocInfo.allocType = E_FY_MEM_VMM_CACHED;
    m_stModel.memInfo[0].allocInfo.shareType = E_MEM_EXCLUSIVED;
    m_stModel.memInfo[0].allocInfo.size = nSize;
    nRet = allocMemSegment(&m_stModel);
    if (nRet != 0)
    {
        CHECK_RETURN(nRet, "allocMemSegment");
        goto EXIT;
    }
    nRet = fread(reinterpret_cast<char *>(m_stModel.memInfo[0].mem.virAddr), 1, m_stModel.memInfo[0].mem.size, pFp);
    if (nRet != nSize)
    {
        printf("读取失败\n");
        goto EXIT;
    }
    nRet = FY_MPI_SYS_MmzFlushCache(m_stModel.memInfo[0].mem.phyAddr, reinterpret_cast<void *>(m_stModel.memInfo[0].mem.virAddr), m_stModel.memInfo[0].mem.size);
    CHECK_RETURN(nRet, "FY_MPI_SYS_MmzFlushCache");

EXIT:
    nModelSize = nSize;
    if (pFp)
    {
        fclose(pFp);
        pFp = nullptr;
    }
}
