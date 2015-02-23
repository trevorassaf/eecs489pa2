#include "DhtNode.h"

#include "dht_packets.h"

#include <stdio.h>

void DhtNode::initFingers() {
  // Finger table should hold only FINGER_TABLE_SIZE fingers
  fingerTable_ = std::vector<finger_t>(FINGER_TABLE_SIZE);
  for (size_t i = 0; i < FINGER_TABLE_SIZE; ++i) {
  uint16_t unfolded_finger_id = (1 << i) + id_; 
    
    // Build finger pointing to *self*
    finger_t finger;
    finger.finger_id = foldId(unfolded_finger_id);
    finger.node_id = id_;

    fingerTable_[i] = finger;
  }

  predecessor_.node_id = id_;
}

void DhtNode::fixUp(size_t j) {
  // Fail b/c finger table is malformed
  assert(fingerTable_.size() == FINGER_TABLE_SIZE);

  // Fail b/c j is out of bounds
  assert(j < fingerTable_.size());

  finger_t& finger_j = fingerTable_.at(j);
  for (size_t k = j + 1; k < fingerTable_.size(); ++k) {
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
  assert(fingerTable_.size() == FINGER_TABLE_SIZE);

  // Fail b/c j is out of bounds
  assert(j < fingerTable_.size() && j > 0);

  finger_t& finger_j = fingerTable_.at(j);
  for (size_t k = j - 1; k >= 0; --k) {
    finger_t& finger = fingerTable_.at(k);
    if (finger.finger_id == finger.node_id) {
      break;
    } else if (ID_inrange(finger_j.node_id, finger.finger_id, finger.node_id)) { 
      finger.node_id = finger_j.node_id;
    }
  }
}

// TODO remove this!
void DhtNode::printFingers() const {
  std::cout << "--- FINGER PRINT ---" << std::endl;

  for (size_t i = 0; i < FINGER_TABLE_SIZE; ++i) {
    const finger_t finger = fingerTable_[i];
    std::cout << "finger<idx: " << i << ", finger-id: " << (int) finger.finger_id
        << ", node-id: " << (int) finger.node_id << std::endl;
  }

  std::cout << "predecessor<node-id: " << (int) predecessor_.node_id 
    << ">"<< std::endl;
}

void DhtNode::initReceiver() {
  // Initialize listening socket
  ServiceBuilder builder;
  receiver_ = builder
    .enableAddressReuse()
    .buildNew();

  predecessor_.port = receiver_->getPort();
  predecessor_.ipv4 = receiver_->getIpv4();

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
  reportDhtMsg(message, connection);

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
  
  connection.close();
  
  // Handle ops that permit closed connections
  switch (message.header.type) {
    case REDRT:
      handleRedrt(message, connection);
      break;
    case REID:
      handleReid(message, connection);
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
  std::cout << "--- Adjacent Node Info ---\n\t- predecessor ID: "
      << (int) predecessor_.node_id;
  
  if (predecessor_.node_id == id_) {
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
  // Reject join request, if node's id collides
  if (doesJoinCollide(join_msg)) {
    // Close connection to sender, we're going to reconnect
    // to join initiator
    cxn.close();

    // Report collision
    std::cout << "Requesting JOIN collides with us! Id(self) = " << (int) id_ <<
      ", Id(predecessor) = " << (int) predecessor_.node_id << 
      ", Id(joining-node)" << (int) join_msg.node.id << std::endl;

    // Notify join initiator of collision
    handleJoinCollision(join_msg);

  } else if (canJoin(join_msg)) {
    // Close connection to sender, we're going to reconnect
    // to join initiator
    cxn.close();
    
    // Accept join request, make requester new predecessor
    uint8_t old_predecessor_id = predecessor_.node_id;
    handleJoinAcceptance(join_msg);
    
    // Report successful join
    std::cout << "JOIN accepted! Old precessor id = " << old_predecessor_id <<
      ", new predecessor id = " << predecessor_.node_id << std::endl;

  } else {
    // Report failed JOIN attempt
    std::cout << "JOIN request failed! Couldn't find " << (int) join_msg.node.id
      << " in identifier space (" << (int) id_ << ", " << (int) predecessor_.node_id
      << std::endl;

    if (senderExpectedJoin(join_msg)) {
      // Report that sender expected the join to succeed but it didn't
      std::cout << "But sender " << cxn.getRemoteDomainName() << ":"
        << (int) cxn.getRemotePort() << " DID expect to JOIN with us..."
        << std::endl;

      // Inform sender that the join failed, even though the sender
      // expected the join to succeed
      handleUnexpectedJoinFailure(join_msg, cxn);
      
      // Close connection to sender b/c we're finished with it
      cxn.close();

    } else {
      // Close connection to sender b/c we're going to forward the join
      // along the finger table
      cxn.close();

      // Report that sender did not expect to join with us
      std::cout << "Sender " << cxn.getRemoteDomainName() << ":"
        << (int) cxn.getRemotePort() << " DID NOT expect to JOIN with us."
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
  std::cout << "Sending REID to " << connection.getRemoteDomainName() << ":"
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
  pred.id = predecessor_.node_id;
  pred.port = htons(predecessor_.port);
  pred.ipv4 = htonl(predecessor_.ipv4);
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

  // Report sending WLCM
  std::cout << "Sending WLCM to " << connection.getRemoteDomainName() << ":"
    << connection.getRemotePort() << std::endl;

  // Make joining node our new predecessor
  predecessor_.node_id = join_msg.node.id;
  predecessor_.port = ntohs(join_msg.node.port);
  predecessor_.ipv4 = ntohl(join_msg.node.ipv4);

  // Make joining node our successor too, if we were previously the 
  // only node in the network
  if (id_ == fingerTable_.front().node_id) {
    fingerTable_.front().remote
      .setRemotePort(predecessor_.port)
      .setRemoteIpv4Address(predecessor_.ipv4);

    // Fix table to reflect new node
    fixUp(0);
  } 

  // Reload db because our identifer space has now changed
  reloadDb();
}

void DhtNode::handleUnexpectedJoinFailure(
  const dhtmsg_t& join_msg,
  const Connection& dead_cxn
) const {
  // Assemble REDRT packet
  dhtmsg_t redrt_msg;
  redrt_msg.header = {DHTM_VERS, REDRT};
  
  dhtnode_t pred;
  pred.id = predecessor_.node_id;
  pred.port = htons(predecessor_.port);
  pred.ipv4 = htonl(predecessor_.ipv4);
  redrt_msg.node = pred;
  std::string redrt_str((char *) &redrt_msg, sizeof(redrt_msg));

  // Send packet to sender (not neccessarily node that initiated join request)
  ServerBuilder builder;
  Connection sender = builder
    .setRemotePort(dead_cxn.getRemotePort())
    .setRemoteIpv4Address(dead_cxn.getRemoteIpv4())
    .build();

  sender.writeAll(redrt_str);
  sender.close();

  // Report unexpected join failure
  std::cout << "Sending REDRT packet to " << sender.getRemoteDomainName() << ":"
    << (int) sender.getRemotePort() << "." << std::endl;
}

void DhtNode::forwardJoin(const dhtmsg_t& join_msg) const {
   
}

void DhtNode::reloadDb() {}

void DhtNode::handleRedrt(const dhtmsg_t& msg, const Connection& dead_cxn) {

}

void DhtNode::handleReid(const dhtmsg_t& msg, const Connection& connection) {

}

void DhtNode::handleWlcmAndCloseCxn(
  const dhtmsg_t& msg,
  const Connection& connection
) {

}

void DhtNode::handleSrch(const dhtmsg_t& msg, const Connection& connection) {

}

bool DhtNode::doesJoinCollide(const dhtmsg_t& join_msg) const {
  return predecessor_.node_id == join_msg.node.id || id_ == join_msg.node.id;  
}

bool DhtNode::canJoin(const dhtmsg_t& join_msg) const {
  return ID_inrange(join_msg.node.id, predecessor_.node_id, id_);
}

bool DhtNode::senderExpectedJoin(const dhtmsg_t& join_msg) const {
  return JOIN_ATLOC == join_msg.header.type;
}

void DhtNode::reportDhtMsg(
  const dhtmsg_t& dhtmsg,
  const Connection& connection) const 
{
  std::cout <<
    "Received " << getDhtTypeString(static_cast<DhtType>(dhtmsg.header.type)) <<
    " from " << connection.getRemoteDomainName() << ":" << (int) connection.getRemotePort() <<
    "\n\t- ttl: " << (int) ntohs(dhtmsg.ttl) << 
    "\n\t- id: " << (int) dhtmsg.node.id << 
    "\n\t- port: " << (int) ntohs(dhtmsg.node.port) << 
    "\n\t- ipv4: " << (int) ntohl(dhtmsg.node.ipv4) << std::endl;
}

std::string DhtNode::getDhtTypeString(DhtType type) const {
  switch (type) {
    case JOIN:
      return JOIN_STR;
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

DhtNode::DhtNode(uint8_t id) : id_(id) {
  initReceiver();
  initFingers();
  reportId();
}

DhtNode::DhtNode() {
  initReceiver();
  deriveId();
  initFingers();
  reportId();
}

void DhtNode::joinNetwork(const std::string& fqdn, uint16_t port) {
  // Assemble header packet 
  dhtheader_t header = {DHTM_VERS, DHTM_JOIN};
  
  // Assemble 'self' packet 
  dhtnode_t self;
  self.id = id_; // one byte, no host -> network converstion needed!
  self.port = htons(receiver_->getPort());
  self.ipv4 = htonl(receiver_->getIpv4());

  // Assemble join packet
  dhtmsg_t dhtmsg = {header, htons(DHTM_TTL), self};
  const std::string message((const char *) &dhtmsg, sizeof(dhtmsg));

  std::cout << "Sent JOIN to " << fqdn << ":" << (int) port <<
    "\n\t- ttl: " << (int) ntohs(dhtmsg.ttl) << 
    "\n\t- id: " << (int) dhtmsg.node.id << 
    "\n\t- port: " << (int) ntohs(dhtmsg.node.port) << 
    "\n\t- ipv4: " << (int) ntohl(dhtmsg.node.ipv4) << std::endl;

  // Connect to DHT network through specified target
  ServerBuilder builder; 
  const Connection remote = builder
    .setRemoteDomainName(fqdn)
    .setRemotePort(port)
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
