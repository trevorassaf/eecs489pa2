#pragma once

#include "Connection.h"
#include "SocketException.h"

#include <string>
#include <unistd.h>        // getopt(), STDIN_FILENO, gethostname()
#include <signal.h>        // signal()
#include <netdb.h>         // gethostbyname(), gethostbyaddr()
#include <netinet/in.h>    // struct in_addr
#include <arpa/inet.h>     // htons(), inet_ntoa()
#include <sys/types.h>     // u_short
#include <sys/socket.h>    // socket API, setsockopt(), getsockname()
#include <sys/select.h>    // select(), FD_*

#define MAXFQDN 256

class Service {

  private:
    /**
     * Socket file-descriptor.
     */
    int fileDescriptor_;

    /**
     * Port number for this service in network-byte-order.
     */
    uint16_t port_;

    /**
     * Domain name of service host.
     */
    std::string domainName_;

    /**
     * Ipv4 address of host.
     */
    uint32_t ipv4_;

    /**
     * Specifies if socket should linger after closure.
     */
    bool shouldLinger_;

    /**
     * Specifies duration of socket linger.
     */
    unsigned int lingerDuration_;

    /**
     * initService()
     * - Initialize service data.
     */
    void initService();

    /**
     * initSocket()
     * - Accept connection and spawn socket.
     * @return fd for new socket
     */
    int initSocket() const;

  public:
    /**
     * ServerSocket()
     * - Ctor for ServerSocket.
     * @param file_descriptor : fd for socket.
     * @param linger_duration : time that socket should wait in millis 
     */
    explicit Service(
        int file_descriptor,
        unsigned int linger_duration);

    /**
     * ServerSocket()
     * - Ctor for ServerSocket.
     * @param file_descriptor : fd for socket.
     */
    explicit Service(int file_descriptor);

    /**
     * getFd()
     * - Return file descriptor.
     */
    int getFd() const;

    /**
     * getPort()
     * - Return port that this service is running on in network-byte-order.
     */
    uint16_t getPort() const;

    /**
     * getDomainName()
     * - Return the domain name of this host.
     */
    const std::string& getDomainName() const;

    /**
     * getIpv4()
     * - Retur ipv4 of host.
     */
    uint32_t getIpv4() const;

    /**
     * accept()
     * - Spawn connection for client.
     */
    const Connection accept() const;
    
    /**
     * acceptNew()
     * - Spawn connection for client with dynamic memory.
     */
    const Connection* acceptNew() const;

    /**
     * close()
     * - Closes socket.
     * @throws SocketException
     */
    void close() const;
};
