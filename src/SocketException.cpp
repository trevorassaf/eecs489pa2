#include "SocketException.h"

SocketException::SocketException(const std::string& msg) 
  : std::runtime_error(msg) {}

BusyAddressSocketException::BusyAddressSocketException(const std::string& msg) 
  : SocketException(msg) {}
