#ifndef PUBSUBMESSENGER__PUB_SUB_MESSENGER_H_
#define PUBSUBMESSENGER__PUB_SUB_MESSENGER_H_

#include <string_view>
#include <functional>
#include <memory>
#include <zmq.hpp>
#include <thread>
#include <iostream>
#include <deque>
#include <atomic>

using namespace std::chrono_literals;

namespace messenger {
using Handler = std::function<void(const std::string &, const std::string &)>;

struct Message {
    std::string topic;
    std::string message;

    Message() {}

    Message(std::string input_topic, std::string input_message)
        : topic(std::move(input_topic)), message(std::move(input_message)) {}
    // move constructor
    Message(Message &&other) : topic(std::move(other.topic)), message(std::move(other.message)) {}

    // move assignment operator
    Message &operator=(Message &&other) {
        topic = std::move(other.topic);
        message = std::move(other.message);
        return *this;
    }
};

class Publisher {
    public:
        virtual void publish(const std::string &topic,
                            const std::string &message) = 0;

        virtual void connect(const std::string &address) = 0;

        virtual ~Publisher() = default;                       
};


class Subscriber {
    public:

        Subscriber(Handler handler) : handler_(handler) {}

        virtual void connect(const std::string &address) const = 0;

        virtual void subscribe(const std::vector<std::string> &topics) const = 0;

        virtual auto receive() const -> Message = 0;

        void start_listening() {
            auto dispatch = [this]() {
            while (is_running_) {
                const auto &[topic, message] = receive();
                if (!topic.empty() && !message.empty()) {
                handler_(topic, message);
                }
            }

            handler_finished_ = true;
            };

            std::thread dispatch_thread(dispatch);
            dispatch_thread.detach();
        }

        void terminate() {
            if (is_running_) {
                is_running_ = false;

                // just wait for the handler to finish
                while (!handler_finished_) {
                    std::this_thread::sleep_for(10ms);
                }
            }
        }

        virtual ~Subscriber() = default;

    private:
        Handler handler_;
        std::atomic<bool> is_running_{true};
        std::atomic<bool> handler_finished_{false};
};

class ZeroMQPublisher : public Publisher {
    public:
        explicit ZeroMQPublisher();

        void publish(const std::string &topic,
                    const std::string &message) override;

        void connect(const std::string &address) override;

        ~ZeroMQPublisher() override {
            publisher_.close();
        }

    private:
        zmq::socket_t publisher_;
        zmq::context_t context_;

        inline static bool s_sendmore(zmq::socket_t &socket,
                                        const std::string &string) {
            zmq::message_t message(string.size());
            memcpy(message.data(), string.data(), string.size());

            auto result = socket.send(message, zmq::send_flags::sndmore);
            return result.has_value();
        }

        inline static bool s_send(zmq::socket_t &socket, const std::string &string,
                                    zmq::send_flags flags = zmq::send_flags::none) {
            zmq::message_t message(string.size());
            memcpy(message.data(), string.data(), string.size());

            auto result = socket.send(message, flags);
            return result.has_value();
        }
};

class ZeroMQSubscriber : public Subscriber {
    public:
        ZeroMQSubscriber(Handler handler);

        void connect(const std::string &address) const override;

        void subscribe(const std::vector<std::string> &topics) const override;

        auto receive() const -> Message override;

        ~ZeroMQSubscriber() {
            terminate();
            subscriber_->close();
        }

    private:
        std::unique_ptr<zmq::socket_t> subscriber_;
        zmq::context_t context_;
        
        inline static std::string s_recv(zmq::socket_t &socket, int flags = 0) {
            zmq::message_t message;
            auto recv_result = socket.recv(message, zmq::recv_flags(flags));
            if (recv_result < 0) {
                return {};
            }

            return std::string(static_cast<char *>(message.data()), message.size());
        }
};

}

#endif //PUBSUBMESSENGER__PUB_SUB_MESSENGER_H_