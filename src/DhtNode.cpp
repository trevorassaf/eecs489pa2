#include "DhtNode.h"

#include "dht_packets.h"

#include <stdio.h>

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
  predecessor.remote.setRemotePort(receiver_->getPort());
  predecessor.remote.setRemoteIpv4Address(receiver_->getIpv4());
  predecessor.node_id = id_;
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
  self.port = htons(receiver_->getPort());
  self.ipv4 = htonl(receiver_->getIpv4());

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
    const finger_t& tail = fingerTable_.at(idx);
    const finger_t& head = fingerTable_.at(++idx);
    
    if (ID_inrange(object_id, head.node_id, tail.node_id)) {
      break;
    }
  }

  return idx;
}

void DhtNode::forwardJoinToSuccessor(dhtmsg_t join_pkt) {
  // Reassign packet type to JOIN_ATLOC if it was previously JOIN
  if (join_pkt.header.type == JOIN) {
    join_pkt.header.type = JOIN_ATLOC;
  }

  // Fail b/c all packets should have 'type' set to JOIN_ATLOC
  // at this point.
  assert(join_pkt.header.type == JOIN_ATLOC);

  // Open connection to target finger and send join request
  std::string message((const char *) &join_pkt, sizeof(join_pkt));
  const finger_t& successor_finger = fingerTable_.front();

  try {
    Connection successor_cxn = successor_finger.remote.build();
    successor_cxn.writeAll(message); 

    // Wait for REDRT packet or for a closed connection. Remote will send REDRT
    // if it doesn't have the purview that we expected it to have. Conversely,
    // the remote will close the connection if it does accept the join request.
    try {
      dhtmsg_t redrt_pkt;  
      size_t redrt_pkt_size = sizeof(redrt_pkt);

      // Try to read REDRT packet
      successor_cxn.readAll( (void *) &redrt_pkt, redrt_pkt_size);
      successor_cxn.close();

      // Handle REDRT pkt
      handleRedrt(redrt_pkt, join_pkt, 0);

    } catch (const PrematurelyClosedSocketException& e) {
      // Report that no REDRT packet was received
      std::cout << "Remote closed connection. No REDRT packet received." << std::endl;
      successor_cxn.close();
    }
  } catch (const SocketException& e) {
    std::cout << "Encountered error while forwarding join to successor" << std::endl;
    exit(1);
  }
}

void DhtNode::forwardJoinToFinger(
    dhtmsg_t join_pkt,
    const finger_t& finger
) const {

  // Reassign packet type to JOIN if it was previously JOIN_ATLOC
  if (join_pkt.header.type == JOIN_ATLOC) {
    join_pkt.header.type = JOIN;
  }

  // Fail b/c all packets should have 'type' set to JOIN
  // at this point.
  assert(join_pkt.header.type == JOIN);

  // Open connection to target finger and send message
  std::string message((const char *) &join_pkt, sizeof(join_pkt));

  Connection remote = finger.remote.build();

  try {
    remote.writeAll(message);
    remote.close();
  } catch (const SocketException& e) {
    std::cout << "Failed while trying to forward join packet!" << std::endl;
    exit(1);
  }
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
      << "us<node-id: " << (int) id_ << ">"std::endl;
}

void DhtNode::initReceiver() {
  // Initialize listening socket
  ServiceBuilder builder;
  receiver_ = builder
    .enableAddressReuse()
    .buildNew();

  // Report address of this DhtNode
  std::cout << "DhtNode address: " << receiver_->getDomainName()
      << ":" << receiver_->getPort() << std::endl;
}

void DhtNode::deriveId() {
  // Prepare SHA1 hash inpute: port+address
  uint16_t port = receiver_->getPort();
  size_t port_num_bytes = sizeof(port);
  
  uint32_t ipv4 = receiver_->getIpv4();
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
  const Connection connection = receiver_->accept();

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
    finger_t& predecessor_finger = getPredecessor();
    uint8_t old_predecessor_id = predecessor_finger.node_id;
    handleJoinAcceptance(join_msg);
    
    // Report successful join
    std::cout << "\t- Now replacing former predecessor with joining node..." << 
        "\n\t\t- Id(old predecessor) : " << (int) old_predecessor_id <<
        "\n\t\t- Id(new predecessor) : " << (int) predecessor_finger.node_id << std::endl;

  } else {
    // Report failed JOIN attempt
    std::cout << "\t- Request failed! Couldn't find " << (int) join_msg.node.id
        << " in identifier space (" << (int) id_ << ", " << (int) getPredecessor().node_id
        << std::endl;

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
      std::cout << "\t- Sender " << cxn.getRemoteDomainName() << ":"
        << (int) cxn.getRemotePort() << " DID NOT expect to join with us."
        << std::endl;

      // Forward join request to next best node
      forwardJoin(join_msg);
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
  self.port = htons(receiver_->getPort());
  self.ipv4 = htonl(receiver_->getIpv4());
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
  self.port = htons(receiver_->getPort());
  self.ipv4 = htonl(receiver_->getIpv4());
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
  predecessor_finger.remote.setRemotePort(ntohs(join_msg.node.port));
  predecessor_finger.remote.setRemoteIpv4Address(ntohl(join_msg.node.ipv4));

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

    // Fix table to reflect new node
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
    << (int) cxn.getRemotePort() << "." << std::endl;
}

