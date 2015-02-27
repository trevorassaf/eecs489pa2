/* 
 * Copyright (c) 2014, 2015 University of Michigan, Ann Arbor.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of Michigan, Ann Arbor. The name of the University 
 * may not be used to endorse or promote products derived from this 
 * software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Authors: Allen Hillaker (hillaker@umich.edu), Sugih Jamin (jamin@eecs.umich.edu)
 *
*/
#include <stdio.h>         // fprintf(), perror(), fflush()
#include <stdlib.h>        // atoi()
#include <assert.h>        // assert()
#include <limits.h>        // LONG_MAX
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>      // socklen_t
#include "wingetopt.h"
#else
#include <string.h>        // memset(), memcmp(), strlen(), strcpy(), memcpy()
#include <unistd.h>        // getopt(), STDIN_FILENO, gethostname()
#include <signal.h>        // signal()
#include <netdb.h>         // gethostbyname()
#include <netinet/in.h>    // struct in_addr
#include <arpa/inet.h>     // htons()
#include <sys/types.h>     // u_short
#include <sys/socket.h>    // socket API
#endif
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "packets.h"

extern void netimg_imginit();
extern void netimg_glutinit(int *argc, char *argv[], void (*idlefunc)());

int sd;                   /* socket descriptor */
imsg_t imsg;
long img_size;    
long img_offset=0L;
char *image;

void
netimg_usage(char *progname)
{
  fprintf(stderr, "Usage: %s -s <server>%c<port> -q <image> [-v <version>].tga\n",
          progname, NETIMG_PORTSEP); 
  exit(1);
}

/*
 * netimg_args: parses command line args.
 *
 * Returns 0 on success or 1 on failure.  On successful return,
 * "*sname" points to the server's name, and "port" points to the port
 * to connect at server, in network byte order. Both "*sname", and
 * "port" must be allocated by caller. .  The variable "*imagename"
 * points to the name of the image to search for. The variable "*vers"
 * points to the version used for query packet. Nothing else is modified.
 */
int
netimg_args(int argc, char *argv[], char **sname, u_short *port, char **imagename, char *vers)
{
  char c, *p;
  extern char *optarg;

  if (argc < 5) {
    return (1);
  }
  
  *vers = NETIMG_VERS;
  
  while ((c = getopt(argc, argv, "s:q:v:")) != EOF) {
    switch (c) {
    case 's':
      for (p = optarg+strlen(optarg)-1;      // point to last character of addr:port arg
           p != optarg && *p != NETIMG_PORTSEP;  // search for ':' separating addr from port
           p--);
      *p++ = '\0';
      *port = htons((u_short) atoi(p)); // always stored in network byte order

      *sname = optarg;
      break;
    case 'q':
      *imagename = optarg;
      break;
    case 'v':
      *vers = (char) atoi(optarg);
      break;
    default:
      return(1);
      break;
    }
  }

  return (0);
}

/*
 * Task 1: YOUR CODE HERE: Fill out this function
 * netimg_sockinit: creates a new socket to connect to the provided server.
 * The server's FQDN and port number are provided.  The port number
 * provided is assumed to already be in netowrk byte order.
 *
 * On success, the global socket descriptor sd is initialized.
 * On error, terminates process.
 */
void
netimg_sockinit(char *sname, u_short port)
{
#ifdef _WIN32
  WSADATA wsa;
  
  err = WSAStartup(MAKEWORD(2,2), &wsa);  // winsock 2.2
#endif

  /* 
   * create a new TCP socket, store the socket in the global variable sd
  */
    sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    // Fail due to bad local socket
    if (sd == -1) {
      fprintf(stderr, "Client couldn't create local socket.");
      exit(1);
    }

  /* 
   * obtain the server's IPv4 address from sname and initialize the
   *  socket address with server's address and port number . 
  */
    struct sockaddr_in server;
    size_t size_server = sizeof(server);

    memset((char *) &server, 0, size_server);
    server.sin_family = AF_INET;
    server.sin_port = port;

    struct hostent *sp = gethostbyname(sname);
    memcpy(&server.sin_addr, sp->h_addr, sp->h_length);

  /* connect to server */
    int result = connect(sd, (struct sockaddr *) &server, size_server);
    
    // Fail due to bad connection
    if (result == -1) {
        fprintf(stderr, "Client couldn't connect to server socket.");
        exit(1);
    }
}

/*
 * netimg_sendqry: send a query for provided imagename to connected
 * server (using the global variable "sd" for the connected socket).
 * Query is of type iqry_t, defined in netimg.h.  Set the iq_vers
 * field of the query packet to the provided "vers". The query message
 * carries the image filename the client is searching for.
 *
 * On send error, return 0, else return 1
 */
int
netimg_sendqry(char *imagename, char vers)
{
  int bytes;
  iqry_t iqry;

  iqry.header.vers = vers;
  iqry.header.type = NETIMG_QRY;
  strcpy(iqry.iq_name, imagename);

  bytes = send(sd, (char *) &iqry, sizeof(iqry_t), 0);
  if (bytes != sizeof(iqry_t)) {
    return(0);
  }

  return(1);
}

