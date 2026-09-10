/*** 
 * @FilePath     : safe_queue.h
 * @Author       : zjc
 * @Date         : 2022-06-28 20:23:23
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-08-25 20:23:25
 * @Description  : 安全队列,带阻塞，销毁需调用exit
 */

#pragma once

#ifndef SAFE_QUEUE_H_
#define SAFE_QUEUE_H_

#include <stdint.h>
#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <ctime>
#include <chrono>
#include <atomic>

template <class T>
class SafeQueue  {
public:
    static const int32_t TIMEOUT_FOREVER = -1;
    static const int32_t TIMEOUT_NONE = 0;
	static const int32_t DAFULT_TIMEOUT_MS = 3000;
	SafeQueue(uint32_t nSize = 20) :m_nMaxSize(nSize),m_bExit(false) {}
	SafeQueue(SafeQueue& other) :m_nMaxSize(other.m_nMaxSize),m_bExit(false) {}
	SafeQueue(SafeQueue&& other) :m_nMaxSize(other.m_nMaxSize),m_bExit(false) {}
	~SafeQueue() {
	    exit();
    }
    // 移动赋值运算符
    SafeQueue& operator=(SafeQueue&& other) noexcept {
        if (this != &other) {
	        m_nMaxSize = std::move(other.m_nMaxSize);
        }
        return *this;
    }
	int pop(T& out, int32_t nTimeout = DAFULT_TIMEOUT_MS) {
		std::unique_lock<std::mutex> mtx(m_mutex);
        if (m_bExit.load()) {
            return -1;
        }
		if (empty()) {
            if (nTimeout == TIMEOUT_FOREVER) {
                m_cond.wait(mtx);
            } else {
                m_cond.wait_for(mtx, std::chrono::milliseconds(nTimeout));
            }

			if (empty() || m_bExit.load()) {
				return -1;
			}
		}

		out = m_que.front();
		m_que.pop();
		mtx.unlock();
		m_cond.notify_all();

		return 0;
	}


	int pop_front(T& out, int32_t nTimeout = DAFULT_TIMEOUT_MS) {
		std::unique_lock<std::mutex> mtx(m_mutex);
        if (m_bExit.load()) {
            return -1;
        }
		if (empty()) {
            if (nTimeout == TIMEOUT_FOREVER) {
                m_cond.wait(mtx);
            } else {
                m_cond.wait_for(mtx, std::chrono::milliseconds(nTimeout));
            }

			if (empty() || m_bExit.load()) {
				return -1;
			}
		}

		out = m_que.front();
		mtx.unlock();
		m_cond.notify_all();
		return 0;
	}
	int push(T const& src, int32_t nTimeout = DAFULT_TIMEOUT_MS) {
		std::unique_lock<std::mutex> mtx(m_mutex);
        if (m_bExit.load()) {
            return -1;
        }

		if (full()) {
            if (nTimeout == TIMEOUT_FOREVER) {
                m_cond.wait(mtx);
            } else {
			    m_cond.wait_for(mtx, std::chrono::milliseconds(nTimeout));
            }
			if (full() || m_bExit.load()) {
				return -1;
			}
		}
		m_que.push(src);
		mtx.unlock();
		m_cond.notify_all();
		return 0;
	}

	uint32_t size() {
		return m_que.size();
	}
	bool empty() {
		return m_que.empty();
	}
	bool full() {
		return size() >= m_nMaxSize;
	}
    void exit() {
        m_bExit.store(true);
        m_cond.notify_all();
    }
private:
	std::queue<T> m_que;
	uint32_t m_nMaxSize;
	std::mutex m_mutex;
	std::condition_variable m_cond;

    std::atomic<bool> m_bExit;
};

#endif

