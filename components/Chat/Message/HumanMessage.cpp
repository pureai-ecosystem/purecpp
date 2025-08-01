#include "HumanMessage.h"

namespace purecpp {
namespace chat {

HumanMessage::HumanMessage(std::string content) : content(std::move(content)) {}

std::string HumanMessage::get_type() const {
    return "human";
}

std::string HumanMessage::get_content() const {
    return content;
}

} // namespace chat
} // namespace purecpp
