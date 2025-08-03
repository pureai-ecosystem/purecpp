#ifndef PURECPP_AIMESSAGE_CHUNK_H
#define PURECPP_AIMESSAGE_CHUNK_H

#include "BaseMessage.h"
#include "AIMessage.h"

namespace purecpp {
namespace chat {

class AIMessageChunk : public BaseMessage {
private:
    std::string content;
public:
    explicit AIMessageChunk(std::string  content);
    std::string get_type() const override;
    std::string get_content() const override;
    AIMessageChunk __add__() const;
};

} // namespace chat
} // namespace purecpp

#endif // PURECPP_AIMESSAGE_H
