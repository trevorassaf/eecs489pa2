#include "ServiceBuilder.h"

ServiceBuilder::ServiceBuilder() :
    port_(0),
    backlog_(BACKLOG_LEN),
    isAddressReuseEnabled_(false),
    isLingerEnabled_(false),
    lingerDuration_(0) {}

ServiceBuilder& ServiceBuilder::setPort(uint16_t port) {
  port_ = port;
  return *this;
}

ServiceBuilder& ServiceBuilder::setBacklog(unsigned int backlog) {
  backlog_ = backlog;
  return *this;
}

ServiceBuilder& ServiceBuilder::enableAddressReuse() {
  isAddressReuseEnabled_ = true;
  return *this;
}

ServiceBuilder& ServiceBuilder::disableAddressReuse() {
  isAddressReuseEnabled_ = false;
  return *this;
}

ServiceBuilder& ServiceBuilder::enableLinger(unsigned int duration) {
  isLingerEnabled_ = true;
  lingerDuration_ = duration;
  return *this;
}

ServiceBuilder& ServiceBuilder::disableLinger() {
  isLingerEnabled_ = false;
  return *this;
}

const Service ServiceBuilder::build() const {
  // Initialize socket
  int sd = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sd == -1) {
    throw SocketException("Failed to init socket");
  }

  // Configure socket for address resuse
  if (isAddressReuseEnabled_) {
    int reuseaddr_optval = 1;
    // Configure address for reuse
    if (::setsockopt(
          sd,
          SOL_SOCKET,
          SO_REUSEADDR,
          &reuseaddr_optval,
          sizeof(reuseaddr_optval)) == -1
    ) {
      throw SocketException("Failed to configure socket for address reuse");
    }
   
    // Configure port for reuse
    if (::setsockopt(
          sd,
          SOL_SOCKET,
          SO_REUSEPORT,
          &reuseaddr_optval,
          sizeof(reuseaddr_optval)) == -1
    ) {
      throw SocketException("Failed to configure socket for port reuse");
    }
  }

  // Bind socket
  struct sockaddr_in self;
  memset(&self, 0, sizeof(sockaddr_in));
  self.sin_family = AF_INET;
  self.sin_addr.s_addr = INADDR_ANY;
  self.sin_port = htons(port_);
  
  if (::bind(sd, (struct sockaddr *) &self, sizeof(self)) == -1) {
    throw SocketException("Failed to bind socket");
  }

  // Listen on socket
  if (::listen(sd, backlog_) == -1) {
    throw SocketException("Failed to listen on socket");
  }

  // Compose Service
  if (isLingerEnabled_) {
    return Service(sd, lingerDuration_);
  } else {
    return Service(sd);
  }
}
