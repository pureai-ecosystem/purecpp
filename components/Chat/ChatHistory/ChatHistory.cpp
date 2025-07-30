#include "ChatHistory.h"

namespace purecpp {
namespace chat {

ChatHistory::ChatHistory(){   
    messages.reserve(INITIAL_CAPACITY);
}
void ChatHistory::add_message(std::shared_ptr<BaseMessage> message) {
    if (messages.size() == messages.capacity()) {
        messages.reserve(messages.capacity() * GROWTH_FACTOR);
    }
    messages.emplace_back(std::move(message));
}
const std::vector<std::shared_ptr<BaseMessage>>& ChatHistory::get_messages() const{
    return messages;
}

void ChatHistory::clear() {
    messages.clear();
}

} // namespace chat
} // namespace purecpp
