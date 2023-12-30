// #include "zhelpers.hpp"
#include <pthread.h>
#include <queue>
#include "messenger.h"
#include <iostream>

#include <signal.h>

static volatile int s_interrupted = 0;
static void s_signal_handler (int signal_value)
{
    s_interrupted = 1;
}

static void s_catch_signals (void)
{
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
}

int main(int argc, char *argv[])
{
    std::cout << "Hello ZMQ" << std::endl;
    s_catch_signals();
    auto &mn = messenger::MessengerManager::get_instance();
    bool init_xsub_xpub = true;
    std::vector<int> cam_list = {1,2,3};

    mn.register_xsub_xpub(cam_list, init_xsub_xpub);

    // std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    std::unordered_map<int, std::unique_ptr<messenger::Subscriber>> subscribers_;
    int xpub_port = 1111;
    std::string subcribe_address = "tcp://127.0.0.1:" + std::to_string(xpub_port);

    auto handler = [&](const std::string &topic, const std::string &message) {
        std::cout << "Topic: " << topic <<", Message: " << message << std::endl;
    };
    
    for (auto &camid: cam_list) {
        auto camid_str = "cam" + std::to_string(camid);
        subscribers_[camid] = std::make_unique<messenger::ZeroMQSubscriber>(std::move(handler));
        subscribers_[camid]->connect(subcribe_address);
        subscribers_[camid]->subscribe({camid_str});
        subscribers_[camid]->start_listening();
    }

    while(1) {
        try {
            for (auto &camid: cam_list) {
                auto camid_str = "cam" + std::to_string(camid);
                mn.publish_message(camid_str, camid_str, camid_str);
            }
        }
        catch(zmq::error_t& e) {
            std::cout << "W: interrupt publish, proceeding…" << std::endl;
        }
        if (s_interrupted) {
            std::cout << "W: interrupt published, killing server…" << std::endl;
            break;
        }
    }

    return 0;
}
