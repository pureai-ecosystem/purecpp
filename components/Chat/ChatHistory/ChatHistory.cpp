#include "ChatHistory.h"
#include <omp.h>
namespace purecpp {
namespace chat {

ChatHistory::ChatHistory(){   
    messages.reserve(INITIAL_CAPACITY);
}
void ChatHistory::add_message(const std::shared_ptr<BaseMessage> &message) {
    if (messages.size() == messages.capacity()) {
        messages.reserve(messages.capacity() * GROWTH_FACTOR);
    }
    messages.emplace_back(std::move(message));
}
void ChatHistory::add_message(const std::vector<std::shared_ptr<BaseMessage>> &_messages) {
    if (messages.size() == _messages.capacity()) {
        messages.reserve(_messages.capacity() * GROWTH_FACTOR);
    }
    #pragma omp parallel for
    for(size_t i = 0; i < _messages.size(); i++){
        messages.emplace_back(std::move(_messages[i]));
    }
}
const std::vector<std::shared_ptr<BaseMessage>>& ChatHistory::get_messages() const{
    return messages;
}

void ChatHistory::clear() {
    messages.clear();
}

} // namespace chat
} // namespace purecpp
