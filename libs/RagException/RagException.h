#pragma once
#include <exception>
#include <string>
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

#include <filesystem>
namespace RAGLibrary
{
    class RagException : public std::exception
    {
    public:
        RagException(const std::string& errorMsg) :
            m_errorMsg(errorMsg)
        {}

        const char * what() const noexcept final 
        {
            return m_errorMsg.c_str();
        }
    private:
        std::string m_errorMsg;
    };
}