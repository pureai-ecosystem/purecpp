#pragma once

#include <string>
#include <syncstream>
#include <fstream>
#include <memory>
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
#include <sstream>
#include <format>
#include <iostream> 
#include <filesystem>
#include "RagException.h"

namespace RAGLibrary
{
    static std::string FileReader(const std::string& filePath)
    {
        std::shared_ptr<std::ifstream> filePtr(new std::ifstream, [](std::ifstream* fil){
            fil->close();
        });

        try
        {
            filePtr->exceptions(std::ios::failbit);
            filePtr->open(filePath, std::ios::in);
            return {std::istreambuf_iterator<char>{*filePtr}, std::istreambuf_iterator<char>{}};
        }
        catch(const std::ios::failure& e)
        {
            // std::osyncstream(std::cerr) << e.what() << std::endl;
            std::cerr << e.what() << std::endl;
            throw RAGLibrary::RagException(e.what());
        }
    }

}