/*
 *  File Name: RingBuffer.h
 *  Created on: 2022年7月4日
 *  Author: zjc
 *  description: 循环buffer,带阻塞，销毁需调用exit
 */
#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include <cstring>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unistd.h>

class CRingBuffer {
public:
    /* 20M */
    static const int MAX_BUFFER_LEN = (20 * 1024 * 1024);
    static const int NO_WAIT_MODE = 0;
    static const int READ_WAIT_MODE = 1;
    static const int WRITE_WAIT_MODE = 1 << 2;
    static const int DEFAULT_MODE =  READ_WAIT_MODE | WRITE_WAIT_MODE;

    CRingBuffer() = delete;
    CRingBuffer(uint32_t nSize, int nMode = DEFAULT_MODE)
        : m_buf(nullptr),
          m_nSize(0),
          m_write(0),
          m_read(0),
          m_nMode(nMode),
          m_bExit(false)
    {
        m_nSize = create(nSize);
        clear();
    }

    ~CRingBuffer() {
       exit();
    }

    void clear() {
        m_bExit.store(true);
        m_cond.notify_all();
        usleep(1000);
        m_read = m_write = 0;
        m_bExit.store(false);
    }

    uint64_t read(uint8_t *pData, uint64_t nSize) {
        if (pData == nullptr) {
            return 0;
        }
        std::unique_lock<std::mutex> mtx(m_mtx);
        uint64_t nLen = 0;
        if (!readWait(mtx, nSize)) {
            return 0;
        }

        /* first get the data from fifo->out until the end of the buffer */
        nLen = min(nSize, m_nSize - (m_read & (m_nSize - 1)));
        memcpy(pData, m_buf + (m_read & (m_nSize - 1)), nLen);

        /* then get the rest (if any) from the beginning of the buffer */
        memcpy(pData + nLen, m_buf, nSize - nLen);

        m_read += nSize;


        m_cond.notify_all();

        return nSize;
    }
    uint64_t write(uint8_t *pData, uint64_t nSize) {
        if (pData == nullptr || nSize == 0) {
            return 0;
        }
        std::unique_lock<std::mutex> mtx(m_mtx);
        uint64_t nLen = 0;

        if (!writeWait(mtx, nSize)) {
            return 0;
        }

        /* first put the data starting from fifo->in to buffer end */
        nLen = min(nSize, m_nSize - (m_write & (m_nSize - 1)));
        memcpy(m_buf + (m_write & (m_nSize - 1)), pData, nLen);

        /* then put the rest (if any) at the beginning of the buffer */
        memcpy(m_buf, pData + nLen, nSize - nLen);

        m_write += nSize;

        m_cond.notify_all();
        return nSize;
    }
    uint64_t thrown(uint64_t nSize) {
        nSize = min(nSize, m_write - m_read);
        m_write += nSize;
        return nSize;
    }

    void exit() {
        m_bExit.store(true);
        m_cond.notify_all();
        if (m_buf) {
            delete m_buf;
            m_buf = nullptr;
        }
    }

    uint64_t length() {
        std::unique_lock<std::mutex> mtx(m_mtx);
        return used();
    }

private:
    int create(uint32_t nSize) {
       if (nSize == 0 || nSize > MAX_BUFFER_LEN) {
           nSize = MAX_BUFFER_LEN;
       }

       /* 是否是2的次方 */
       uint32_t nReSize = nSize;
       if ((nSize & (nSize - 1)) != 0) {
           nReSize = 1;
           while (nSize > nReSize) {
               nReSize *= 2;
           }
       }
       m_buf = new (std::nothrow) uint8_t[nReSize];
       if (!m_buf) {
           ::exit(1);
       }
       return nReSize;
    }
    bool readWait(std::unique_lock<std::mutex> &mtx, uint64_t nSize) {
        while (nSize > used() && !m_bExit.load()) {
            if (m_nMode & READ_WAIT_MODE) {
                m_cond.wait(mtx);
            } else {
                return 0;
            }
        }
        return !m_bExit.load();
    }

    bool writeWait(std::unique_lock<std::mutex> &mtx, uint64_t nSize) {
        while (nSize > free() && !m_bExit.load()) {
            if (m_nMode & WRITE_WAIT_MODE) {
                m_cond.wait(mtx);
            } else {
                return 0;
            }
        }
        return !m_bExit.load();
    }

    uint64_t free() {
        return total() - used();
    }
    uint64_t used() {
        return m_write - m_read;
    }
    uint64_t total() {
        return m_nSize;
    }

    inline uint64_t min(uint64_t a, uint64_t b) {
        return (a < b ? a : b);
    }

private:
    uint8_t *m_buf;
    uint32_t m_nSize;
    uint64_t m_write;
    uint64_t m_read;
	std::condition_variable m_cond;
    std::mutex m_mtx;
    int m_nMode;
    std::atomic<bool> m_bExit;
};

#endif

