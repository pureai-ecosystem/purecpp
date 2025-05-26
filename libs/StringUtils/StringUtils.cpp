#include "StringUtils.h"

#include <re2/re2.h>

#include <numeric>
#include <regex>
#include <unordered_map>

using namespace StringUtils;

std::string StringUtils::escapeRegex(const std::string& str)
{
    static const std::regex specialChars(R"([-[\]{}()*+?.,\^$|#\s])");
    return std::regex_replace(str, specialChars, R"(\$&)");
}

void StringUtils::joinStr(const std::string& str, const std::vector<std::string> & inputs, std::string & output)
{
    if (!inputs.empty())
    {
        int size = std::accumulate(inputs.begin(), inputs.end(), std::size_t(0), [](std::size_t sum, const std::string & str ){
            return sum + str.size();
        });
        output.reserve(output.size() + size + (inputs.size() -1)*str.size());
        output.append(inputs.front());
        for (size_t i = 1; i < inputs.size(); i++)
        {
            output.append(str);
            output.append(inputs[i]);
        }
    }
}

std::string StringUtils::ellipsis(const std::string& text, const std::size_t maxLength)
{
    if(text.length() > maxLength)
    {
        return text.substr(0, maxLength -3) + "...";
    }
    return text;
}

std::string StringUtils::any2str(const std::any& var)
{
    std::stringstream ss;
    auto isStr = var.type() == typeid(std::string);
    // auto type = isStr ? "std::string" : var.type().name();
    // ss << "(" << type << ")";
    if (!var.has_value()) ss << "None";
    else if (var.type() == typeid(int)) ss << std::any_cast<int>(var);
    else if (var.type() == typeid(double)) ss << std::any_cast<double>(var);
    else if (var.type() == typeid(float)) ss << std::any_cast<float>(var);
    else if (isStr) ss << std::any_cast<std::string>(var);

    return ss.str();
}

std::string StringUtils::str_details(const std::string& text)
{
    std::stringstream ss;
    ss << "{\"value\":\""  << ellipsis(text) << "\",\"size\":" << text.size() <<"}";
    return ss.str();
}

std::string StringUtils::removeAccents(const std::string &input)
{
    std::string result = input;

    static std::vector<std::pair<std::string, std::string>> replacements = {
        {"á", "a"}, {"é", "e"}, {"í", "i"}, {"ó", "o"}, {"ú", "u"},
        {"â", "a"}, {"ê", "e"}, {"î", "i"}, {"ô", "o"}, {"û", "u"},
        {"à", "a"}, {"è", "e"}, {"ì", "i"}, {"ò", "o"}, {"ù", "u"},
        {"ä", "a"}, {"ë", "e"}, {"ï", "i"}, {"ö", "o"}, {"ü", "u"},
        {"ã", "a"}, {"õ", "o"}, {"ç", "c"}, {"Á", "A"}, {"É", "E"},
        {"Í", "I"}, {"Ó", "O"}, {"Ú", "U"}, {"Â", "A"}, {"Ê", "E"},
        {"Î", "I"}, {"Ô", "O"}, {"Û", "U"}, {"À", "A"}, {"È", "E"},
        {"Ì", "I"}, {"Ò", "O"}, {"Ù", "U"}, {"Ä", "A"}, {"Ë", "E"},
        {"Ï", "I"}, {"Ö", "O"}, {"Ü", "U"}, {"Ã", "A"}, {"Õ", "O"},
        {"Ç", "C"}
    };

    for (const auto &pair : replacements)
    {
        re2::RE2::GlobalReplace(&result, pair.first, pair.second);
    }

    return result;
}