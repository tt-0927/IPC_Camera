#pragma once

#include "dcl.h"
#include "dcl_memory.h"

#define DCLMDL_DESC_FREE(ptr)             \
    do                                    \
    {                                     \
        if (ptr)                          \
        {                                 \
            (void)dclmdlDestroyDesc(ptr); \
            ptr = nullptr;                \
        }                                 \
    } while (0)

#define DCLMDL_DATASET_FREE(ptr)             \
    do                                       \
    {                                        \
        if (ptr)                             \
        {                                    \
            (void)dclmdlDestroyDataset(ptr); \
            ptr = nullptr;                   \
        }                                    \
    } while (0)

#define DCLMDL_DATABUFFER_FREE(ptr)          \
    do                                       \
    {                                        \
        if (ptr)                             \
        {                                    \
            (void)dclDestroyDataBuffer(ptr); \
            ptr = nullptr;                   \
        }                                    \
    } while (0)

#define DCLRT_FREE(ptr)           \
    do                            \
    {                             \
        if (ptr)                  \
        {                         \
            (void)dclrtFree(ptr); \
            ptr = nullptr;        \
        }                         \
    } while (0)

#define DCL_PROFILE_FREE(ptr)          \
    do                                 \
    {                                  \
        if (ptr)                       \
        {                              \
            dclprofDestroyConfig(ptr); \
            ptr = nullptr;             \
        }                              \
    } while (0)

#define SAFE_FREE(ptr)     \
    do                     \
    {                      \
        if (ptr)           \
        {                  \
            delete ptr;    \
            ptr = nullptr; \
        }                  \
    } while (0)
