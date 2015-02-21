#pragma once

#include "SocketException.h"
#include "Service.h"

#include <string.h>        // memset(), memcmp(), strlen(), strcpy(), memcpy()
#include <unistd.h>        // getopt(), STDIN_FILENO, gethostname()
#include <signal.h>        // signal()
#include <netdb.h>         // gethostbyname(), gethostbyaddr()
#include <netinet/in.h>    // struct in_addr
#include <arpa/inet.h>     // htons(), inet_ntoa()
#include <sys/types.h>     // u_short
#include <sys/socket.h>    // socket API, setsockopt(), getsockname()
#include <sys/select.h>    // select(), FD_*

#define BACKLOG_LEN 10 

class ServiceBuilder {

  private:
    /**
     * Port of service in host-byte-order. If 0, then start service 
     * on ephemeral port.
     */
    uint16_t port_;

    /**
     * Maximum allowed queue length for incomming connections.
     */
    unsigned int backlog_;

    /**
     * Indicates that this socket should permit address reuse.
     */
    bool isAddressReuseEnabled_;

    /**
     * Indicates that this socket should linger after it's closed. 
     */
    bool isLingerEnabled_;

    /**
     * Specifies the amount of time that the socket should linger.
     */
    unsigned int lingerDuration_;

    /**
     * initSocket()
     * - Initialize listening socket.
     * @return sd of socket
     */
    int initSocket() const;

  public:
    /**
     * ServiceBuilder()
     * - Ctor for ServiceBuilder.
     */
    ServiceBuilder();

    /**
     * setPort()
     * - Sets port value. 
     * @param port : port number in host-byte-order. 
     */
    ServiceBuilder& setPort(uint16_t port);

    /**
     * setBacklog()
     * - Set backlog for this socket.
     */
    ServiceBuilder& setBacklog(unsigned int backlog);

    /**
     * enableAddressReuse()
     * - Configure socket for peering.
     */
    ServiceBuilder& enableAddressReuse();
    
    /**
     * disableAddressReuse()
     * - Configure socket to prevent.
     */
    ServiceBuilder& disableAddressReuse();

    /**
     * enableLinger()
     * - Configure socket for linger.
     * @param duration : linger time  
     */
    ServiceBuilder& enableLinger(unsigned int duration); 

    /**
     * disableLinger()
     * - Configure socket to prevent.
     */
    ServiceBuilder& disableLinger(); 

    /**
     * build()
     * - Spawn socket and set to listen on port.
     */
    const Service build() const; 

    /**
     * build()
     * - Spawn socket w/dynamic memory and set to listen on port.
     */
    const Service* buildNew() const;
};
