#ifndef PURECPP_TOOL_H
#define PURECPP_TOOL_H

#include "BaseMessage.h"
#include "BaseMessageChunk.h"
#include <any>

namespace purecpp{
namespace chat{
class ToolMessage : BaseMessage{
private:
    std::string tool_call_id;
    std::any artifact;
    std::string status;
    std::string type;

public:

};
}
}

#endif
