#pragma once

#include "hash.h"

#define DHTN_UNITTESTING 0

#define DHTN_UNINIT_SD -1
#define DHTN_FINGERS 8  // reaches half of 2^8-1
                        // with integer IDs, fingers[0] is immediate successor
#define DHTM_VERS  0x2
#define DHTM_TTL   10
#define DHTM_QRY 0x01  // 0x01
#define DHTM_RPY 0x02  // 0x02
#define DHTM_JOIN  0x08
#define DHTM_WLCM  0x04
#define DHTM_REID  0x0c
#define DHTM_REDRT 0x40
#define DHTM_ATLOC 0x80

// the following are used in PA2
#define DHTM_SRCH 0x10   // image search on the DHT
#define DHTM_RPLY 0x20   // reply to image search on the DHT
#define DHTM_MISS 0x22   // image not found on the DHT 

#define DHT_MAX_FILE_NAME 256

enum DhtType {
  JOIN = 0x08,
  JOIN_ATLOC = (DHTM_ATLOC | JOIN),
  REDRT = 0x40,
  REID = 0x0c,
  WLCM = 0x04,
  SRCH = 0x10,
  SRCH_ATLOC = (DHTM_ATLOC | SRCH),
  RPLY = 0x20,
  MISS = 0x22
};

typedef struct {
  uint8_t vers; // must be DHTM_VERS
  uint8_t type; // [REDRT | JOIN | REID]
} dhtheader_t;  // 2 bytes

typedef struct {            // inherit from struct sockaddr_in
  uint8_t rsvd;        // == sizeof(sin_len)
  uint8_t id;          // == sizeof(sin_family)
  uint16_t port;       // port#, always stored in network byte order
  uint32_t ipv4;       // IPv4 address
} dhtnode_t;           // 8 bytes

typedef struct {
  dhtheader_t header;
  uint16_t ttl;       // used by JOIN only
  dhtnode_t node;     // REDRT: new successor
                      // JOIN: node attempting to join DHT
} dhtmsg_t;           // 12 bytes

typedef struct {
  dhtmsg_t msg;
  dhtnode_t predecessor;      // WLCM: predecessor node 
} dhtwlcm_t;

typedef struct {
  uint8_t id;
  char name[DHT_MAX_FILE_NAME];
} dhtimg_t;

typedef struct {            // PA2
  dhtmsg_t msg;                
  dhtimg_t img;
} dhtsrch_t;                // used by QUERY, REPLY, and MISS
