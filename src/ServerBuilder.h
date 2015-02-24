#pragma once

#include <string>
#include <assert.h>
#include <unistd.h>        // getopt(), STDIN_FILENO, gethostname()
#include <signal.h>        // signal()
#include <netdb.h>         // gethostbyname(), gethostbyaddr()
#include <netinet/in.h>    // struct in_addr
#include <arpa/inet.h>     // htons(), inet_ntoa()
#include <sys/types.h>     // u_short
#include <sys/socket.h>    // socket API, setsockopt(), getsockname()
#include <sys/select.h>    // select(), FD_*
#include <errno.h>

#include "Connection.h"
#include "SocketException.h"

class ServerBuilder {

  private:
    /**
     * Domain name of target.
     */
    std::string remoteDomainName_;

    /**
     * Specifies if remote domain name has been indicated.
     */
    bool hasRemoteDomainName_;

    /**
     * Port of target server in host-byte-order.
     */
    uint16_t localPort_, remotePort_;

    /**
     * Specifies if a local port has been indicated.
     */
    bool hasLocalPort_;

    /**
     * Ipv4 address of target in host-byte-order.
     */
    uint32_t remoteIpv4Address_;
    
    /**
     * Specifies if IPv4 address of remote machine is provided.
     */
    bool hasRemoteIpv4Address_;

    /**
     * Indicates whether the socket's address may be reused or not.
     */
    bool shouldEnableAddressReuse_;

  public:
    /**
     * ServerBuilder()
     * - Ctor for ServerBuilder.
     */
    ServerBuilder();

    /**
     * setRemoteDomainName()
     * - Establish domain name.
     */
    ServerBuilder& setRemoteDomainName(const std::string& domain_name);
    
    /**
     * setLocalPort()
     * - Set local port. Means that socket must bind() before
     *   calling connect()
     * @param port : local port in host-byte-order  
     */
    ServerBuilder& setLocalPort(uint16_t port);

    /**
     * setRemotePort()
     * - Set target's port.
     * @param port : port value in host-byte-order  
     */
    ServerBuilder& setRemotePort(uint16_t port);

    /**
     * getRemotePort()
     * - Return remote's port.
     */
    uint16_t getRemotePort() const;

    /**
     * setRemoteIpv4Address()
     * - Specify address of target in ipv4 format.
     * @param ipv4_addr : host's ipv4 address in host-byte-order
     */
    ServerBuilder& setRemoteIpv4Address(uint32_t ipv4_addr);

    /**
     * getRemoteIpv4Address()
     * - Return remote's ipv4 address.
     */
    uint32_t getRemoteIpv4Address() const;

    /**
     * enableAddressReuse()
     * - Configure socket for address reuse.
     */
    ServerBuilder& enableAddressReuse();

    /**
     * disableAddressReuse()
     * - Configure socket to prevent address reuse.
     */
    ServerBuilder& disableAddressReuse();

    /**
     * build()
     * - Construct Connection from ServerBuilder.
     */
    Connection build() const;
};
