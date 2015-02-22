#pragma once

#include <sys/select.h>
#include <sys/time.h>
#include <stdint.h>
#include <map>
#include <functional>

typedef std::function<void(int sd)> socket_callback_t;

class Selector {
  
   private:
    /**
     * Map of socket 'sd' -> callback function.
     */
    std::map<int, socket_callback_t> socketCallbacks_;
  
  public:
    /**
     * listen()
     * - Try to read input off of the wire.
     */
    void listen() const;

    /**
     * listen()
     * - Wait for input for specified time.
     * @param duration : time to wait in millis
     */
    void listen(time_t sec=0, suseconds_t usec=0) const;

    /**
     * bind()
     * - Register sd and bind callback to sd.
     */
    void bind(int sd, socket_callback_t callback);
};
