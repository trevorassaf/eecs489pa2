#include "ServerBuilder.h"

ServerBuilder::ServerBuilder() :
  hasRemoteDomainName_(false),
  localPort_(0),
  remotePort_(0),
  hasLocalPort_(false),
  remoteIpv4Address_(0),
  hasRemoteIpv4Address_(false),
  shouldEnableAddressReuse_(false) 
{}

ServerBuilder& ServerBuilder::setRemoteDomainName(const std::string& domain_name) {
  remoteDomainName_ = domain_name;
  hasRemoteDomainName_ = true;
  return *this;
}

ServerBuilder& ServerBuilder::setLocalPort(uint16_t port) {
  localPort_ = port;
  hasLocalPort_ = true;
  return *this;
}

ServerBuilder& ServerBuilder::setRemotePort(uint16_t port) {
  remotePort_ = port;
  return *this;
}

uint16_t ServerBuilder::getRemotePort() const {
  return remotePort_;
}

ServerBuilder& ServerBuilder::setRemoteIpv4Address(uint32_t ipv4_addr) {
  remoteIpv4Address_ = ipv4_addr;
  hasRemoteIpv4Address_ = true;
  return *this;
}

uint32_t ServerBuilder::getRemoteIpv4Address() const {
  // Fail b/c we don't have a remote ipv4 yet
  assert(hasRemoteIpv4Address_);

  return remoteIpv4Address_;
}

ServerBuilder& ServerBuilder::enableAddressReuse() {
  shouldEnableAddressReuse_ = true;
  return *this;
}

ServerBuilder& ServerBuilder::disableAddressReuse() {
  shouldEnableAddressReuse_ = false;
  return *this;
}

Connection ServerBuilder::build() const {
  // Fail b/c remote was not specified
  assert(hasRemoteIpv4Address_ || hasRemoteDomainName_);
  
  // Initialize socket
  int sd = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sd == -1) {
    throw SocketException("Failed to start server socket.");
  }

  // Configure socket for address reuse
  if (shouldEnableAddressReuse_) {
    int reuseaddr_optval = 1;
    // Configure address for reuse
    if (::setsockopt(
          sd,
          SOL_SOCKET,
          SO_REUSEADDR,
          &reuseaddr_optval,
          sizeof(int)) == -1
    ) {
      throw SocketException("Failed to configure socket for address reuse.");
    }
    
    // Configure port for reuse
    if (::setsockopt(
          sd,
          SOL_SOCKET,
          SO_REUSEPORT,
          &reuseaddr_optval,
          sizeof(int)) == -1
    ) {
      throw SocketException("Failed to configure socket for port reuse.");
    }
  }

  // Bind to local address (bind before connect pattern)
  if (hasLocalPort_) {
    struct sockaddr_in local_addr;
    size_t local_addr_len = sizeof(local_addr);
    memset(&local_addr, 0, local_addr_len);
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(localPort_);

    if (::bind(sd, (struct sockaddr *) &local_addr, local_addr_len) == -1) {
      throw SocketException("Failed to bind local address for outgoing connection.");
    }
  }

  // Lookup peer server address
  struct sockaddr_in server;
  size_t size_server = sizeof(server);

  memset(&server, 0, size_server);
  server.sin_family = AF_INET;
  server.sin_port = htons(remotePort_);

  // Identify target server by ipv4 address or domain name
  if (hasRemoteIpv4Address_) {
    server.sin_addr.s_addr = htonl(remoteIpv4Address_);  
  } else if (hasRemoteDomainName_) {
    struct hostent *sp = ::gethostbyname(remoteDomainName_.c_str());
    memcpy(&server.sin_addr, sp->h_addr, sp->h_length);
  } 

  // Connect to peer server
  if (::connect(sd, (struct sockaddr *) &server, size_server) == -1) {
    if (errno == EADDRNOTAVAIL) {
      throw BusyAddressSocketException("Address is in use, already connected.");
    }

    throw SocketException("Failed to connect to peer server: " + remoteDomainName_);
  }

  return Connection(sd);
}
