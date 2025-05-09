#pragma once
#include <mutex>
#include <queue>
#include <optional>
#include <string>
#include <iostream>
#include <vector>
// #if __has_include(<semaphore>)
//     #include <semaphore>
// #else
//     // Alternativa: inclua uma implementação própria ou de terceiros
//     #include "semaphore_compat.h"
// #endif

// #if __has_include(<format>)
//     #include <format>
// #else
//     // Alternativa: use a biblioteca fmt ou implemente uma função de formatação simples
//     #include <fmt/core.h>
// #endif
#include <semaphore>
#include <format>

#include <fstream>
#include <sstream>
#include <iostream> 
//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
#include <filesystem>
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
            for (auto elem : vect)
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
            if (m_queue.empty())
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
            while (!m_queue.empty())
            {
                m_queue.pop();
            }
        }

    private:
        mutable std::mutex m_mutex;
        mutable std::queue<Type> m_queue;
    };
}