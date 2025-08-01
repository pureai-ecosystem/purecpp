#ifndef PURECPP_SYSTEMMESSAGE_H
#define PURECPP_SYSTEMMESSAGE_H

#include "BaseMessage.h"

namespace purecpp {
namespace chat {

class SystemMessage : public BaseMessage {
private:
    std::string content;
public:
    explicit SystemMessage(std::string  content);
    std::string get_type() const override;
    std::string get_content() const override;
};

} // namespace chat
} // namespace purecpp

#endif // PURECPP_SYSTEMMESSAGE_H
