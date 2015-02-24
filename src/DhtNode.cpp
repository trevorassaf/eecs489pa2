#include "DhtNode.h"

#include "dht_packets.h"

#include <stdio.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

void DhtNode::initFingers() {
  // Finger table should hold only FINGER_TABLE_SIZE fingers
  fingerTable_ = std::vector<finger_t>(FINGER_TABLE_SIZE + 1);
  for (size_t i = 0; i < FINGER_TABLE_SIZE; ++i) {
  uint16_t unfolded_finger_id = (1 << i) + id_; 
    
    // Build finger pointing to *self*
    finger_t finger;
    finger.finger_id = foldId(unfolded_finger_id);
    finger.node_id = id_;

    fingerTable_[i] = finger;
  }

  // Initialize predecessor to point to ourself
  finger_t& predecessor = getPredecessor();
  predecessor.node_id = id_;
  predecessor.remote
      .setRemotePort(dhtReceiver_->getPort())
      .setRemoteIpv4Address(dhtReceiver_->getIpv4());
}

void DhtNode::fixUp(size_t j) {
  // Fail b/c finger table is malformed
  assert(fingerTable_.size() == FINGER_TABLE_SIZE + 1);

  // Fail b/c j is out of bounds
  assert(j < fingerTable_.size());

  finger_t& finger_j = fingerTable_.at(j);
  size_t limit = fingerTable_.size() - 1; /* don't include predecessor */
  for (size_t k = j + 1; k < limit; ++k) {
    finger_t& finger = fingerTable_.at(k);
    if (ID_inrange(finger.finger_id, id_, finger_j.node_id)) {
      finger.node_id = finger_j.node_id;
      finger.remote.setRemotePort(finger_j.remote.getRemotePort());
      finger.remote.setRemoteIpv4Address(finger_j.remote.getRemoteIpv4Address());
    } else {
      break;
    }
  } 
}

void DhtNode::fixDown(size_t j) {
  // Fail b/c finger table is malformed
  assert(fingerTable_.size() == FINGER_TABLE_SIZE + 1);

  // Fail b/c j is out of bounds
  assert(j < fingerTable_.size() && j > 0);

  finger_t& finger_j = fingerTable_.at(j);
  for (int k = j - 1; k >= 0; --k) {
    finger_t& finger = fingerTable_.at(k);
    if (finger.finger_id == finger.node_id) {
      break;
    } else if (ID_inrange(finger_j.node_id, finger.finger_id, finger.node_id)) { 
      finger.node_id = finger_j.node_id;
      finger.remote.setRemotePort(finger_j.remote.getRemotePort());
      finger.remote.setRemoteIpv4Address(finger_j.remote.getRemoteIpv4Address());
    }
  }
}

void DhtNode::sendJoinRequest() {
  // Fail b/c target was not specified
  assert(hasTarget_);
  
  // Assemble header packet 
  dhtheader_t header = {DHTM_VERS, DHTM_JOIN};
  
  // Assemble 'self' packet 
  dhtnode_t self;
  self.id = id_; // one byte, no host -> network converstion needed!
  self.port = htons(dhtReceiver_->getPort());
  self.ipv4 = htonl(dhtReceiver_->getIpv4());

  // Assemble join packet
  dhtmsg_t join_pkt = {header, htons(DHTM_TTL), self};
  const std::string message((const char *) &join_pkt, sizeof(join_pkt));

  std::cout << "Sending JOIN packet to " << targetFqdn_ << ":" << (int) targetPort_ <<
      " <ttl: " << (int) ntohs(join_pkt.ttl) << 
      ", id: " << (int) join_pkt.node.id << 
      ", port: " << (int) ntohs(join_pkt.node.port) << 
      ", ipv4: " << stringifyIpv4(join_pkt.node.ipv4) << ">" << std::endl;

  // Connect to DHT network through specified target
  ServerBuilder builder; 
  const Connection remote = builder
    .setRemoteDomainName(targetFqdn_)
    .setRemotePort(targetPort_)
    .build();

  // Send join message to first network node and close connection
  try {
    remote.writeAll(message);
    remote.close();
  } catch (const SocketException& e) {
    std::cout << "Failed while writing data to first target!" << std::endl;
    exit(1);
  }
}

DhtNode::finger_t& DhtNode::getPredecessor() {
  return fingerTable_.back();
}

const DhtNode::finger_t& DhtNode::getPredecessor() const {
  return fingerTable_.back();
}

size_t DhtNode::findFingerForForwarding(uint8_t object_id) const {
  size_t limit = fingerTable_.size() - 1;
  size_t idx = 0;
  
  while (idx < limit) {
    const finger_t& finger = fingerTable_.at(idx);
    
    if (ID_inrange(object_id, id_, finger.finger_id)) {
      return (finger.node_id == object_id)
          ? idx
          : idx - 1;
    }

    ++idx;
  }

  return limit - 1; /* last finger (not predecessor) */
}

bool DhtNode::expectToFindObject(uint8_t object_id, const finger_t& finger) const {
  return ID_inrange(object_id, finger.finger_id, finger.node_id) 
      || object_id == finger.finger_id; 
}