/*
 * netimg_recvimsg: receive an imsg_t packet from server and store it
 * in the global variable imsg. Return NETIMG_OK if image received
 * successfully.  Otherwise return NETIMG_EVERS if packet is of the
 * wrong version.  Return NETIMG_ESIZE if packet received is of the
 * wrong size.  Upon return, allthe integer fields of imsg are in host
 * byte order.
 */
unsigned char
netimg_recvimsg()
{
  double img_dsize;
  /*
   * Task 1: YOUR CODE HERE 
   * netimg_recvimsg: receive an imsg_t packet from server and store it 
   * in the global variable imsg.
   * If message is of a wrong version number, return NETIMG_EVERS.
   * If message is of the wrong size, return NETIMG_ESIZE.
   * Convert the integer fields of imsg back to host byte order.
  */

    // Read imsg packet
    size_t buff_size = sizeof(imsg_t);
    int bytes_read = recv(sd, (char *) &imsg, buff_size, 0);

    // Fail due to bad imsg read
    if (bytes_read == -1) {
      fprintf(stderr, "Failed to read imsg");
      exit(1);
    }

    // Validate packet size
    if ((size_t) bytes_read != buff_size) {
      fprintf(stderr, "Wrong number of bytes sent by server during imsg read: expected: %i, received: %i", (int) buff_size, (int) bytes_read);
      return NETIMG_ESIZE;
    }
    
    // Validate packet version number
    if (imsg.header.vers != NETIMG_VERS) {
        fprintf(stderr, "Bad version number: %c\n", (char) imsg.header.vers);
        return NETIMG_EVERS;
    }

    // Convert fields to host byte order
    imsg.im_format = ntohs(imsg.im_format);
    imsg.im_width = ntohs(imsg.im_width);
    imsg.im_height = ntohs(imsg.im_height);

  if (imsg.im_found == NETIMG_FOUND) {
    img_dsize = (double) (imsg.im_height*imsg.im_width*(u_short)imsg.im_depth);
    img_size = (long) img_dsize;                 // global
  } 
 
  return imsg.im_found; 
}

/* Callback functions for GLUT */

/*
 * netimg_recvimage: called by GLUT when idle
 * On each call, receive as much of the image is available on the network and
 * store it in global variable "image" at offset "img_offset" from the
 * start of the buffer.  The global variable "img_offset" must be updated
 * to reflect the amount of data received so far.  Another global variable "img_size"
 * stores the expected size of the image transmitted from the server.
 * The variable "img_size" must NOT be modified.
 * Terminate process on receive error.
 */
void
netimg_recvimg(void)
{
   
  // img_offset is a global variable that keeps track of how many bytes
  // have been received and stored in the buffer.  Initialy it is 0.
  //
  // img_size is another global variable that stores the size of the image.
  // If all goes well, we should receive img_size bytes of data from the server.
  if (img_offset <  img_size) { 

    /* Task 1: YOUR CODE HERE
     * Receive as much of the remaining image as available from the network
     * put the data in the buffer pointed to by the global variable 
     * "image" starting at "img_offset".
     *
     * For example, the first time this function is called, img_offset is 0
     * so the received data is stored at the start (offset 0) of the "image" 
     * buffer.  The global variable "image" should not be modified.
     *
     * Update img_offset by the amount of data received, in preparation for the
     * next iteration, the next time this function is called.
     */
    int num_bytes_read = recv(sd, image + img_offset, img_size - img_offset, 0);
    
    // Fail due to network error
    if (num_bytes_read == -1) {
        fprintf(stderr, "Network error occurred in recvimg");
        exit(1);
    }
    
    img_offset += num_bytes_read;

    /* give the updated image to OpenGL for texturing */
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint) imsg.im_format,
                 (GLsizei) imsg.im_width, (GLsizei) imsg.im_height, 0,
                 (GLenum) imsg.im_format, GL_UNSIGNED_BYTE,
                 image);

    /* redisplay */
    glutPostRedisplay();
  }

  return;
}

int
main(int argc, char *argv[])
{
  unsigned char err;
  char *sname, *imagename;
  u_short port;
  char vers;

  // parse args, see the comments for netimg_args()
  if (netimg_args(argc, argv, &sname, &port, &imagename, &vers)) {
    netimg_usage(argv[0]);
  }

#ifndef _WIN32
  signal(SIGPIPE, SIG_IGN);    /* don't die if peer is dead */
#endif
  
  netimg_sockinit(sname, port);  // Task 1

  if (netimg_sendqry(imagename, vers)) {
    err = netimg_recvimsg();  // Task 1

    if (err == (unsigned char) NETIMG_FOUND) { // if image received ok
      netimg_glutinit(&argc, argv, netimg_recvimg);
      netimg_imginit();
      
      glutMainLoop(); /* start the GLUT main loop */
    } else if (err == (unsigned char) NETIMG_NFOUND) {
      fprintf(stderr, "%s: %s image not found.\n", argv[0], imagename);
    } else if (err == (unsigned char) NETIMG_EBUSY) {
      fprintf(
          stderr, 
          "Peer busy! Please wait for %s:%d to finish servicing its query.\n",
          sname,
          port
      );
    } else {
      fprintf(stderr, "%s: image receive error %d.\n", argv[0], err);
    }
  }
  
  return(0);
}
