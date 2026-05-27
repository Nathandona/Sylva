#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace Sylva {

/**
 * @brief Fixed-size worker pool that runs chunk generation jobs off the main
 *        thread so terrain + feature placement doesn't block rendering.
 *
 * Jobs are arbitrary std::function<void()>; the caller decides what each job
 * does (typically: generate terrain + features for one chunk and flip its
 * ChunkState to Ready when done).
 *
 * GL calls are forbidden inside a job — only the main thread holds the GL
 * context. Use the pool for CPU-side work (noise sampling, block writes)
 * and let the main thread pick up Ready chunks for meshing/upload.
 */
class ChunkJobSystem {
public:
    /**
     * @brief Construct with N worker threads. N defaults to hardware concurrency - 1
     *        (min 1) so the main thread keeps a core free for rendering.
     */
    explicit ChunkJobSystem(unsigned int threadCount = 0);

    /**
     * @brief Joins all workers. Pending jobs are drained before exit.
     */
    ~ChunkJobSystem();

    ChunkJobSystem(const ChunkJobSystem&) = delete;
    ChunkJobSystem& operator=(const ChunkJobSystem&) = delete;
    ChunkJobSystem(ChunkJobSystem&&) = delete;
    ChunkJobSystem& operator=(ChunkJobSystem&&) = delete;

    /**
     * @brief Enqueue a job. Returns immediately; job runs on a worker thread.
     */
    void submit(std::function<void()> job);

    /**
     * @brief Number of jobs queued or in-flight (for diagnostics).
     */
    size_t pending() const;

private:
    void workerLoop();

    std::vector<std::thread> m_workers;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<std::function<void()>> m_jobs;
    std::atomic<int> m_inFlight{0};
    bool m_stop{false};
};

} // namespace Sylva
