#pragma once

#include <any>
#include <string>
#include <vector>

namespace StringUtils 
{

    std::string escapeRegex(const std::string& str);

    void joinStr(const std::string& str, const std::vector<std::string> & inputs, std::string & output);


    std::string ellipsis(const std::string& text, const std::size_t maxLength = 100);

    std::string any2str(const std::any& var);

    std::string str_details(const std::string& text);

    std::string removeAccents(const std::string &input);
}