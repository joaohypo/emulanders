#pragma once

#include <thread>
#include <utility>

class ScopedThread {
public:
    ScopedThread() = default;

    template <typename F, typename... Args>
    explicit ScopedThread(F&& f, Args&&... args)
        : m_thread(std::forward<F>(f), std::forward<Args>(args)...) {}

    ~ScopedThread() {
        if (m_thread.joinable()) m_thread.join();
    }

    ScopedThread(const ScopedThread&) = delete;
    ScopedThread& operator=(const ScopedThread&) = delete;

    ScopedThread(ScopedThread&& other) noexcept
        : m_thread(std::move(other.m_thread)) {}

    ScopedThread& operator=(ScopedThread&& other) noexcept {
        if (this != &other) {
            if (m_thread.joinable()) m_thread.join();
            m_thread = std::move(other.m_thread);
        }
        return *this;
    }

    template <typename F, typename... Args>
    void launch(F&& f, Args&&... args) {
        if (m_thread.joinable()) m_thread.join();
        m_thread = std::thread(std::forward<F>(f), std::forward<Args>(args)...);
    }

    bool joinable() const { return m_thread.joinable(); }
    void join() { if (m_thread.joinable()) m_thread.join(); }

private:
    std::thread m_thread;
};
