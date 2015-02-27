#pragma once

#define NETIMG_VERS 0x2

#define NETIMG_QRY 0x1      // DO NOT REMOVE used in PA1
#define NETIMG_RPY 0x2      // DO NOT REMOVE used in PA1

/**
 * Packet types for netimg messages.
 */
enum NetimgTypes {
  NFOUND = 0x0,
  FOUND = 0x01,
  ESIZE = 0x9,
  EVERS = 0xa,
  ETYPE = 0xb,
  ENAME = 0xc,
  BUSY = 0xd
};

#define NETIMG_QLEN       10 
#define NETIMG_LINGER      2
#define NETIMG_NUMSEG     25
#define NETIMG_MSS      1440
#define NETIMG_USLEEP 250000    // 250 ms
#define NETIMG_SECTIO 1         // timeout seconds DO NOT REMOVE used in PA1
#define NETIMG_USECTIO 0        // timeout useconds DO NOT REMOVE used in PA1

#define NETIMG_WIDTH  640
#define NETIMG_HEIGHT 480

#define NETIMG_MAXFNAME    256  // including terminating NULL

typedef struct {
  uint8_t vers, type;
} netimg_header_t;

typedef struct {               
  netimg_header_t header;
  char name[NETIMG_MAXFNAME];  // must be NULL terminated
} iqry_t;

typedef struct {
  netimg_header_t header;
  unsigned char im_found;
  unsigned char im_depth;   // in bytes, not in bits as returned by LTGA.GetPixelDepth()
  unsigned short im_format;
  unsigned short im_width;
  unsigned short im_height;
  unsigned char im_adepth;  // not used
  unsigned char im_rle;     // not used
} imsg_t;

