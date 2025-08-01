#ifndef PURECPP_CHATHISTORY_H
#define PURECPP_CHATHISTORY_H

#include "../Message/BaseMessage.h"
#include <vector>
#include <memory>

namespace purecpp {
namespace chat {

class ChatHistory {
private:
    int INITIAL_CAPACITY = 1000;
    int GROWTH_FACTOR = 2;
    std::vector<std::shared_ptr<BaseMessage>> messages;

public:
    ChatHistory();
    void add_message(const std::shared_ptr<BaseMessage> &message);
    void add_message(const std::vector<std::shared_ptr<BaseMessage>> &message);
    const std::vector<std::shared_ptr<BaseMessage>>& get_messages() const;
    void clear();
};

} // namespace chat
} // namespace purecpp

#endif // PURECPP_CHATHISTORY_H
