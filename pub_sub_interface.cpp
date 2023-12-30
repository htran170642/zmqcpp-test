#include "pub_sub_interface.h"

namespace messenger {

ZeroMQPublisher::ZeroMQPublisher() : context_(1) {
    publisher_ = zmq::socket_t(context_, ZMQ_PUB);
}

void ZeroMQPublisher::publish(const std::string &topic,
                              const std::string &message) {
    s_sendmore(publisher_,topic);
    s_send(publisher_,message);
}

void ZeroMQPublisher::connect(const std::string &address) {
    publisher_.connect(address);
    // retry after 1 second if a connection is lost
    publisher_.set(zmq::sockopt::recovery_ivl, 1000);
    // discard unsent messages immediately on close
    publisher_.set(zmq::sockopt::linger, 0);

    int hwm = 10000;
    publisher_.set(zmq::sockopt::sndhwm, hwm);
}

ZeroMQSubscriber::ZeroMQSubscriber(Handler handler)
    : Subscriber(handler), context_(1) {
  subscriber_ = std::make_unique<zmq::socket_t>(context_, ZMQ_SUB);
}

void ZeroMQSubscriber::connect(const std::string &address) const {
  subscriber_->connect(address);
}

void ZeroMQSubscriber::subscribe(const std::vector<std::string> &topics) const {
  for (const auto &topic : topics) {
    subscriber_->set(zmq::sockopt::subscribe, topic);
  }

  subscriber_->set(zmq::sockopt::rcvtimeo,
                   1000); // wait up to 1 second for a message to arrive

  int hwm = 10000;
  subscriber_->set(zmq::sockopt::rcvhwm, hwm);
}

auto ZeroMQSubscriber::receive() const -> Message {
  std::string topic = s_recv(*subscriber_);
  if (topic.empty()) {
    return {};
  }

  std::string msg = s_recv(*subscriber_);
  return {topic, msg};
}

}