#include "ChatHistory.h"
#include "../Message/AIMessage.h"
#include "../Message/HumanMessage.h"
#include <mutex>
#include <omp.h>
#include <vector>
#include <shared_mutex>

namespace purecpp {
namespace chat {

ChatHistory::ChatHistory() = default;

void ChatHistory::add_message(const std::shared_ptr<BaseMessage> &message) {
    std::unique_lock<std::shared_mutex> lock(messages_mutex);
    messages.push_back(message);
}

void ChatHistory::add_message(const std::vector<std::shared_ptr<BaseMessage>> &new_messages) {
    std::unique_lock<std::shared_mutex> lock(messages_mutex);
    messages.insert(messages.end(), new_messages.begin(), new_messages.end());
}


std::vector<std::shared_ptr<BaseMessage>> ChatHistory::get_messages() const {
    std::shared_lock<std::shared_mutex> lock(messages_mutex);
    if (messages.empty()) {
        return {};
    }
    std::vector<std::shared_ptr<BaseMessage>> result(messages.size());
    #pragma omp parallel for
    for (size_t i = 0; i < messages.size(); ++i) {
        result[i] = messages[i];
    }
    return result;
}

void ChatHistory::clear() {
    std::unique_lock<std::shared_mutex> lock(messages_mutex);
    messages.clear();
}

size_t ChatHistory::size() const {
    std::shared_lock<std::shared_mutex> lock(messages_mutex);
    return messages.size();
}

void ChatHistory::add_benchmark_messages_omp(int num_messages) {
    std::vector<std::vector<std::shared_ptr<BaseMessage>>> thread_local_messages;

#pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
#pragma omp single
        {
            thread_local_messages.resize(omp_get_num_threads());
        }

#pragma omp for
        for (int i = 0; i < num_messages; ++i) {
            if (i % 2 == 0) {
                thread_local_messages[thread_id].push_back(std::make_shared<HumanMessage>("This is a test message from user " + std::to_string(i)));
            } else {
                thread_local_messages[thread_id].push_back(std::make_shared<AIMessage>("This is a response from AI " + std::to_string(i)));
            }
        }
    }

    std::unique_lock<std::shared_mutex> lock(messages_mutex);
    for (const auto& local_vec : thread_local_messages) {
        messages.insert(messages.end(), local_vec.begin(), local_vec.end());
    }
}

} // namespace chat
} // namespace purecpp