const std::string DhtNode::stringifyIpv4(uint32_t ipv4) const {
  char ipv4_cstr[INET_ADDRSTRLEN];
  memset(ipv4_cstr, 0, INET_ADDRSTRLEN);
  struct in_addr addr = {ipv4};
  inet_ntop(AF_INET, (void *) &addr, ipv4_cstr, INET_ADDRSTRLEN);

  return std::string(ipv4_cstr);
}

// TODO remove this!
void DhtNode::printFingers() const {
  std::cout << "--- FINGER TABLE DUMP ---" << std::endl;

  for (size_t i = 0; i < FINGER_TABLE_SIZE; ++i) {
    const finger_t finger = fingerTable_[i];
    std::cout << "finger<idx: " << i << ", fID: " << (int) finger.finger_id
        << ", finger: " << (int) finger.node_id << std::endl;
  }

  std::cout << "predecessor<node-id: " << (int) getPredecessor().node_id << ">"
      << "\nus<node-id: " << (int) id_ << ">" << std::endl;
}

void DhtNode::initDhtReceiver() {
  // Initialize listening socket
  ServiceBuilder builder;
  dhtReceiver_ = builder.buildNew();

  // Report address of this DhtNode
  std::cout << "DhtNode address: " << dhtReceiver_->getDomainName()
      << ":" << dhtReceiver_->getPort() << std::endl;
}

void DhtNode::initImageReceiver() {
  // Initialize listening socket
  ServiceBuilder builder;
  imageReceiver_ = builder.buildNew();
  
  // Report address of this image socket 
  std::cout << "Image receiver address: " << imageReceiver_->getDomainName()
      << ":" << (int) imageReceiver_->getPort() << std::endl;
}

void DhtNode::deriveId() {
  // Prepare SHA1 hash inpute: port+address
  uint16_t port = dhtReceiver_->getPort();
  size_t port_num_bytes = sizeof(port);
  
  uint32_t ipv4 = dhtReceiver_->getIpv4();
  size_t ipv4_num_bytes = sizeof(ipv4);
  
  size_t addrport_num_bytes = ipv4_num_bytes + port_num_bytes; 
  char* addrport = new char[addrport_num_bytes + 1];
  addrport[addrport_num_bytes] = '\0';

  memcpy(addrport, (const char *) &port, port_num_bytes);
  memcpy(addrport + port_num_bytes, (const char *) &ipv4, ipv4_num_bytes);

  // Compute SHA1 hash
  unsigned char md[SHA1_MDLEN + 1];
  md[SHA1_MDLEN] = '\0';
  SHA1((unsigned char *) addrport, SIZE_OF_ADDR_PORT, md);
  delete[] addrport;
  
  // Fold SHA1 hash into node id 
  id_ = ID(md);
}

uint8_t DhtNode::foldId(uint16_t unfolded_id) const {
  return unfolded_id % NUM_IDS; 
}

void DhtNode::handleDhtTraffic() {
  // Accept connection from requesting remote
  const Connection connection = dhtReceiver_->accept();

  // Read dht message and close connection
  dhtmsg_t message;
  size_t message_size = sizeof(message);
  
  connection.readAll( (void *) &message, message_size);

  // Fail b/c we received an invalid version
  if (message.header.vers != DHTM_VERS) {
    std::cout << "Invalid type received! Expected: " << DHTM_VERS <<
        ", but received: " << (int) message.header.vers << std::endl;
    exit(1);
  }

  // Report request
  reportDhtMsgReceived(message);

  // Handle ops that require open connections
  uint8_t type = message.header.type;
  switch (type) {
    case JOIN:
    case JOIN_ATLOC:
      handleJoinAndCloseCxn(message, connection);
      return;
    case WLCM:
      handleWlcmAndCloseCxn(message, connection);
      return;
    case SRCH:
    case SRCH_ATLOC:
      handleSrchAndCloseCxn(message, connection);
      return;
    case RPLY:
      handleRplyAndCloseCxn(message, connection);
      return;
    case MISS:
      handleMissAndCloseCxn(message, connection);
      return;
  }
 
  // Subsequent ops don't neet the connection, so close it.
  connection.close();
  
  // Handle ops that permit closed connections
  switch (message.header.type) {
    case REID:
      handleReid();
      break;
    default:
      std::cout << "Invalid header type received: " << (int) message.header.type
          << std::endl;
      exit(1);
  }
}

bool DhtNode::handleCliInput() {
  // Read string from stdin 
  std::string cli_input;
  std::cin >> cli_input;
  
  if (cli_input.size() != 1) {
    reportCliInstructions();
  } else {
    // Process single input char
    switch (cli_input[0]) {
      case EOF:
      case 'q':
      case 'Q':
        std::cout << "Bye!" << std::endl;
        return false;
      case 'p':
        reportAdjacentNodes();
        break;
      case 'f':
        printFingers(); 
        break;
      default:
        reportCliInstructions();
        break;
    }
  }

  fflush(stdin);
  return true;
}

