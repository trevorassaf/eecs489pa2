#pragma once

#include <iostream>
#include <vector>

#include "SocketException.h"
#include "ServiceBuilder.h"
#include "Service.h"
#include "ServerBuilder.h"
#include "Connection.h"
#include "hash.h"
#include "Selector.h"

#define FINGER_TABLE_SIZE 8
#define NUM_IDS 0x100 // 2^FINGER_TABLE_SIZE
#define SIZE_OF_ADDR_PORT 6

class DhtNode {

  private:
    /**
     * Socket for receiving join messages.
     */
    const Service* receiver_; 

    /**
     * SHA1 id into 8 bits.
     */
    uint8_t id_;

    /**
     * Finger container -- tracks a successor.
     */
    struct finger_t {
      ServerBuilder remote;
      uint8_t finger_id, node_id;
    };

    /**
     * Predecessor node.
     */
    uint8_t predecessorNodeId_;

    /**
     * Finger table holding addresses of known successors.
     */
    std::vector<finger_t> fingerTable_;

    /**
     * initFingers()
     * - Construct and set finger table based on the node's current id.
     */
    void initFingers();

    /**
     * initReceiver()
     * - Construct and set listening socket. Prints out the address.
     */
    void initReceiver();

    /**
     * deriveId()
     * - Compute and set id from receiver address.
     */
    void deriveId();

    /**
     * foldId()
     * - Collapse id onto 8-bit ring.
     * @param unfolded_id : 16-bit id 
     */
    uint8_t foldId(uint16_t unfolded_id) const; 

    /**
     * reportId()
     * - Notify user of id value.
     */
    void reportId() const;

    /**
     * handleDhtTraffic()
     * - Read dht traffic from the wire and process request.
     */
    void handleDhtTraffic();

    /**
     * handleCliInput()
     * - Read cli input from stdin and process request.
     * - EOF | q | Q -> quit node
     * - p -> print node's successor/predecessor IDs and flush input
     * @return true iff node should continue to listen 
     */
    bool handleCliInput();

    /**
     * reportCliInstructions()
     * - Print instructions for controlling dht node from cli.
     */
    void reportCliInstructions() const;

    /**
     * reportAdjacentNodes()
     * - Print IDs for predecessor and successor nodes.
     */
    void reportAdjacentNodes() const;

    /**
     * handleJoin()
     * - Read and process join message request
     */
    void handleJoin(const Connection& connection);
    
    /**
     * handleRedrt()
     * - Read and process join message request
     */
    void handleRedrt(const Connection& connection);
    
    /**
     * handleReid()
     * - Read and process join message request
     */
    void handleReid(const Connection& connection);
    
    /**
     * handleWlcm()
     * - Read and process join message request
     */
    void handleWlcm(const Connection& connection);

    // TODO remove!
    void printFingers() const;

  public:
    /**
     * DhtNode()
     * - Create node with custom id. 
     * @param id : id of node (override default computation)
     */
    DhtNode(uint8_t id);

    /**
     * DhtNode()
     * - Create node with id derived from address.
     */
    DhtNode();

    /**
     * joinNetwork()
     * - Send join request to successor.
     * @param fqdn : fqdn of target
     * @param port : port of target
     */
    void joinNetwork(const std::string& fqdn, uint16_t port);

    /**
     * run()
     * - Await incoming messages.
     */
    void run();

    /**
     * resetId()
     * - Restart listening socket on new ephemeral port and 
     *   recompute id.
     */
    void resetId();

    /**
     * getId()
     * - Return this node's ID.
     */
    uint8_t getId() const;

    /**
     * close()
     * - Tear down this node.
     */
    void close();

};
