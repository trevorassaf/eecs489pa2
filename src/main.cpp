#include <iostream>
#include <stdio.h>         // fprintf(), perror(), fflush()
#include <stdlib.h>        // atoi()
#include <assert.h>        // assert()
#include <limits.h>        // LONG_MAX
#include <string.h>        // memset(), memcmp(), strlen(), strcpy(), memcpy()
#include <unistd.h>        // getopt(), STDIN_FILENO, gethostname()
#include <signal.h>        // signal()
#include <netdb.h>         // gethostbyname(), gethostbyaddr()
#include <netinet/in.h>    // struct in_addr
#include <arpa/inet.h>     // htons(), inet_ntoa()
#include <sys/types.h>     // u_short
#include <sys/socket.h>    // socket API, setsockopt(), getsockname()
#include <sys/ioctl.h>     // ioctl(), FIONBIO

#include "ServiceBuilder.h"
#include "Service.h"
#include "ServerBuilder.h"
#include "Connection.h"
#include "ltga.h"
#include "SocketException.h"
#include "netimg.h"

#define SHA1_LENGTH 20 // bytes
#define ID_LENGTH 20

// Cli constants
#define MAX_NUM_CLI_ARGS 4
#define MAX_NUM_FLAGS 2

#define CLI_FLAG_TOKEN '-'
#define TARGET_DELIMITER ':'

#define TARGET_FLAG 'p'
#define ID_FLAG 'I'


/**
 * Specifies type of node as determined by cli arguments.
 */
enum NodeType {
  TARGET,
  ID_OVERRIDE,
};

/**
 * Configuration for target node.
 */
struct cli_targ_config_t {
  std::string fqdn;
  uint16_t port;
};

/**
 * Configuration for id overridden node.
 */
struct cli_id_config_t {
  char id[ID_LENGTH + 1];
};

/**
 * Configuration for node.
 */
struct cli_config_t {
  cli_targ_config_t targ_config;
  cli_id_config_t id_config;
  NodeType types[MAX_NUM_FLAGS];
  size_t num_types;
};

/**
 * failCliWithMessage()
 * - Fail the program due to invalid cli parameters.
 * @param message : message to report to user
 */
void failCliWithMessage(const std::string& message) {
  std::cout << message << "\nCli invocation: ./dhtdb [-p <node>:<port> -I <ID>]" << std::endl;
  exit(1);
}

/**
 * registerCliConfigType()
 * - Push new cli config type.
 * @param config : cli param configuration
 * @param type : node setting
 */
void registerCliConfigType(cli_config_t config, NodeType type) {
  for (size_t i = 0; i < config.num_types; ++i) {
    if (config.types[i] == type) {
      failCliWithMessage("Duplicate type found.");
    }
  }

  config.types[config.num_types] = type;
  ++config.num_types;
}

/**
 * deserializeTarget()
 * - Deserialize target fqdn and port from human-readable address.
 * @param target : <fqdn>:<port>
 */
const cli_targ_config_t deserializeTarget(const char* target) {
  std::string targ_str(target);
  size_t delim_idx = targ_str.find(TARGET_DELIMITER);
  
  // Fail due to invalid address (missing address delimiter)
  if (delim_idx == std::string::npos) {
    failCliWithMessage("Malformed target address. Missing delimiter.");
  }

  cli_targ_config_t targ_config;
  targ_config.fqdn = targ_str.substr(0, delim_idx);

  try {
    targ_config.port = (uint16_t) std::stoi(targ_str.substr(delim_idx + 1));
  } catch (...) {
    failCliWithMessage("Non-numeric port number.");    
  }

  return targ_config;
}

/**
 * deserializeId()
 * - Parse and validated id from cli string.
 * @param id : id string
 */
const cli_id_config_t deserializeId(const char* id) {
  // Fail b/c 'id' exceeds legal length
  if (strlen(id) > ID_LENGTH) {
    failCliWithMessage("Invalid id -- to long.");
  }

  cli_id_config_t id_config;
  memcpy(id_config.id, id, ID_LENGTH);
  id_config.id[ID_LENGTH] = '\0';

  return id_config;
}

/**
 * processCliParam()
 * - Deserialize cli params.
 * @param config : current cli configuration
 * @param cli_params : remaining cli parameters
 * @param num_cli_params : count of remaining cli params
 */
size_t processCliParam(cli_config_t config, const char * const * cli_params, size_t num_cli_params) {
  // Fail bc 'num_cli_params' should never be 0
  assert(num_cli_params);

  // Fail bc 'config.types' shouldn't be maxed out
  assert(config.num_types != MAX_NUM_FLAGS);

  char flag_token = **cli_params;
  
  // Fail due to invalid flag token
  if (flag_token != CLI_FLAG_TOKEN) {
    failCliWithMessage(std::string("Invalid flag-token: ") + flag_token);
    exit(1);
  }

  const char* flag_str = *cli_params;
  
  // Fail due to invalid flag length
  if (strlen(flag_str) != 2) {
    failCliWithMessage(std::string("Invalid flag length: ") + flag_str);
  }

  char flag = *(flag_str + 1);
  const char* param_str = *(cli_params + 1);
  size_t num_consumed_cli_params = 0;

  switch (flag) {
    case TARGET_FLAG: {
      const cli_targ_config_t targ_config = deserializeTarget(param_str); 
      registerCliConfigType(config, TARGET);
      config.targ_config = targ_config;
      num_consumed_cli_params = 1;
      break;
    }
    case ID_FLAG: {
      const cli_id_config_t id_config = deserializeId(param_str);
      registerCliConfigType(config, ID_OVERRIDE);
      config.id_config = id_config;
      num_consumed_cli_params = 1;
      break;
    }
    default:
      failCliWithMessage(std::string("Invalid flag: ") + flag);
  }

  ++config.num_types;

  return num_consumed_cli_params + 1;
}

/**
 * processDhtdbArgs()
 * - Validate and deserialize command line arguments of dhtdb.
 * @param argc : number of cli params (including program name)
 * @param argv : array of string arguments 
 */
const cli_config_t processDhtdbArgs(int argc, const char * const * argv) {
  size_t num_args = argc - 1;
  
  // Fail due to superfluous cli args
  if (num_args > MAX_NUM_CLI_ARGS) {
    failCliWithMessage("Too many cli arguments.");
  } 

  // Process cli args
  cli_config_t config;
  size_t arg_idx = 0;

  while (arg_idx != num_args) {
    arg_idx += processCliParam(config, argv + arg_idx, num_args - arg_idx);  
  }

  return config;
}

int main(int argc, char** argv) {
  // Process cli args 
  const cli_config_t config = processDhtdbArgs(argc, argv);

  // Initialize listening socket for accepting joins
  ServiceBuilder service_builder;
  Service dht_service = service_builder.build();

  
  return 0;
}
