
#ifndef THREAD_SAFE_QUEUE
#define THREAD_SAFE_QUEUE

#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>

template <typename T>
class ThreadSafeQueue {
 public:
  ThreadSafeQueue() {}

  void push(T new_value) {
    auto data = std::make_shared<T>(std::move(new_value)); // this can throw, but it's OK
    std::lock_guard lock(m_);
    data_queue_.push(data);
    cond_.notify_one();
  }

  bool empty() const {
    std::lock_guard lock(m_);
    return data_queue_.empty();
  }

  int size() const {
    std::lock_guard lock(m_);
    return data_queue_.size();
  }

  void wait_and_pop(T &value) {
    std::unique_lock lock(m_);
    cond_.wait(lock, [this] {
      return !data_queue_.empty();
    });

    value = std::move(*data_queue_.front());
    data_queue_.pop();
  }

  bool try_pop(T &value) {
    std::lock_guard lock(m_);
    if (data_queue_.empty()) {
      return false;
    }

    value = std::move(*data_queue_.front());
    data_queue_.pop();
    return true;
  }

  std::shared_ptr<T> wait_and_pop() {
    std::unique_lock lock(m_);
    cond_.wait(lock, [this] {
      return !data_queue_.empty();
    });

    auto res = data_queue_.front(); // safe, cannot throw
    data_queue_.pop();
    return res;
  }

  std::shared_ptr<T> try_pop() {
    std::lock_guard lock(m_);
    if (data_queue_.empty()) {
      return {};
    }

    auto res = data_queue_.front();
    data_queue_.pop();
    return res;
  }

 private:
  mutable std::mutex m_;
  std::queue<std::shared_ptr<T>> data_queue_;
  std::condition_variable cond_;
};

#endif //THREAD_SAFE_QUEUE
