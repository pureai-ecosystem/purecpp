#ifndef PURECPP_BASEMESSAGE_CHUNK_H
#define PURECPP_BASEMESSAGE_CHUNK_H

#include <string>
#include <memory>

namespace purecpp {
namespace chat {

class BaseMessageChunk {
public:
    virtual ~BaseMessageChunk() = default;
    virtual std::string get_type() const = 0;
    virtual std::string get_content() const = 0;
};

} // namespace chat
} // namespace purecpp

#endif // PURECPP_BASEMESSAGE_CHUNK_H
