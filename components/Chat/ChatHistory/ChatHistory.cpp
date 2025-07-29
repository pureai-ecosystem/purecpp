#include "ChatHistory.h"

namespace purecpp {
namespace chat {

void ChatHistory::add_message(std::shared_ptr<BaseMessage> message) {
    messages.push_back(std::move(message));
}

const std::vector<std::shared_ptr<BaseMessage>>& ChatHistory::get_messages() const {
    return messages;
}

void ChatHistory::clear() {
    messages.clear();
}

} // namespace chat
} // namespace purecpp
