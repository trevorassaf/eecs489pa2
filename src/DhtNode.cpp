#include "DhtNode.h"

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

  predecessorNodeId_ = id_;

  printFingers();
}

void DhtNode::printFingers() const {
  std::cout << "--- FINGER PRINT ---" << std::endl;

  for (size_t i = 0; i < FINGER_TABLE_SIZE; ++i) {
    const finger_t finger = fingerTable_[i];
    std::cout << "finger<idx: " << i << ", finger-id: " << (int) finger.finger_id
        << ", node-id: " << (int) finger.node_id << std::endl;
  }

  std::cout << "predecessor<node-id: " << (int) predecessorNodeId_ 
    << ">"<< std::endl;
}

void DhtNode::initReceiver() {
  // Initialize listening socket
  ServiceBuilder builder;
  receiver_ = builder.buildNew();

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

  reportId();
}

void DhtNode::reportId() const {
  std::cout << "DhtNode ID: " << (int) id_ << std::endl;
}

uint8_t DhtNode::foldId(uint16_t unfolded_id) const {
  return unfolded_id % NUM_IDS; 
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
}

void DhtNode::join(const std::string& fqdn, uint16_t port) {
 // TODO write this thang 
}

void DhtNode::listen() {
 // TODO write this thang 
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
