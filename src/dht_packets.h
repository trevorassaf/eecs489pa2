#pragma once

#include "hash.h"
#include "netimg.h"

#define DHTN_UNITTESTING 0

#define DHTN_UNINIT_SD -1
#define DHTN_FINGERS 8  // reaches half of 2^8-1
                        // with integer IDs, fingers[0] is immediate successor
#define DHTM_VERS  0x2
#define DHTM_TTL   10
#define DHTM_QRY NETIMG_QRY  // 0x01
#define DHTM_RPY NETIMG_RPY  // 0x02
#define DHTM_JOIN  0x08
#define DHTM_WLCM  0x04
#define DHTM_REID  0x0c
#define DHTM_REDRT 0x40
#define DHTM_ATLOC 0x80

// the following are used in PA2
#define DHTM_SRCH 0x10   // image search on the DHT
#define DHTM_RPLY 0x20   // reply to image search on the DHT
#define DHTM_MISS 0x22   // image not found on the DHT 

enum DhtType {
  JOIN = 0x08,
  REDRT = 0x40,
  REID = 0x0c,
  WLCM = 0x04
};

typedef struct {
  uint8_t vers; // must be DHTM_VERS
  uint8_t type; // [REDRT | JOIN | REID]
} dhtheader_t;

typedef struct {            // inherit from struct sockaddr_in
  uint8_t rsvd;  // == sizeof(sin_len)
  uint8_t id;    // == sizeof(sin_family)
  uint16_t port;        // port#, always stored in network byte order
  struct in_addr ipv4; // IPv4 address
} dhtnode_t;

typedef struct {
  uint16_t ttl;     // used by JOIN only
  dhtnode_t node;   // REDRT: new successor
                    // JOIN: node attempting to join DHT
} dhtmsg_body_t;

typedef struct {
  dhtheader_t header;
  dhtmsg_body_t body;
} dhtmsg_t;

typedef struct {
  dhtmsg_t msg;   // WLCM: successor node
  dhtnode_t pred; // WLCM: predecessor node
} dhtwlcm_t;

/*
typedef struct {
  unsigned char dhtm_vers;  // must be DHTM_VERS
  unsigned char dhtm_type;  // DHTM_WLCM
  u_short dhtm_ttl;         // reserved
  dhtnode_t dhtm_node;      // WLCM: successor node
  dhtnode_t dhtm_pred;      // WLCM: predecessor node 
} dhtwlcm_t;
*/

typedef struct {            // PA2
  dhtmsg_t msg;                
  uint8_t imgID;
  char dhts_name[NETIMG_MAXFNAME];
} dhtsrch_t;                // used by QUERY, REPLY, and MISS
