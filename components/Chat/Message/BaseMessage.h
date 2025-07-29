#ifndef PURECPP_BASEMESSAGE_H
#define PURECPP_BASEMESSAGE_H

#include <string>
#include <memory>

namespace purecpp {
namespace chat {

class BaseMessage {
public:
    virtual ~BaseMessage() = default;
    virtual std::string get_type() const = 0;
    virtual std::string get_content() const = 0;
};

} // namespace chat
} // namespace purecpp

#endif // PURECPP_BASEMESSAGE_H
