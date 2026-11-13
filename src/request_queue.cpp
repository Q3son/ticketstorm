#include "request_queue.hpp"

RequestQueue::RequestQueue(std::size_t capacity)
    : capacity_(capacity == 0 ? 1 : capacity) {}

RequestQueue::PushResult RequestQueue::push(const Request& req,
                                            const BlockHook& onBlock,
                                            const EnqueuedHook& onEnqueued) {
    std::unique_lock<std::mutex> lock(mtx_);

    bool blocked = false;
    if (queue_.size() >= capacity_ && !closed_) {
        blocked = true;
        if (onBlock) {
            onBlock();
        }
        // Espera con predicado: protege contra despertares espurios.
        notFull_.wait(lock, [this] {
            return queue_.size() < capacity_ || closed_;
        });
    }

    if (closed_) {
        return PushResult{false, blocked, queue_.size()};
    }

    queue_.push(req);
    const std::size_t sizeAfter = queue_.size();
    if (onEnqueued) {
        onEnqueued(sizeAfter);
    }

    lock.unlock();
    notEmpty_.notify_one();  // despierta a un Worker: "hay trabajo"
    return PushResult{true, blocked, sizeAfter};
}

RequestQueue::PopResult RequestQueue::pop(Request& out, const BlockHook& onBlock) {
    std::unique_lock<std::mutex> lock(mtx_);

    bool blocked = false;
    if (queue_.empty() && !closed_) {
        blocked = true;
        if (onBlock) {
            onBlock();
        }
        notEmpty_.wait(lock, [this] {
            return !queue_.empty() || closed_;
        });
    }

    if (queue_.empty()) {
        // closed_ == true y no queda trabajo pendiente: el Worker debe terminar.
        return PopResult{false, blocked};
    }

    out = queue_.front();
    queue_.pop();

    lock.unlock();
    notFull_.notify_one();  // despierta al Dispatcher: "hay espacio"
    return PopResult{true, blocked};
}

void RequestQueue::close() {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        closed_ = true;
    }
    notEmpty_.notify_all();
    notFull_.notify_all();
}