void DhtNode::handleImageTraffic() {

  // Accept connection from netimg client
  const Connection * cxn = imageReceiver_->acceptNew();
  
  // Report that we're processing image traffic
  std::cout << "Netimg request received! Procesing image traffic from <" << 
      cxn->getRemoteIpv4() << ":" << cxn->getRemotePort() << ">" << std::endl;

  // Read message from netimg client
  iqry_t message;
  cxn->readAll( (void *) &message, sizeof(message));

  // Fail due to incorrect netimg packet version
  assert(message.header.vers == NETIMG_VERS);

  // Reject the netimg query if we're busy
  if (servicingImageQuery_) {
    rejectNetimgQuery(cxn);
    return;
  } 

  // Block other requests until this one is finished
  servicingImageQuery_ = true;

  // Query local db for the requested image
  const std::string file_name(message.name);
  QueryResult result = imageDb_->query(file_name);

  switch (result) {
    //// IMAGE IS LOCAL -> FORWARD TO CLIENT ////
    case QUERY_SUCCESS:
      // Report image found locally
      std::cout << "\t- Image found locally!" << std::endl;
      handleLocalQuerySuccess(file_name, cxn);
      return;

    //// IMAGE IS NOT LOCAL -> QUERY DHT  -> FORWARD TO CLIENT ////
    case BLOOM_FILTER_MISS:
      // Report bloom filter miss
      std::cout << "\t- Image NOT found locally -- bloom filter false positive!" << std::endl;
      break;

    case QUERY_FAILURE:
      std::cout << "\t- Image NOT found locally -- not in bloom filter" << std::endl;
      break;

    default:
      std::cout << "Invalid QueryResult type" << std::endl;
      exit(1);
  }
  
  // Cache connection so that we can send back to the netimg client
  // when we find the image
  imageClient_ = cxn;

  // Forward packet to dht
  forwardInitialImageQuery(file_name);   
}

void DhtNode::forwardInitialImageQuery(const std::string& file_name) {
 
  // Fail b/c we should be in the 'servicing query' state
  assert(servicingImageQuery_);

  // Fail b/c we should have a valid image-client
  assert(imageClient_);

  // Report that we're forwarding the image query over the dht
  std::cout << "\t- Forwarding image query to the DHT!" << std::endl;

  // Assemble dhtsrch_t packet
  dhtsrch_t srch_pkt;
  srch_pkt.msg.header = {DHTM_VERS, DHTM_QRY};

  // Provide our dht receiver as the node to connect to when
  // the image is found. We will listen for a response on this socket
  // and stream down the result once the DHT provides one.
  dhtnode_t self;
  self.id = id_;
  self.port = htons(dhtReceiver_->getPort());
  self.ipv4 = htons(dhtReceiver_->getIpv4());
  srch_pkt.msg.node = self;

  // Add image query details to packet
  unsigned char md[SHA1_MDLEN];
  SHA1((unsigned char *) file_name.c_str(), file_name.size(), md);

  srch_pkt.img.id = static_cast<uint8_t>(ID(md)); 
  memset(srch_pkt.img.name, 0, DHT_MAX_FILE_NAME);
  memcpy(srch_pkt.img.name, file_name.c_str(), file_name.size());

  // Forward search packet to network
  forwardImageQueryWithoutTtl(srch_pkt);
}

void DhtNode::forwardImageQuery(const dhtsrch_t& srch_pkt) {
  // Kill search request if ttl has expired
  if (srch_pkt.msg.ttl == 1) {
    // Report that we've dropped the search request
    std::cout << "- Dropped search request b/c ttl is now 0!" << std::endl;
  } else {
    forwardImageQueryWithoutTtl(srch_pkt);
  }
}

