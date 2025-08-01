#include "AIMessage.h"

namespace purecpp {
namespace chat {

AIMessage::AIMessage(std::string content) : content(std::move(content)) {}

std::string AIMessage::get_type() const {
    return "ai";
}

std::string AIMessage::get_content() const {
    return content;
}

} // namespace chat
} // namespace purecpp
