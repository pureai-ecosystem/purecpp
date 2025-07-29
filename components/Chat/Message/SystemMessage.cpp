#include "SystemMessage.h"

namespace purecpp {
namespace chat {

SystemMessage::SystemMessage(std::string content) : content(std::move(content)) {}

std::string SystemMessage::get_type() const {
    return "system";
}

std::string SystemMessage::get_content() const {
    return content;
}

} // namespace chat
} // namespace purecpp