void DhtNode::forwardImageQueryWithoutTtl(dhtsrch_t srch_pkt) {
  
  // Select finger to forward the search to
  size_t finger_idx = findFingerForForwarding(srch_pkt.img.id);
  
  // Fail b/c finger idx is out of bounds
  assert(finger_idx < fingerTable_.size() - 1);

  const finger_t& target_finger = fingerTable_.at(finger_idx);   

  // Fail b/c we shouldn't be forwarding to ourselves
  assert(target_finger.node_id != id_);

  // Fail b/c a non-search packet made it into this function
  assert(srch_pkt.msg.header.type == SRCH 
      || srch_pkt.msg.header.type == SRCH_ATLOC); 
  
  // Set ATLOC bit, if we expect to find the object at the finger that 
  // we're forwarding the image query to
  srch_pkt.msg.header.type = (expectToFindObject(srch_pkt.img.id, target_finger))
      ? SRCH_ATLOC
      : SRCH;
  
  // Serialize image query packet
  std::string message((const char *) &srch_pkt, sizeof(srch_pkt));

  // Report that we're forwarding the search request
  std::cout << "\t- Forwarding SRCH to finger[" << finger_idx << "]:" <<
      "<id: " << (int) target_finger.node_id <<
      ", port: " << (int) target_finger.remote.getRemotePort() <<
      ", ipv4: " << stringifyIpv4(htonl(target_finger.remote.getRemoteIpv4Address())) << 
      ", type: " << getDhtTypeString(static_cast<DhtType>(srch_pkt.msg.header.type)) << 
      ">." << std::endl;

  try {
    // Forward packet to target finger
    Connection remote = target_finger.remote.build();
    remote.writeAll(message);

    if (srch_pkt.msg.header.type == JOIN_ATLOC) {
      // Wait for REDRT packet or for a closed connection. Remote will send REDRT
      // if it doesn't have the purview that we expected it to have. Conversely,
      // the remote will close the connection if it does accept the join request.
      dhtmsg_t redrt_pkt;  
      size_t redrt_pkt_size = sizeof(redrt_pkt);

      try {
        // Try to read REDRT packet
        remote.readAll( (void *) &redrt_pkt, redrt_pkt_size);
        remote.close();

        // Handle REDRT pkt
        handleSrchRedrt(redrt_pkt, srch_pkt, finger_idx);
        return;

      } catch (const PrematurelyClosedSocketException& e) {
        // Report that no REDRT packet was received
        std::cout << "\t- Remote closed connection. No REDRT packet received." << std::endl;
      }
    } 
   
    // Finally, close the connection (should never 
    // reach here if a redrt packet was received)
    remote.close();

  } catch (const SocketException& e) {
    std::cout << "Failed while trying to forward search packet!" << std::endl;
    exit(1);
  }
}

void DhtNode::handleLocalQuerySuccess(
  const std::string& file_name,
  const Connection* cxn
) {
 
  // Report that we're sending the image down to the client
  std::cout << "\t- Streaming image down to client!" << std::endl;

  // Load image into memory
  LTGA ltga(file_name);

  // Assemble imsg_t packet, then serialize
  imsg_t message;
  message.header = {NETIMG_VERS, FOUND};
  size_t image_size = (size_t) loadImsgPacket(ltga, message);
  
  std::string message_str( (char *) &message, sizeof(message));
 
  // Send image meta data to netimg client
  cxn->writeAll(message_str);

  // Assemble image pixel payload and send to client
  std::string image_str( (char *) ltga.GetPixels(), image_size);

  cxn->writeAll(image_str);
  cxn->close();
  delete cxn;

  // Make this node available to service other image queries
  servicingImageQuery_ = false;

  // Report that we can service other netimg queries again
  std::cout << "\t- Finished servicing query, we can now service additional queries!"
      << std::endl;
}

size_t DhtNode::loadImsgPacket(LTGA& curimg, imsg_t& imsg) const {
  
  int alpha, greyscale;
  
  imsg.im_depth = (unsigned char)(curimg.GetPixelDepth()/8);
  imsg.im_width = htons(curimg.GetImageWidth());
  imsg.im_height = htons(curimg.GetImageHeight());
  alpha = curimg.GetAlphaDepth();
  greyscale = curimg.GetImageType();
  greyscale = (greyscale == 3 || greyscale == 11);
  if (greyscale) {
    imsg.im_format = alpha ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
  } else {
    imsg.im_format = alpha ? GL_RGBA : GL_RGB;
  }

  imsg.im_format = htons(imsg.im_format);

  return (size_t) ((double) (curimg.GetImageWidth() *
     curimg.GetImageHeight() *
     (curimg.GetPixelDepth()/8)));
}

void DhtNode::rejectNetimgQuery(const Connection* cxn) const {
 
  // Report that we're rejecting the netimg query because we're
  // already servicing another netimg request.
  std::cout << "\t- Rejecting netimg query because we're already handling another query" << std::endl;

  // Assemble imsg_t packet
  imsg_t message;
  message.header = {NETIMG_VERS, NETIMG_RPY};
  message.im_found = BUSY;
  
  std::string message_str( (char *) &message, sizeof(message));

  // Send rejection to netimg client
  try {
    cxn->writeAll(message_str);
    cxn->close();
  } catch (const SocketException& e) {
    std::cout << "Failed while sending rejection packet to netimg client." << std::endl;
    exit(1);
  }
}

void DhtNode::reportCliInstructions() const {
  std::cout << "CLI instructions: \n\t- ['Q' | 'q' | EOF] -> quit\n"
      << "\t- ['p'] -> print predecessor/successor ID's" << std::endl;
}

void DhtNode::reportAdjacentNodes() const {
  const finger_t& predecessor_finger = getPredecessor();
  std::cout << "--- Adjacent Node Info ---\n\t- predecessor ID: "
      << (int) predecessor_finger.node_id;
  
  if (predecessor_finger.node_id == id_) {
    std::cout << " (self)";
  }

  uint8_t successor_id = fingerTable_.front().node_id;
  std::cout << "\n\t- successor ID: " << (int) successor_id;
  
  if (successor_id == id_) {
    std::cout << " (self)";
  }

  std::cout << "\n--------------------" << std::endl;
}

