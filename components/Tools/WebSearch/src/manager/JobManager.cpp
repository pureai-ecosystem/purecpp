#include "src/manager/JobManager.h"
#include <chrono>
#include <random>
#include <sstream>

namespace purecpp::websearch {

/* ------------- util: gera id pseudo-único ---------------------------- */

static std::string make_uuid()
{
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream oss;
    oss << std::hex << dist(rng);
    return oss.str();
}

/* ------------- ctor -------------------------------------------------- */

JobManager::JobManager(std::shared_ptr<IWebSearch> search,
                       std::shared_ptr<ICrawler>   crawler,
                       std::shared_ptr<ICleaner>   cleaner,
                       std::size_t                 n_workers)
    : pool_(n_workers)
    , search_(std::move(search))
    , crawler_(std::move(crawler))
    , cleaner_(std::move(cleaner))
{}

/* ------------- submit ------------------------------------------------ */

std::future<SearchJob> JobManager::submit(const std::string&   query,
                                          int                  k,
                                          ProgressCallback     cb,
                                          const std::string&   lang)
{
    return pool_.enqueue([=] {
        const auto now = [] { return std::chrono::system_clock::now(); };

        auto emit = [&](JobStage st, std::string detail = {}) {
            if (cb) cb(ProgressEvent{st, std::move(detail), now()});
        };

        emit(JobStage::Started);

        /* 1. busca URLs */
        std::vector<UrlResult> urls = search_->query(query, k, lang);

        SearchJob job;
        job.id     = make_uuid();
        job.query  = query;
        job.k      = k;
        job.urls   = urls;
        job.md_pages.resize(urls.size());

        /* 2. processa cada URL (paralelo no mesmo pool) */
        std::vector<std::future<void>> futs;
        for (std::size_t i = 0; i < urls.size(); ++i) {
            futs.push_back(pool_.enqueue([&, i] {
                try {
                    std::string mime;
                    auto html = crawler_->fetch(urls[i].url, mime);
                    emit(JobStage::UrlFetched, urls[i].url);

                    auto md  = cleaner_->to_markdown(html, mime);
                    job.md_pages[i] = std::move(md);

                    emit(JobStage::UrlCleaned, urls[i].url);
                } catch (const std::exception& e) {
                    emit(JobStage::Error, e.what());
                }
            }));
        }
        /* espera */
        for (auto& f : futs) f.get();

        emit(JobStage::Done);
        return job;
    });
}

} // namespace purecpp::websearch
