#pragma once
#include <functional>
#include <string>
#include <chrono>

namespace purecpp::websearch {

/* Etapas de um job de WebSearch. */
enum class JobStage {
    Started,
    UrlFetched,
    UrlCleaned,
    Done,
    Error
};

/* Evento que pode ser emitido durante o processamento. */
struct ProgressEvent {
    JobStage                                           stage;
    std::string                                        detail;   // URL ou mensagem
    std::chrono::system_clock::time_point              timestamp;
};

using ProgressCallback = std::function<void(const ProgressEvent&)>;

/**
 * Interface opcional para publicar eventos em outros meios (ex.: WebSocket).
 */
class IEmitter {
public:
    virtual ~IEmitter() = default;
    virtual void emit(const ProgressEvent& ev) = 0;
};

} // namespace purecpp::websearch