void DhtNode::handleJoinAndCloseCxn(
  const dhtmsg_t& join_msg,
  const Connection& cxn
) {
  // Fail b/c msg is not a join message
  assert(join_msg.header.type == JOIN || join_msg.header.type == JOIN_ATLOC);

  // Report join request
  std::cout << "\t- Remote is attempting to join with id: " << (int) join_msg.node.id << std::endl;
  
  // Reject join request, if node's id collides
  if (doesJoinCollide(join_msg)) {
    // Close connection to sender, we're going to reconnect
    // to join initiator
    cxn.close();

    // Report that incomming node collides 
    std::cout << "\t- Join request declined! Requested id collides with us! Id(self) : " << (int) id_ <<
        ", Id(predecessor) : " << (int) getPredecessor().node_id << std::endl;

    // Notify requesting node of id collision
    handleJoinCollision(join_msg);

  } else if (inOurPurview(join_msg)) {
    // Close connection to sender, we're going to reconnect
    // to join initiator and send along a WLCM message
    cxn.close();
   
    // Report that we've accepted the join 
    std::cout << "\t- We've accepted the remote's join request!" << std::endl;

    // Accept join request, make requester new predecessor
    const finger_t& predecessor_finger = getPredecessor();
    uint8_t old_predecessor_id = predecessor_finger.node_id;
    handleJoinAcceptance(join_msg);
    
    // Report successful join
    std::cout << "\t- Now replacing former predecessor with joining node..." << 
        "\n\t\t- Id(old predecessor) : " << (int) old_predecessor_id <<
        "\n\t\t- Id(new predecessor) : " << (int) predecessor_finger.node_id << std::endl;

  } else {
    // Report failed JOIN attempt
    std::cout << "\t- Request failed! Couldn't find " << (int) join_msg.node.id
        << " in identifier space (" << (int) getPredecessor().node_id << ", " << (int) id_
        << "]" << std::endl;

    if (senderExpectedJoin(join_msg)) {
      // Report that sender expected the join to succeed but it didn't
      std::cout << "\t- But sender " << cxn.getRemoteDomainName() << ":"
          << (int) cxn.getRemotePort() << " DID expect to join with us..."
          << std::endl;

      // Inform sender that the join failed, even though the sender
      // expected the join to succeed
      handleUnexpectedJoinFailureAndClose(join_msg, cxn);

    } else {
      // Close connection to sender b/c we're going to forward the join
      // along the finger table
      cxn.close();

      // Report that sender did not expect to join with us
      std::cout << "\t- Sender DID NOT expect to join with us."
        << std::endl;

      // Kill join request if ttl has run out
      if (join_msg.ttl == 1) {
        // Report that we've dropped the join request
        std::cout << "- Dropped join request b/c ttl is now 0!" << std::endl;
      } else {
        // Forward join request to next best node
        forwardJoin(join_msg);
      }
    }
  }
}

void DhtNode::handleJoinCollision(const dhtmsg_t& join_msg) {
  // Assemble reid payload
  dhtmsg_t reid_msg;
  reid_msg.header = {DHTM_VERS, REID};
 
  // Assemble 'self'
  dhtnode_t self;
  self.id = id_;
  self.port = htons(dhtReceiver_->getPort());
  self.ipv4 = htonl(dhtReceiver_->getIpv4());
  reid_msg.node = self;
 
  // Serialize packet
  std::string reid_msg_str((char *) &reid_msg, sizeof(reid_msg));

  // Send reid messsage to node that initiated the join
  ServerBuilder builder;
  Connection connection = builder
    .setRemotePort(ntohs(join_msg.node.port))
    .setRemoteIpv4Address(ntohl(join_msg.node.ipv4))
    .build();

  connection.writeAll(reid_msg_str);
  connection.close();
  
  // Report sending REID
  std::cout << "\t- Sending REID packet to " << connection.getRemoteDomainName() << ":"
    << connection.getRemotePort() << std::endl;
}

