#include "chunk_job_system.h"

#include "core/logger.h"

#include <algorithm>

namespace Sylva {

ChunkJobSystem::ChunkJobSystem(unsigned int threadCount) {
    if (threadCount == 0) {
        const unsigned int hw = std::thread::hardware_concurrency();
        threadCount = std::max(1u, hw > 1 ? hw - 1 : 1);
    }
    m_workers.reserve(threadCount);
    for (unsigned int i = 0; i < threadCount; ++i) {
        m_workers.emplace_back(&ChunkJobSystem::workerLoop, this);
    }
    Logger::logInfo("ChunkJobSystem started with " + std::to_string(threadCount) + " worker thread(s)");
}

ChunkJobSystem::~ChunkJobSystem() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_cv.notify_all();
    for (auto& t : m_workers) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void ChunkJobSystem::submit(std::function<void()> job) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_jobs.push(std::move(job));
        m_inFlight.fetch_add(1, std::memory_order_relaxed);
    }
    m_cv.notify_one();
}

size_t ChunkJobSystem::pending() const {
    return static_cast<size_t>(m_inFlight.load(std::memory_order_relaxed));
}

void ChunkJobSystem::workerLoop() {
    for (;;) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return m_stop || !m_jobs.empty(); });
            if (m_stop && m_jobs.empty()) {
                return;
            }
            job = std::move(m_jobs.front());
            m_jobs.pop();
        }
        try {
            job();
        } catch (const std::exception& e) {
            Logger::logError(std::string("Chunk worker job threw: ") + e.what());
        } catch (...) {
            Logger::logError("Chunk worker job threw unknown exception");
        }
        m_inFlight.fetch_sub(1, std::memory_order_relaxed);
    }
}

} // namespace Sylva
