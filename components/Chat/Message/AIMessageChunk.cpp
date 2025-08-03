
#include "AIMessageChunk.h"

namespace purecpp {
namespace chat {

AIMessageChunk::AIMessageChunk(std::string content) : content(std::move(content)) {}

std::string AIMessageChunk::get_type() const {
    return "ai";
}

std::string AIMessageChunk::get_content() const {
    return content;
}

} // namespace chat
} // namespace purecpp