void DhtNode::handleJoinAcceptance(const dhtmsg_t& join_msg) {
  // Assemble 'wlcm' packet
  dhtwlcm_t wlcm;
  wlcm.msg.header = {DHTM_VERS, WLCM};
  
  // Make 'self' new successor of joining node
  dhtnode_t self;
  self.id = id_;
  self.port = htons(dhtReceiver_->getPort());
  self.ipv4 = htonl(dhtReceiver_->getIpv4());
  wlcm.msg.node = self;

  // Make our current predecessor the predecessor of the joining node
  dhtnode_t pred;
  finger_t& predecessor_finger = getPredecessor();
  pred.id = predecessor_finger.node_id;
  pred.port = htons(predecessor_finger.remote.getRemotePort());
  pred.ipv4 = htonl(predecessor_finger.remote.getRemoteIpv4Address());
  wlcm.predecessor = pred;

  std::string wlcm_str((char *) &wlcm, sizeof(wlcm));

  // Send wlcm messsage to node that initiated the join
  ServerBuilder builder;
  Connection connection = builder
      .setRemotePort(ntohs(join_msg.node.port))
      .setRemoteIpv4Address(ntohl(join_msg.node.ipv4))
      .build();

  connection.writeAll(wlcm_str);
  connection.close();

  // Report sending WLCM message
  std::cout << "\t- Sending WLCM packet to " << connection.getRemoteDomainName() << ":"
      << connection.getRemotePort() << std::endl;

  // Make joining node our new predecessor
  predecessor_finger.node_id = join_msg.node.id;
  predecessor_finger.remote
      .setRemotePort(ntohs(join_msg.node.port))
      .setRemoteIpv4Address(ntohl(join_msg.node.ipv4));

  // Fix table to reflect new predecessor
  fixDown(fingerTable_.size() - 1);

  // Make joining node our successor too, if we were previously the 
  // only node in the network
  if (id_ == fingerTable_.front().node_id) {
    finger_t& successor = fingerTable_.front();
    uint8_t old_successor_id = successor.node_id; 

    successor.node_id = predecessor_finger.node_id;
    successor.remote
        .setRemotePort(predecessor_finger.remote.getRemotePort())
        .setRemoteIpv4Address(predecessor_finger.remote.getRemoteIpv4Address());

    // Report that we've updated the successor
    std::cout << "\t- Now replacing former successor with joining node..." <<
        "\n\t\t- Id(old successor) : " << (int) old_successor_id <<
        "\n\t\t- Id(new successor) : " << (int) successor.node_id << std::endl;

    // Fix table to reflect new successor
    fixUp(0);
  } 

  // Reload db because our identifer space has now changed
  reloadDb();
}

void DhtNode::handleUnexpectedJoinFailureAndClose(
  const dhtmsg_t& join_msg,
  const Connection& cxn
) {
  // Assemble REDRT packet
  dhtmsg_t redrt_msg;
  redrt_msg.header = {DHTM_VERS, REDRT};

  // Send back our current predecessor to the redirected node.
  // The redirected node will make our predecessor its new successor
  // and then try to forward the join across its finger table.
  const finger_t& predecessor_finger = getPredecessor();
  
  dhtnode_t pred;
  pred.id = predecessor_finger.node_id;
  pred.port = htons(predecessor_finger.remote.getRemotePort());
  pred.ipv4 = htonl(predecessor_finger.remote.getRemoteIpv4Address());
  redrt_msg.node = pred;
  
  // Send packet to sender (not neccessarily node that initiated join request)
  std::string redrt_str((char *) &redrt_msg, sizeof(redrt_msg));

  cxn.writeAll(redrt_str);
  cxn.close();

  // Report unexpected join failure
  std::cout << "\t- Sending REDRT packet to " << cxn.getRemoteDomainName() << ":"
      << (int) cxn.getRemotePort() << " with our predecessor: <id: " << 
      (int) predecessor_finger.node_id << ", port: " << 
      (int) predecessor_finger.remote.getRemotePort() << ", ipv4: " <<
      stringifyIpv4(predecessor_finger.remote.getRemoteIpv4Address()) << ">" << std::endl;
}

void DhtNode::forwardJoin(dhtmsg_t join_msg) {

  // Select finger to forward the join to
  size_t finger_idx = findFingerForForwarding(join_msg.node.id);
  
  // Fail b/c finger idx is out of bounds
  assert(finger_idx < fingerTable_.size() - 1);

  const finger_t& target_finger = fingerTable_.at(finger_idx);   
  
  // Fail b/c this is not a JOIN packet
  assert(join_msg.header.type == JOIN || join_msg.header.type == JOIN_ATLOC);
  
  // Set ATLOC bit, if we expect to find the object at the finger 
  join_msg.header.type = (expectToFindObject(join_msg.node.id, target_finger))
      ? JOIN_ATLOC
      : JOIN;

  // Report that we're forwarding the join request and that we don't necessarily
  // expect the target finger to accept the join
  std::cout << "\t- Forwarding JOIN to finger[" << finger_idx << "]:" <<
      "<id: " << (int) target_finger.node_id <<
      ", port: " << (int) target_finger.remote.getRemotePort() <<
      ", ipv4: " << stringifyIpv4(htonl(target_finger.remote.getRemoteIpv4Address())) << 
      ", type: " << getDhtTypeString(static_cast<DhtType>(join_msg.header.type)) << 
      ">." << std::endl;

  // Fail b/c we shouldn't be forwarding to ourselves
  assert(target_finger.node_id != id_);
  
  // Open connection to target finger and send message
  std::string message((const char *) &join_msg, sizeof(join_msg));

  Connection remote = target_finger.remote.build();

  try {
    remote.writeAll(message);

    if (join_msg.header.type == JOIN_ATLOC) {
      // Wait for REDRT packet or for a closed connection. Remote will send REDRT
      // if it doesn't have the purview that we expected it to have. Conversely,
      // the remote will close the connection if it does accept the join request.
      try {
        dhtmsg_t redrt_pkt;  
        size_t redrt_pkt_size = sizeof(redrt_pkt);

        // Try to read REDRT packet
        remote.readAll( (void *) &redrt_pkt, redrt_pkt_size);
        remote.close();

        // Handle REDRT pkt
        handleJoinRedrt(redrt_pkt, join_msg, finger_idx);
        return;

      } catch (const PrematurelyClosedSocketException& e) {
        // Report that no REDRT packet was received
        std::cout << "\t- Remote closed connection. No REDRT packet received." << std::endl;
      }
    } 
   
    // Finally, close the connection (should never 
    // reach here if a redrt packet was received)
    remote.close();

  } catch (const SocketException& e) {
    std::cout << "Failed while trying to forward join packet!" << std::endl;
    exit(1);
  }
}

