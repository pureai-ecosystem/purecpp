#pragma once

#include <exception>
#include <string>

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