void DhtNode::forwardJoin(dhtmsg_t join_msg) {
  // Fail b/c this is not a JOIN packet
  assert(join_msg.header.type == JOIN || join_msg.header.type == JOIN_ATLOC);

  // Kill join request if ttl has run out
  if (--join_msg.ttl) {
    return;
  }

  // Check if the join message falls in the range of our successor 
  if (inSuccessorsPurview(join_msg)) {
    // Send to successor and indicate that we expect the sender to have
    // the target id in its purview
    forwardJoinToSuccessor(join_msg);

  } else {
    // Find next best finger to forward the join request to
    size_t finger_idx = findFingerForForwarding(join_msg.node.id);
    
    // Fail b/c finger idx is out of bounds
    assert(finger_idx < fingerTable_.size() - 1);

    const finger_t& target_finger = fingerTable_.at(finger_idx);   

    // Report that we're forwarding the join request and that we don't necessarily
    // expect the target finger to accept the join
    std::cout << "\t- Forwarding JOIN to finger[" << finger_idx << "]:" <<
        "<id: " << (int) target_finger.node_id <<
        ", port: " << (int) target_finger.remote.getRemotePort() <<
        ", ipv4: " << stringifyIpv4(htonl(target_finger.remote.getRemoteIpv4Address())) << 
        ">. We do NOT necessarily expect success..." << std::endl;
   
    // Forward join request to target finger
    forwardJoinToFinger(join_msg, target_finger);
  }
}

void DhtNode::reloadDb() {}

void DhtNode::handleRedrt(
  const dhtmsg_t& redrt_pkt,
  const dhtmsg_t& join_pkt,
  size_t finger_idx
) {
  // Fail b/c 'finger_idx' is out of bounds
  assert(finger_idx < fingerTable_.size() - 1);

  // Fail b/c header of redrt-packet is not REDRT
  assert(redrt_pkt.header.type == REDRT);

  // Fail b/c header of join-packet is not JOIN_ATLOC
  assert(join_pkt.header.type == JOIN_ATLOC);
  
  // Report REDRT packet received
  std::cout << "\t- Received REDRT packet providing new successor: " <<
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
  finger.remote.setRemotePort(ntohs(redrt_pkt.node.port));
  finger.remote.setRemoteIpv4Address(ntohl(redrt_pkt.node.ipv4));

  // Report new finger that we're replacing the old one with 
  std::cout << "\n\t\t- To " <<
      "<id: " << (int) finger.node_id << 
      ", port: " << (int) finger.remote.getRemotePort() << 
      ", ipv4: " << stringifyIpv4(htonl(finger.remote.getRemoteIpv4Address())) << 
      ">" << std::endl;

  // Reload db
  reloadDb();

  // Fix finger table
  fixUp(0);

  // Forward join back to network
  forwardJoin(join_pkt);
}

void DhtNode::handleReid() {
  // Report that we're generating a new id
  std::cout << "\t- Restarting dht socket and generating new id..." << std::endl;

  // Close receiver socket and create a new one
  close();
  initReceiver();

  // Derive a new id from the new address+port of the new 
  // receiving socket and recompute fingers
  deriveId();
  initFingers();

  reportId();
 
  // Retry join
  sendJoinRequest();
}

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
  predecessor_finger.remote.setRemotePort(ntohs(pred.port));
  predecessor_finger.remote.setRemoteIpv4Address(ntohl(pred.ipv4));

  finger_t& successor_finger = fingerTable_.front();
  successor_finger.node_id = succ.id;
  successor_finger.remote
    .setRemotePort(ntohs(succ.port))
    .setRemoteIpv4Address(ntohl(succ.ipv4));

  // Reload db
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

bool DhtNode::inSuccessorsPurview(const dhtmsg_t& join_msg) const {
  const finger_t& successor_finger = fingerTable_.front();
  return ID_inrange(join_msg.node.id, id_, successor_finger.node_id);
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
    default:
      std::cout << "Invalid NodeType: " << type << std::endl;
      exit(1);
  }
}

void DhtNode::reportId() const {
  std::cout << "DhtNode ID: " << (int) id_ << std::endl;
}

DhtNode::DhtNode(uint8_t id) : id_(id), hasTarget_(false) {
  initReceiver();
  initFingers();
  reportId();
}

DhtNode::DhtNode() : hasTarget_(false) {
  initReceiver();
  deriveId();
  initFingers();
  reportId();
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

  bool should_continue = true;

  do {
    int receiver_fd = receiver_->getFd();

    // Listen on 'receiver' socket for dht messages 
    selector.bind(
        receiver_->getFd(),
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

void DhtNode::resetId() {
  // Close current receiver socket
  try {
    receiver_->close(); 
  } catch (const SocketException& e) {
    // Report failure and kill node
    std::cout << "Failed to close receiver socket during id-reset. "
        << "Terminating node with failure." << std::endl;
    exit(1);
  }

  // Open new receiver socket and compute derived id
  initReceiver();  
  deriveId();
}

uint8_t DhtNode::getId() const {
  return id_;
}

void DhtNode::close() {
  try {
    receiver_->close();
  } catch (const SocketException& e) {
    std::cout << "Failed to close DhtNode!" << std::endl;
    exit(1);
  }

  delete receiver_;
}
