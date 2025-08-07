#pragma once
#include <string>

namespace purecpp::websearch {

class ICleaner {
public:
    virtual ~ICleaner() = default;

    /**
     * @param html  
     * @param mime  
     * @return      
     * @throw       
     */
    virtual std::string to_markdown(const std::string& html,
                                    const std::string& mime) = 0;
};

} // namespace purecpp::websearch
