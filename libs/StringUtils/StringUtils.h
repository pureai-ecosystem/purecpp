#pragma once
#include <any>
#include <string>
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
#include <filesystem>

#include <iostream> 
namespace StringUtils 
{
    std::string escapeRegex(const std::string& str);

    void joinStr(const std::string& str, const std::vector<std::string> & inputs, std::string & output);

    std::string ellipsis(const std::string& text, const std::size_t maxLength = 100);

    std::string any2str(const std::any& var);

    std::string str_details(const std::string& text);

    std::string removeAccents(const std::string &input);
}