#pragma once

#include <string.h>        // memset(), memcmp(), strlen(), strcpy(), memcpy()
#include <unistd.h>        // getopt(), STDIN_FILENO, gethostname()
#include <signal.h>        // signal()
#include <netdb.h>         // gethostbyname(), gethostbyaddr()
#include <netinet/in.h>    // struct in_addr
#include <arpa/inet.h>     // htons(), inet_ntoa()
#include <sys/types.h>     // u_short
#include <sys/socket.h>    // socket API, setsockopt(), getsockname()
#include <sys/select.h>    // select(), FD_*

#include "SocketException.h"

#define BUFFER_SIZE 1024

class Connection {

  private:
    /**
     * File descriptor for socket.
     */
    int fileDescriptor_;

    /**
     * Ports for local and remote connections in host-byte-order.
     */
    uint16_t localPort_, remotePort_;

    /**
     * Ipv4 address for local and remote connections in host-byte-order.
     */
    uint32_t localIpv4_, remoteIpv4_;

    /**
     * Domain names for local and remote hosts.
     */
    std::string localDomainName_, remoteDomainName_;

  public:
    /**
     * Connection()
     * - Ctor for Connection.
     * @param file_descriptror : socket fd
     */
    explicit Connection(int file_descriptor); 

    /**
     * getFd()
     * - Return fild descriptor for socket.
     */
    int getFd() const;

    /**
     * read()
     * - Read data from socket. Return data as string.
     */
    const std::string read() const;

    /**
     * write()
     * - Write data to socket.
     * @param data : string of data to write to socket
     * @return number of unsent chars 
     */
    size_t write(const std::string data) const;

    /**
     * getLocalPort()
     * - Return port of local connection in host-byte-order.
     */
    uint16_t getLocalPort() const;

    /**
     * getRemotePort()
     * - Return port that remote is listening on in host-byte-order.
     */
    uint16_t getRemotePort() const;

    /**
     * getRemoteIpv4()
     * - Return ipv4 of local host in host-byte-order.
     */
    uint32_t getLocalIpv4() const;
    
    /**
     * getRemoteIpv4()
     * - Return ipv4 of remote in host-byte-order.
     */
    uint32_t getRemoteIpv4() const;

    /**
     * getLocalDomainName()
     * - Return domain name of localhost.
     */
    const std::string& getLocalDomainName() const;

    /**
     * getRemoteDomainName()
     * - Return domain name of remote host.
     */
    const std::string& getRemoteDomainName() const;

    /**
     * operator==
     * - Return true iff the local/remote name/ports are equivalent.
     */
    bool operator==(const Connection& other) const;

    /**
     * operator!=
     * - Return true iff operator== returns false.
     */
    bool operator!=(const Connection& other) const;
    
    /**
     * close()
     * - Close socket.
     */
    void close() const;

};
