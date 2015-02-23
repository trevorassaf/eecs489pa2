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

  predecessorNodeId_ = id_;
}

// TODO remove this!
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

  // Read packet header
  dhtheader_t header;
  size_t header_size = sizeof(header);
  
  connection.readAll( (void *) &header, header_size);

  // Fail b/c we received an invalid version
  if (header.vers != DHTM_VERS) {
    std::cout << "Invalid type received! Expected: " << DHTM_VERS <<
        ", but received: " << (int) header.vers << std::endl;
    exit(1);
  }
  
  // Report dht packet type. Delegate functions MUST close connection asap.
  switch (header.type) {
    case JOIN:
      handleJoin(connection);
      break;
    case REDRT:
      handleRedrt(connection);
      break;
    case REID:
      handleReid(connection);
      break;
    case WLCM:
      handleWlcm(connection);
      break;
    default:
      std::cout << "Invalid header type received: " << (int) header.type
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
      << (int) predecessorNodeId_;
  
  if (predecessorNodeId_ == id_) {
    std::cout << " (self)";
  }

  uint8_t successor_id = fingerTable_.front().node_id;
  std::cout << "\n\t- successor ID: " << (int) successor_id;
  
  if (successor_id == id_) {
    std::cout << " (self)";
  }

  std::cout << "\n--------------------" << std::endl;
}

void DhtNode::handleJoin(const Connection& connection) {
  // Read dhtmsg_body packet
  dhtmsg_body_t body; 
  size_t body_size = sizeof(body);

  connection.readAll((void *) &body, body_size);
  connection.close(); 

  // Report that we've received a JOIN request
  std::cout << "Received JOIN<ttl: " << (int) (body.ttl) << 
    ", id: " << (int) body.node.id << ", port: " <<
    (int) (body.node.port) << ", ipv4: " << 
    (int) (body.node.ipv4.s_addr) << std::endl;
  /*std::cout << "Received JOIN<ttl: " << (int) ntohs(body.ttl) << 
    ", id: " << (int) body.node.id << ", port: " <<
    (int) ntohs(body.node.port) << ", ipv4: " << 
    (int) ntohl(body.node.ipv4.s_addr) << std::endl;
 */ 
}

void DhtNode::handleRedrt(const Connection& connection) {

}

void DhtNode::handleReid(const Connection& connection) {

}

void DhtNode::handleWlcm(const Connection& connection) {

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
  // dhtheader_t header = {DHTM_VERS, DHTM_JOIN};
  //
  // // Assemble 'self' packet 
  // dhtnode_t self;
  // self.id = id_; // one byte, no host -> network converstion needed!
  // self.port = htons(receiver_->getPort());
  // self.ipv4.s_addr = htonl(receiver_->getIpv4());
  //
  // dhtmsg_body_t body = {htons(DHTM_TTL), self};
  dhtheader_t header = {DHTM_VERS, DHTM_JOIN};
  
  // Assemble 'self' packet 
  dhtnode_t self;
  self.id = id_; // one byte, no host -> network converstion needed!
  self.port = htons(receiver_->getPort());
  self.ipv4.s_addr = htonl(receiver_->getIpv4());

  // Assemble 'body' packet
  dhtmsg_body_t body;
  body.ttl = htons(DHTM_TTL);
  body.node = self;

  std::cout << "Sent JOIN<ttl: " << (int) (body.ttl) << 
    ", id: " << (int) body.node.id << ", port: " <<
    (int) (body.node.port) << ", ipv4: " << 
    (int) (body.node.ipv4.s_addr) << std::endl;

  // Assemble join message
  dhtmsg_t dhtmsg = {header, body};
  const std::string message((const char *) &dhtmsg, sizeof(dhtmsg));

  // Connect to DHT network through specified target
  ServerBuilder builder; 
  const Connection remote = builder
    .setRemoteDomainName(fqdn)
    .setRemotePort(port)
    .build();

  // Send join message to first network node and close connection
  try {
    std::cout << "writing message!" << std::endl;
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
