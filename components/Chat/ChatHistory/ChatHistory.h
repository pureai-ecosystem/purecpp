#ifndef PURECPP_CHATHISTORY_H
#define PURECPP_CHATHISTORY_H

#include "../Message/BaseMessage.h"
#include <vector>
#include <memory>

namespace purecpp {
namespace chat {

class ChatHistory {
private:
    std::vector<std::shared_ptr<BaseMessage>> messages;

public:
    void add_message(std::shared_ptr<BaseMessage> message);
    const std::vector<std::shared_ptr<BaseMessage>>& get_messages() const;
    void clear();
};

} // namespace chat
} // namespace purecpp

#endif // PURECPP_CHATHISTORY_H
