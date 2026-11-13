#include "request_queue.hpp"

#include "tracer.hpp"

namespace {

// Adquiere el mutex de la cola registrando si hubo contención real: primero
// se intenta sin bloquear y, si otro hilo lo tiene, se registra la espera.
std::unique_lock<std::mutex> acquire(std::mutex& m) {
    std::unique_lock<std::mutex> lock(m, std::try_to_lock);
    if (!lock.owns_lock()) {
        trace::emit("lock_wait", "queue");
        lock.lock();
    }
    trace::emit("lock", "queue");
    return lock;
}

}  // namespace

RequestQueue::RequestQueue(std::size_t capacity)
    : capacity_(capacity == 0 ? 1 : capacity) {}

RequestQueue::PushResult RequestQueue::push(const Request& req,
                                            const BlockHook& onBlock,
                                            const EnqueuedHook& onEnqueued) {
    std::unique_lock<std::mutex> lock = acquire(mtx_);

    bool blocked = false;
    if (queue_.size() >= capacity_ && !closed_) {
        blocked = true;
        if (onBlock) {
            onBlock();
        }
        // wait() libera el mutex de forma atómica y bloquea al hilo.
        trace::emit("cv_wait", "notFull", {{"size", static_cast<long long>(queue_.size())}});
        // Espera con predicado: protege contra despertares espurios.
        notFull_.wait(lock, [this] {
            return queue_.size() < capacity_ || closed_;
        });
        trace::emit("cv_wake", "notFull");  // despertó y readquirió el mutex
    }

    if (closed_) {
        trace::emit("unlock", "queue");
        return PushResult{false, blocked, queue_.size()};
    }

    queue_.push(req);
    const std::size_t sizeAfter = queue_.size();
    trace::emit("push", "queue", {{"id", req.id}, {"user", req.userId}, {"qty", req.tickets},
                                  {"size", static_cast<long long>(sizeAfter)}});
    if (onEnqueued) {
        onEnqueued(sizeAfter);
    }

    trace::emit("unlock", "queue");
    lock.unlock();
    trace::emit("notify", "notEmpty", {{"all", 0}});
    notEmpty_.notify_one();  // despierta a un Worker: "hay trabajo"
    return PushResult{true, blocked, sizeAfter};
}

RequestQueue::PopResult RequestQueue::pop(Request& out, const BlockHook& onBlock) {
    std::unique_lock<std::mutex> lock = acquire(mtx_);

    bool blocked = false;
    if (queue_.empty() && !closed_) {
        blocked = true;
        if (onBlock) {
            onBlock();
        }
        trace::emit("cv_wait", "notEmpty", {{"size", 0}});
        notEmpty_.wait(lock, [this] {
            return !queue_.empty() || closed_;
        });
        trace::emit("cv_wake", "notEmpty");
    }

    if (queue_.empty()) {
        // closed_ == true y no queda trabajo pendiente: el Worker debe terminar.
        trace::emit("unlock", "queue");
        return PopResult{false, blocked};
    }

    out = queue_.front();
    queue_.pop();
    trace::emit("pop", "queue", {{"id", out.id}, {"size", static_cast<long long>(queue_.size())}});

    trace::emit("unlock", "queue");
    lock.unlock();
    trace::emit("notify", "notFull", {{"all", 0}});
    notFull_.notify_one();  // despierta al Dispatcher: "hay espacio"
    return PopResult{true, blocked};
}

void RequestQueue::close() {
    {
        std::unique_lock<std::mutex> lock = acquire(mtx_);
        closed_ = true;
        trace::emit("close", "queue");
        trace::emit("unlock", "queue");
    }
    trace::emit("notify", "notEmpty", {{"all", 1}});
    notEmpty_.notify_all();
    trace::emit("notify", "notFull", {{"all", 1}});
    notFull_.notify_all();
}
