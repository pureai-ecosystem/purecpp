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

inline AIMessage AIMessage::operator+(const AIMessage& a) const{
    return AIMessage(content + a.content);
}

} // namespace chat
} // namespace purecpp
