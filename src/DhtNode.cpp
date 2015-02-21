#include "DhtNode.h"

void DhtNode::initFingerTable() {
  // Finger table should hold only FINGER_TABLE_SIZE fingers
  fingerTable_ = std::vector<finger_t>(FINGER_TABLE_SIZE);
  uint16_t id_base = id_; 
  for (size_t i = 0; i < FINGER_TABLE_SIZE; ++i) {
    id_base += 1 << i;
    
    // Build finger pointing to *self*
    finger_t finger;
    finger.finger_id = foldId(id_base + id_);
    finger.node_id = id_;

    fingerTable_[i] = finger;
  }
}

void DhtNode::initReceiver() {

  ServiceBuilder builder;
  receiver_ = builder.buildNew();

  // Report address of this DhtNode
  std::cout << "DhtNode address: " << receiver_->getDomainName()
      << ":" << receiver_->getPort() << std::endl;
}

void DhtNode::deriveId() {
  char addrport[SIZE_OF_ADDR_PORT + 1];
  unsigned char md[SHA1_MDLEN];

  // Compute SHA1 of address+port 
  uint16_t port = receiver_->getPort();
  const std::string& domain_name = receiver_->getDomainName();
  
  memcpy(addrport, (char *) &port, sizeof(port));
  memcpy(addrport + sizeof(port), domain_name.c_str(), domain_name.size());
  addrport[SIZE_OF_ADDR_PORT] = '\0';

  SHA1((unsigned char *) addrport, SIZE_OF_ADDR_PORT, md);
  
  // Fold SHA1 hash into node id 
  id_ = ID(md);

  // Report node id
  std::cout << "DhtNode ID: " << id_ << std::endl;
}

uint8_t DhtNode::foldId(uint16_t unfolded_id) const {
  return unfolded_id % NUM_IDS; 
}

DhtNode::DhtNode(uint8_t id) : id_(id) {
  initReceiver();
  initFingerTable();
}

DhtNode::DhtNode() {
  initReceiver();
  deriveId();
  initFingerTable();
}

void DhtNode::join(const std::string& fqdn, uint16_t port) {
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
