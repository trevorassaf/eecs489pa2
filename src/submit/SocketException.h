#pragma once

#include <string>
#include <stdexcept>

class SocketException : public std::runtime_error {

 public:
   /**
    * SocketException()
    * - Ctor for SocketException.
    */
   SocketException(const std::string& msg);
};

class PrematurelyClosedSocketException : public SocketException {

 public:
   /**
    * PrematurelyClosedSocketException()
    */
   PrematurelyClosedSocketException(const std::string& msg);
};

class BusyAddressSocketException : public SocketException {

 public:
   /**
    * BusyAddressSocketException()
    * - Ctor for BusyAddressSocketException.
    */
   BusyAddressSocketException(const std::string& msg);
};
