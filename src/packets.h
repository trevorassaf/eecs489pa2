#pragma once

#include <sys/types.h>
#include "ltga.h"

#define NETIMG_WIDTH  640
#define NETIMG_HEIGHT 480

#define NETIMG_MAXFNAME    256  // including terminating NULL
#define NETIMG_PORTSEP     ':'

#define NETIMG_VERS 0x2

#define NETIMG_QRY 0x1
#define NETIMG_RPY 0x2

#define NETIMG_FOUND 1
#define NETIMG_NFOUND 0
#define NETIMG_EVERS -1
#define NETIMG_ESIZE -2
#define NETIMG_EBUSY -3

#define NETIMG_QLEN       10 
#define NETIMG_LINGER      2
#define NETIMG_NUMSEG     50
#define NETIMG_MSS      1440
#define NETIMG_USLEEP 250000    // 250 ms

#define QUERY_TIMEOUT_SECS 1 
#define QUERY_TIMEOUT_MICROS 0 

#define PR_MAXPEERS 6
#define PR_MAXFQDN 256

#define PR_ADDRESS_FLAG 'p'
#define PR_PORT_DELIMITER ':'
#define PR_MAXPEERS_FLAG 'n'
#define PR_CLI_OPT_PREFIX '-'

#define PR_QLEN   10 
#define PR_LINGER 2

#define PM_VERS   0x1
#define PM_SEARCH 0x4

#define CONNECTED_MSG "connected"
#define PENDING_MSG   "pending"
#define REJECTED_MSG  "rejected"
#define AVAILABLE_MSG "available"

#define net_assert(err, errmsg) { if (!(err)) { perror(errmsg); assert((err)); } }

/**
 * Message type codes for peering traffic.
 */
enum PmMessageType {
  WELCOME  = 0x1,
  REDIRECT = 0x2,
  SEARCH = 0x4
};

enum NetimgMessageType {
  QUERY = 0x1,
  REPLY = 0x2
};

/**
 * Cli option types.
 */
enum CliOption {P, N};


/**
 * Header for all packets.
 */
struct packet_header_t {
  unsigned char vers, type;
};

/**
 * Packet for client image queries. 
 */
struct iqry_t {
  packet_header_t header;
  char iq_name[NETIMG_MAXFNAME];
};

/**
 * Packet header for message transfers.
 */
struct imsg_t {
  packet_header_t header;
  unsigned char im_found;
  unsigned char im_depth;    // in bytes, not bits as returned by LTGA.GetPixelDepth()
  unsigned short im_format;
  unsigned short im_width;
  unsigned short im_height; 
  unsigned char im_adepth;   // not used
  unsigned char im_rle;      // not used
};

/**
 * Packet formats for peer redirection.
 */
struct peer_addr_t {
  uint32_t ipv4;
  uint16_t port, reserved;
};

/**
 * Packet for p2p image-network query.
 */
struct p2p_image_query_t {
  packet_header_t header;
  unsigned short search_id;
  peer_addr_t orig_peer;
  char file_name[NETIMG_MAXFNAME];
};

/**
 * Header for recommending peers.
 */
struct peering_response_header_t {
  packet_header_t header;
  uint16_t num_peers;
};

extern int sd;
extern long img_size;
extern char *image;

extern int imgdb_loadimg(const char* fname, LTGA* image, imsg_t* imsg, long* img_size);
extern void netimg_glutinit(int *argc, char *argv[], void (*idlefunc)());
extern void netimg_imginit();
