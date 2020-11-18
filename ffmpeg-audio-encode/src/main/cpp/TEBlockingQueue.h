//
// Created by 郭鹤 on 2020/4/25.
//

#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <deque>
#include "XWXResult.h"
#include "condition_variable"
#include "list"

#define MAX_CAPACITY 10

#define INTERNAL_WAIT_TIME_US 100000   // 10ms

    template <typename T> class TEBlockingQueue {

    public:
        using QueueIter = typename std::deque<T>::iterator;
        //-----------------------------Construct & Destruct----------------------------------
        explicit TEBlockingQueue(const size_t uMaxSize = MAX_CAPACITY) : m_uCapacity(uMaxSize) {}

        virtual ~TEBlockingQueue() {};

        inline void setCapacity(size_t uCapacity) {
            std::lock_guard<std::mutex> guard(m_oOperate);
            m_uCapacity = uCapacity;
        }

        void setActive(bool active) {
            std::unique_lock<std::mutex> uniqueLock(m_oOperate);
            m_bIsActive = active;
            uniqueLock.unlock();
            m_cvProducer.notify_all();
            m_cvConsumer.notify_all();
        }

        //-----------------------------Blocking Functions----------------------------------
        /**
         * @brief Inserts the specified element into this queue, waiting if necessary for space to become available.
         * @param t
         *
         * @return TER_OK(0) : Success
         *         TER_FAIL(-1) : Failed
         */
        int put(T &t) {
            std::unique_lock<std::mutex> lock(m_oOperate);
            m_cvProducer.wait(lock, [this] {
                return !_isFull() || !m_bIsActive;
            });
            if (m_bIsActive) {
                m_queue.push_back(t);
                m_cvConsumer.notify_one();
            }
            return 0;
        }

        /**
         * @brief Inserts the specified element into this queue, waiting up to the specified wait time if necessary for space to become available.
         * @param t
         * @param iTimeoutUS iTimeoutUS timeout time int microsecond
         *
         * @return TER_OK(0) : Success
         *         TER_FAIL(-1) : Failed
         *         TER_TIMEOUT(-107) : Timeout
         */
        int put(T &t, long iTimeoutUS) {
            std::unique_lock<std::mutex> lock(m_oOperate);
            bool ret = m_cvProducer.wait_for(lock, std::chrono::microseconds(iTimeoutUS), [this] {
                return !_isFull() || !m_bIsActive;
            });
            if (!ret) {
                return -1;
            }
            if (m_bIsActive) {
                m_queue.push_back(t);
                m_cvConsumer.notify_one();
            }
            return 0;
        }

        /**
         * @brief Retrieves and removes the head of this queue, waiting if necessary until an element becomes available.
         *
         * @return the head of this queue.
         */
        int take(T &t) {
            std::unique_lock<std::mutex> lock(m_oOperate);
            m_cvConsumer.wait(lock, [this] {
                return !_isEmpty() || !m_bIsActive;
            });
            if (m_bIsActive) {
                t = m_queue.front();
                m_queue.pop_front();
                m_cvProducer.notify_one();
            }
            return 0;
        }

        /**
         * @brief Retrieves and removes the head of this queue, waiting up to the specified wait time if necessary for space to become available.
         * @param iTimeoutUS timeout time int microsecond
         *
         * @return the head of this queue.
         */
        int take(T &t, long iTimeoutUS) {
            std::unique_lock<std::mutex> lock(m_oOperate);
            bool ret = m_cvConsumer.wait_for(lock, std::chrono::microseconds(iTimeoutUS), [this] {
                return !_isEmpty() || !m_bIsActive;
            });
            if (!ret) {
                return -1;
            }

            if (m_bIsActive) {
                t = m_queue.front();
                m_queue.pop_front();
                m_cvProducer.notify_one();
            }
            return 0;
        }

        /**
         * @brief Push all of the elements to the consumer.
         * @return The number of elements flushed.
         */
        // size_t flush();

        //-----------------------------Unblocking Functions----------------------------------
        void forceWakeUpConsumer() { m_cvConsumer.notify_all(); }

        void forceWakeUpProducer() { m_cvProducer.notify_all(); }

        /**
         * @brief Inserts the specified element into the queue without blocking the queue.
         *
         * @return  TER_OK: upon success
         *          TER_FAIL: if no space is currently available
         */
        int tryPut(T &t) {
            std::unique_lock<std::mutex> lock(m_oOperate);
            if (_isFull()) {
                return -1;
            }

            m_queue.push_back(t);
            m_cvConsumer.notify_one();
            return 0;
        }

        /**
         * @brief Retrieves and removes the head of this queue without blocking the queue.
         * @param t return value
         * @return TER_OK: upon success
         *          TER_FAIL: if queue is empty.
         */
        int tryTake(T &t) {
            std::unique_lock<std::mutex> lock(m_oOperate);
            if (_isEmpty()) {
                return -1;
            }

            t = m_queue.front();
            m_queue.pop_front();
            m_cvProducer.notify_one();
            return 0;
        }

        /**
         * @brief Retrieves, but does not remove, the head of this queue.
         * Attention: Calling this function on empty queue causes undefined behavior.
         *
         * @return the head of this queue.
         */
        inline int front(T &t) {
            std::lock_guard<std::mutex> guard(m_oOperate);
            if (_isEmpty()) {
                return -1;
            }
            t = m_queue.front();
            return 0;
        }

        /**
         * @brief Retrieves, but does not remove, the tail of this queue.
         * Attention: Calling this function on empty queue causes undefined behavior.
         *
         * @param t return the tail of this queue.
         * @return Reference Status
         */
        inline int back(T &t) {
            std::lock_guard<std::mutex> guard(m_oOperate);
            if (_isEmpty()) {
                return -1;
            }

            t = m_queue.back();
            return 0;
        }

        /**
         * @brief Remove all of the elements from the queue.
         */
        inline void clear() {
            std::lock_guard<std::mutex> guard(m_oOperate);
            m_queue.clear();
        }

        /**
         * @brief Get current size of the elements.
         * @return
         */
        inline size_t size() {
            std::lock_guard<std::mutex> guard(m_oOperate);
            return m_queue.size();
        }

    protected:
        inline bool _isFull() { return m_queue.size() >= m_uCapacity; }

        inline bool _isEmpty() { return m_queue.empty(); }

        inline size_t _size() { return m_queue.size(); }

    private:
        std::mutex m_oOperate;
        std::condition_variable m_cvProducer;
        std::condition_variable m_cvConsumer;

        std::list<T> m_queue;
        size_t m_uCapacity;
        bool m_bIsActive = true;
    };

