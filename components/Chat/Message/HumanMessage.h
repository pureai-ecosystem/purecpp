#ifndef PURECPP_HUMANMESSAGE_H
#define PURECPP_HUMANMESSAGE_H

#include "BaseMessage.h"

namespace purecpp {
namespace chat {

class HumanMessage : public BaseMessage {
private:
    std::string content;
public:
    explicit HumanMessage(std::string  content);
    std::string get_type() const override;
    std::string get_content() const override;
};

} // namespace chat
} // namespace purecpp

#endif // PURECPP_HUMANMESSAGE_H
