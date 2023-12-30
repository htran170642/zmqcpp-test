#include "messenger.h"
#define ZMQ_MESSAGE_QUEUE_MAX_SIZE 1000

namespace messenger {

Messenger::Messenger(const std::string &zmq_address, bool init_xsub_xpub)
    : zmq_address_(zmq_address), init_xsub_xpub_(init_xsub_xpub) {
  if (init_xsub_xpub) {
    loop_xpub_xsub = std::thread(&Messenger::init_xpub_xsub, this);
    loop_xpub_xsub.detach();
    std::this_thread::sleep_for(std::chrono::seconds(2)); // sleep sometimes to make sure create the connection
  }
  loop_thread_ = std::thread(&Messenger::run_task, this);
}

Messenger::~Messenger() {
  terminate();
}

void Messenger::terminate() {
  if (is_running_) {
    is_running_ = false;
    // push an empty message to make sure the loop is woke up
    pending_messages_.push({});
    loop_thread_.join();
  }
}

void Messenger::init_publisher() {
  publisher_ = std::make_unique<messenger::ZeroMQPublisher>();
  publisher_->connect(zmq_address_);
}

void Messenger::publish(const std::string &topic, const std::string &message) {
  if (pending_messages_.size() > ZMQ_MESSAGE_QUEUE_MAX_SIZE){
    Message pending_message;;
    std::cout << "pending_messages_ is full with size" << ZMQ_MESSAGE_QUEUE_MAX_SIZE << std::endl;
    pending_messages_.wait_and_pop(pending_message);
  }

  // std::cout << "Queue size at " << topic<< " with " << pending_messages_.size() << std::endl;
  pending_messages_.push({topic, message});
}

void Messenger::publishing_loop() {
  init_publisher();

  while (is_running_) {
    Message pending_message;
    pending_messages_.wait_and_pop(pending_message);
    publisher_->publish(pending_message.topic, pending_message.message);
  }
}

void Messenger::run_task() {
  try {
    publishing_loop();
  } catch (zmq::error_t &err) {
    std::cout << " ERROR: Messenger loop exited with error " << err.what() << std::endl;
    terminate();
  }
}

void Messenger::init_xpub_xsub() {
  zmq::context_t ctx(1);
  zmq::socket_t frontend(ctx, ZMQ_XSUB);
  zmq::socket_t backend(ctx, ZMQ_XPUB);
  auto xpub_port = 1111;
  auto xsub_port = 1112;
  std::string pub_address = "tcp://*:" + std::to_string(xpub_port);
  std::string sub_address = "tcp://*:" + std::to_string(xsub_port);
  //
  frontend.bind(sub_address);
  frontend.set(zmq::sockopt::rcvtimeo,
                   1000); // wait up to 1 second for a message to arrive

  int hwm_sub = 10000;
  frontend.set(zmq::sockopt::rcvhwm, hwm_sub);
  //
  backend.bind(pub_address);
  backend.set(zmq::sockopt::recovery_ivl, 1000);
  // discard unsent messages immediately on close
  backend.set(zmq::sockopt::linger, 0);
  int hwm_pub = 10000;
  backend.set(zmq::sockopt::sndhwm, hwm_pub);
  zmq::proxy(static_cast<zmq::socket_ref>(frontend), static_cast<zmq::socket_ref>(backend), nullptr);
}

MessengerManager::MessengerManager() {
}

void MessengerManager::register_xsub_xpub(std::vector<int> &cam_list, bool init_xsub_xpub) {

  auto xsub_port = 1112;
  std::string bind_address = "tcp://localhost:" + std::to_string(xsub_port);
//  messenger_pub = std::make_unique<Messenger>(bind_address, true);
  messenger_list_.reserve(cam_list.size());
  for (const auto &camid : cam_list) {
    auto camid_str = "cam" + std::to_string(camid);
    messenger_list_[camid_str] = std::make_unique<Messenger>(bind_address, init_xsub_xpub);
    init_xsub_xpub = false;
  }
}

bool MessengerManager::publish_message(const std::string &key, const std::string &topic, const std::string &message) {
  set_publisher(key);
  messenger_list_[key]->publish(topic, message);
  return true;
}

void MessengerManager::set_publisher(const std::string &cam_id) {
  if (messenger_list_.find(cam_id) == messenger_list_.end()) {
    std::cout << "Warning: you're trying to publish to unregistered key = " << cam_id << std::endl;
    auto xsub_port = 1112;
    std::string bind_address = "tcp://localhost:" + std::to_string(xsub_port);
    messenger_list_[cam_id] = std::make_unique<Messenger>(bind_address);
  }
}

MessengerManager &MessengerManager::get_instance() {
  static MessengerManager messenger_manager;
  return messenger_manager;
}

bool publish_message(const std::string &camid, const std::string &topic, const std::string &message) {
  auto &mn = MessengerManager::get_instance();
  return mn.publish_message(camid, topic, message);
}

}