void DhtNode::handleRedrt(const dhtmsg_t& redrt_pkt, size_t finger_idx) {
 
  // Fail b/c 'finger_idx' is out of bounds
  assert(finger_idx < fingerTable_.size() - 1);

  // Fail b/c header of redrt-packet is not REDRT
  assert(redrt_pkt.header.type == REDRT);
  
  // Report REDRT packet received
  std::cout << "\t- Received REDRT packet providing new finger[" << finger_idx << ": " <<
      "<id: " << (int) redrt_pkt.node.id << 
      ", port: " << (int) ntohs(redrt_pkt.node.port) << 
      ", ipv4: " << stringifyIpv4(redrt_pkt.node.ipv4) << ">" << std::endl;

  finger_t& finger = fingerTable_.at(finger_idx);

  // Report old finger we're replacing
  std::cout << "\t- Reconfiguring finger[" << finger_idx << "]\n\t\t- From " <<
      "<id: " << (int) finger.node_id << 
      ", port: " << (int) finger.remote.getRemotePort() << 
      ", ipv4: " << stringifyIpv4(htonl(finger.remote.getRemoteIpv4Address())) << 
      ">" << std::endl;

  // Reassign finger to provided node 
  finger.node_id = redrt_pkt.node.id;
  finger.remote
    .setRemotePort(ntohs(redrt_pkt.node.port))
    .setRemoteIpv4Address(ntohl(redrt_pkt.node.ipv4));

  // Report new finger that we're replacing the old one with 
  std::cout << "\t\t- To " <<
      "<id: " << (int) finger.node_id << 
      ", port: " << (int) finger.remote.getRemotePort() << 
      ", ipv4: " << stringifyIpv4(htonl(finger.remote.getRemoteIpv4Address())) << 
      ">" << std::endl;

  // Fix finger table
  fixUp(finger_idx);

  if (finger_idx != 0) {
    fixDown(finger_idx);
  }
}

void DhtNode::reloadDb() {
  const finger_t& predecessor = getPredecessor();
  imageDb_->load(predecessor.node_id, id_);
}

void DhtNode::handleSrchRedrt(
  const dhtmsg_t& redrt_pkt,
  const dhtsrch_t& srch_pkt,
  size_t finger_idx
) {
  // Fail b/c header of join-packet is not SRCH_ATLOC
  assert(srch_pkt.msg.header.type == SRCH_ATLOC);

  // Update finger table 
  handleRedrt(redrt_pkt, finger_idx);

  // Forward search packet to the DHT
  forwardImageQueryWithoutTtl(srch_pkt);
}

void DhtNode::handleJoinRedrt(
  const dhtmsg_t& redrt_pkt,
  const dhtmsg_t& join_pkt,
  size_t finger_idx
) {
  // Fail b/c header of join-packet is not JOIN_ATLOC
  assert(join_pkt.header.type == JOIN_ATLOC);

  // Update finger table 
  handleRedrt(redrt_pkt, finger_idx);
  
  // Forward join back to network
  forwardJoin(join_pkt);
}

void DhtNode::handleReid() {
  // Report that we're generating a new id
  std::cout << "\t- Restarting dht socket and generating new id..." << std::endl;

  // Close dht receiver socket and create a new one
  dhtReceiver_->close();
  delete dhtReceiver_;
  initDhtReceiver();

  // Derive a new id from the new address+port of the new 
  // receiving socket and recompute fingers
  deriveId();
  initFingers();
  reportId();

  // Reload image db
  imageDb_->load(id_, id_);
 
  // Retry join
  sendJoinRequest();
}

void DhtNode::handleSrchAndCloseCxn(
  const dhtmsg_t& msg,
  const Connection& connection
) {}

void DhtNode::handleRplyAndCloseCxn(
  const dhtmsg_t& msg,
  const Connection& connection
) {}

void DhtNode::handleMissAndCloseCxn(
  const dhtmsg_t& msg,
  const Connection& connection
) {}


