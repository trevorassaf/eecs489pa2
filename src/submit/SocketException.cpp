#include "SocketException.h"

SocketException::SocketException(const std::string& msg) 
  : std::runtime_error(msg) {}

PrematurelyClosedSocketException::PrematurelyClosedSocketException(const std::string& msg) 
  : SocketException(msg) {}

BusyAddressSocketException::BusyAddressSocketException(const std::string& msg) 
  : SocketException(msg) {}
