#pragma once

#include <mutex>
#include <queue>
#include <optional>
#include <string>
#include <iostream>
#include <vector>


namespace RAGLibrary
{
    template <typename Type>
    class ThreadSafeQueue
    {
    public:
        ThreadSafeQueue() = default;

        ThreadSafeQueue(const ThreadSafeQueue& other)
        {
            this->m_queue = other.m_queue;
        }

        ThreadSafeQueue(const std::vector<Type>& vect)
        {
            for(auto elem : vect)
            {
                this->m_queue.push(elem);
            }
        }

        ThreadSafeQueue& operator=(const ThreadSafeQueue& other)
        {
            this->m_queue = other.m_queue;
            return *this;
        }

        void push(const Type& value)
        {
            std::lock_guard lock(m_mutex);
            m_queue.push(value);
        }

        std::optional<Type> pop()
        {
            std::lock_guard lock(m_mutex);
            if(m_queue.empty())
            {
                return std::nullopt;
            }

            auto value = m_queue.front();
            m_queue.pop();
            return value;
        }

        std::size_t size()
        {
            std::lock_guard lock(m_mutex);
            return m_queue.size();
        }

        void clear()
        {
            std::lock_guard lock(m_mutex);
            while(!m_queue.empty())
            {
                m_queue.pop();
            }
        }

    private:
        mutable std::mutex m_mutex;
        mutable std::queue<Type> m_queue;
    };
}