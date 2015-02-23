#include "Selector.h"

#include <iostream>
#include <assert.h>

bool Selector::listen(time_t sec, suseconds_t usec) const {
  fd_set sd_set;
  FD_ZERO(&sd_set);
  int max_sd = 0;
  for (const auto& callback_binding : socketCallbacks_) {
    int sd = callback_binding.first;
    FD_SET(sd, &sd_set);
    max_sd = std::max(sd, max_sd);
  }

  // Wait on sockets for specified period of time
  struct timeval timeout = {sec, usec};
  int result = ::select(max_sd + 1, &sd_set, 0, 0, &timeout);

  // Fail due to invalid select return value
  if (result == -1) {
    std::cout << "Select failed! Errno: " << errno << std::endl;
    exit(1);
  }

  // Invoke callback functions for sockets with activity
  for (const auto& callback_binding : socketCallbacks_) {
    int sd = callback_binding.first;
    if (FD_ISSET(sd, &sd_set)) {
      bool continue_listening = callback_binding.second(sd); 
      if (!continue_listening) {
        return false;
      }
    }
  }

  return true;
}

void Selector::bind(int sd, socket_callback_t callback) {
  socketCallbacks_[sd] = callback; 
}

void Selector::erase(int sd) {
  // Fail b/c/ 'sd' is unset
  assert(socketCallbacks_.count(sd) == 1);
 
  // Remove callback function for socket
  socketCallbacks_.erase(sd);
}

void Selector::clear() {
  socketCallbacks_.clear(); 
}
