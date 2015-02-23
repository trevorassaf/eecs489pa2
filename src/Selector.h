#pragma once

#include <sys/select.h>
#include <sys/time.h>
#include <stdint.h>
#include <map>
#include <functional>

typedef std::function<bool (int sd)> socket_callback_t;

class Selector {
  
   private:
    /**
     * Map of socket 'sd' -> callback function.
     */
    std::map<int, socket_callback_t> socketCallbacks_;
  
  public:
    /**
     * listen()
     * - Wait for input for specified time.
     * @param duration : time to wait in millis
     * @return false iff program should finish
     */
    bool listen(time_t sec=0, suseconds_t usec=0) const;

    /**
     * bind()
     * - Register sd and bind callback to sd.
     */
    void bind(int sd, socket_callback_t callback);
    
    /**
     * clear()
     * - Unset specified callback function.
     * @param sd : id for callback function 
     */
    void erase(int sd);

    /**
     * clear()
     * - Unset all callback functions.
     */
    void clear();
};
