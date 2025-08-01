#include "ChatHistory.h"
#include <omp.h>
#include <shared_mutex>

namespace purecpp {
namespace chat {

ChatHistory::ChatHistory(){   
    messages.reserve(INITIAL_CAPACITY);
}
void ChatHistory::add_message(const std::shared_ptr<BaseMessage> &message) {
    std::unique_lock<std::shared_mutex> lock(messages_mutex);
    if (messages.size() == messages.capacity()) {
        messages.reserve(messages.capacity() * GROWTH_FACTOR);
    }
    messages.emplace_back(std::move(message));
}
void ChatHistory::add_message(const std::vector<std::shared_ptr<BaseMessage>> &_messages) {
    messages.reserve(messages.size() + _messages.size());
    for(const auto &msg : _messages) messages.emplace_back(std::move(msg));
}
const std::vector<std::shared_ptr<BaseMessage>>& ChatHistory::get_messages() const{

}

void ChatHistory::clear() {
    messages.clear();
}

size_t ChatHistory::size() const {
    std::lock_guard<std::mutex> lock(messages_mutex);
    return messages.size();
}

} // namespace chat
} // namespace purecpp
