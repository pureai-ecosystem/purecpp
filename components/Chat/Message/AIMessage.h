#ifndef PURECPP_AIMESSAGE_H
#define PURECPP_AIMESSAGE_H

#include "BaseMessage.h"

namespace purecpp {
namespace chat {

class AIMessage : public BaseMessage {
private:
    std::string content;
public:
    explicit AIMessage(std::string  content);
    std::string get_type() const override;
    std::string get_content() const override;
};

} // namespace chat
} // namespace purecpp

#endif // PURECPP_AIMESSAGE_H