void DhtNode::handleWlcmAndCloseCxn(
  const dhtmsg_t& msg,
  const Connection& connection
) {
 
  // Read predecessor dhtnode_t from wire
  dhtnode_t pred;
  size_t pred_size = sizeof(pred);

  connection.readAll((void *) &pred, pred_size);
  connection.close();

  const dhtnode_t& succ = msg.node;

  // Report WLCM message w/successor/predecessor data
  std::cout << "\t- We've been welcomed into the DHT! Here are our new predecessor/successor nodes:" <<
      "\n\t\t- Predecessor: <id: " << (int) pred.id << ", port: " << 
      (int) ntohs(pred.port) << ", ipv4: " << stringifyIpv4(pred.ipv4) << 
      ">\n\t\t- Successor: <id: " << (int) succ.id << ", port: " << 
      (int) ntohs(succ.port) << ", ipv4: " << stringifyIpv4(succ.ipv4) << ">" << std::endl;

  // Assimilate provided predecessor/successor
  finger_t& predecessor_finger = getPredecessor();
  predecessor_finger.node_id = pred.id;
  predecessor_finger.remote
      .setRemotePort(ntohs(pred.port))
      .setRemoteIpv4Address(ntohl(pred.ipv4));

  finger_t& successor_finger = fingerTable_.front();
  successor_finger.node_id = succ.id;
  successor_finger.remote
      .setRemotePort(ntohs(succ.port))
      .setRemoteIpv4Address(ntohl(succ.ipv4));

  // Reload db b/c our predecessor changed
  reloadDb();

  // Fix finger table
  fixUp(0);
  fixDown(fingerTable_.size() - 1);
}

void DhtNode::handleSrch(const dhtmsg_t& msg, const Connection& connection) {
  std::cout << "HANDLE srch DIE" << std::endl; 
  exit(1);

}

bool DhtNode::doesJoinCollide(const dhtmsg_t& join_msg) const {
  return getPredecessor().node_id == join_msg.node.id || id_ == join_msg.node.id;  
}

bool DhtNode::inOurPurview(const dhtmsg_t& join_msg) const {
  return ID_inrange(join_msg.node.id, getPredecessor().node_id, id_);
}

bool DhtNode::inSuccessorsPurview(uint8_t object_id) const {
  const finger_t& successor_finger = fingerTable_.front();
  return ID_inrange(object_id, id_, successor_finger.node_id);
}

bool DhtNode::senderExpectedJoin(const dhtmsg_t& join_msg) const {
  return JOIN_ATLOC == join_msg.header.type;
}

void DhtNode::reportDhtMsgReceived(const dhtmsg_t& dhtmsg) const {
  std::cout << "Received " << getDhtTypeString(static_cast<DhtType>(dhtmsg.header.type))
      << " message!" << std::endl;
}

std::string DhtNode::getDhtTypeString(DhtType type) const {
  switch (type) {
    case JOIN:
      return JOIN_STR;
    case JOIN_ATLOC:
      return JOIN_ATLOC_STR;
    case REDRT:
      return REDRT_STR;
    case REID:
      return REID_STR;
    case WLCM:
      return WLCM_STR;
    case SRCH:
      return SRCH_STR;
    case SRCH_ATLOC:
      return SRCH_ATLOC_STR;
    default:
      std::cout << "Invalid NodeType: " << type << std::endl;
      exit(1);
  }
}

void DhtNode::reportId() const {
  std::cout << "DhtNode ID: " << (int) id_ << std::endl;
}

DhtNode::DhtNode(uint8_t id) : 
  imageDb_(nullptr),
  imageClient_(nullptr),
  servicingImageQuery_(false),
  id_(id),
  hasTarget_(false) 
{
  initImageReceiver();
  initDhtReceiver();
  initFingers();
  reportId();
  imageDb_ = new ImageDb(id_);
}

DhtNode::DhtNode() : 
  imageDb_(nullptr),
  imageClient_(nullptr),
  servicingImageQuery_(false),
  hasTarget_(false)
{
  initImageReceiver();
  initDhtReceiver();
  deriveId();
  initFingers();
  reportId();
  imageDb_ = new ImageDb(id_);
}

void DhtNode::joinNetwork(const std::string& fqdn, uint16_t port) {
  // Fail b/c target should not have been specified before this
  assert(!hasTarget_);

  // Set target node
  hasTarget_ = true;
  targetFqdn_ = fqdn;
  targetPort_ = port;

  // Attempt to join target
  sendJoinRequest();
}

void DhtNode::run() {
  Selector selector;
  
  // Listen on stdin for keyboard input
  selector.bind(
      STDIN_FILENO,
      [&] (int sd) -> bool {
        return handleCliInput(); 
      }
  );
  
  // Listen on 'image receiver' socket for image trafic 
  selector.bind(
      imageReceiver_->getFd(),
      [&] (int sd) -> bool {
        handleImageTraffic(); 
        return true;
      }
  );

  bool should_continue = true;

  do {
    int receiver_fd = dhtReceiver_->getFd();

    // Listen on 'dht receiver' socket for dht traffic
    selector.bind(
        dhtReceiver_->getFd(),
        [&] (int sd) -> bool {
          handleDhtTraffic();               
          return true;
        }
    );

    should_continue = selector.listen();
 
    // Unset callback b/c the receiver's socket 'fd' might have changed
    selector.erase(receiver_fd);

  } while (should_continue);
}

void DhtNode::close() {
  try {
    dhtReceiver_->close();
    imageReceiver_->close();
  } catch (const SocketException& e) {
    std::cout << "Failed to close DhtNode!" << std::endl;
    exit(1);
  }

  delete dhtReceiver_;
  delete imageReceiver_;
}
