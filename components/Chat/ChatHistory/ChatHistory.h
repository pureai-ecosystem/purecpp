#ifndef PURECPP_CHATHISTORY_H
#define PURECPP_CHATHISTORY_H

#include "../Message/BaseMessage.h"
#include <deque>
#include <vector>
#include <memory>
#include <shared_mutex>
namespace purecpp {
namespace chat {

class ChatHistory {
private:
    int INITIAL_CAPACITY = 1000;
    int GROWTH_FACTOR = 2;
    std::deque<std::shared_ptr<BaseMessage>> messages;
    mutable std::shared_mutex messages_mutex;

public:
    ChatHistory();
    void add_message(const std::shared_ptr<BaseMessage> &message);
    void add_message(const std::vector<std::shared_ptr<BaseMessage>> &messages);
    std::vector<std::shared_ptr<BaseMessage>> get_messages() const;
    void clear();

    // Benchmark-specific method for high-performance testing
    void add_benchmark_messages_omp(int num_messages);

    size_t size() const;
};

} // namespace chat
} // namespace purecpp

#endif // PURECPP_CHATHISTORY_H
