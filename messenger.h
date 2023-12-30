
#ifndef SDKCLIENT_DEEPSTREAM_LIBS_MESSENGERS_MESSENGER_H_
#define SDKCLIENT_DEEPSTREAM_LIBS_MESSENGERS_MESSENGER_H_

#include <string>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <optional>
#include "pub_sub_interface.h"
#include "thread_safe_queue.h"

namespace messenger {

using connection_id_t = uint64_t;

class Messenger {
 public:
  Messenger(const std::string &zmq_address, bool init_xsub_xpub = false);
  void publish(const std::string &topic, const std::string &message);
  void terminate();

  Messenger(Messenger &&) = delete;

  ~Messenger();

 private:
  void init_publisher();
  void publishing_loop();
  void run_task();
  void init_xpub_xsub();

  const std::string zmq_address_;
  std::unique_ptr<Publisher> publisher_;
  std::thread loop_thread_;
  std::thread loop_xpub_xsub;
  bool is_running_{true}, init_xsub_xpub_{false};
  ThreadSafeQueue<Message> pending_messages_;
};

class MessengerManager {
 public:
  bool publish_message(const std::string &key, const std::string &topic,
                       const std::string &message);

  void set_publisher(const std::string &cam_id);

  void register_xsub_xpub(std::vector<int> &cam_list, bool init_xsub_xpub = false);

  static MessengerManager &get_instance();

 private:
  MessengerManager();

  std::unordered_map<std::string, std::unique_ptr<Messenger>> messenger_list_;
//  std::unique_ptr<Messenger> messenger_pub;
};

bool publish_message(const std::string &camid, const std::string &topic, const std::string &message);

} // namespace messenger

#endif //SDKCLIENT_DEEPSTREAM_LIBS_MESSENGERS_MESSENGER_H_
