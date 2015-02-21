#pragma once

#include <iostream>
#include <vector>

#include "SocketException.h"
#include "ServiceBuilder.h"
#include "Service.h"
#include "ServerBuilder.h"
#include "Connection.h"
#include "hash.h"

#define FINGER_TABLE_SIZE 8
#define NUM_IDS 0x10 // 2^FINGER_TABLE_SIZE
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
     * Finger table holding addresses of known successors.
     */
    std::vector<finger_t> fingerTable_;

    /**
     * initFingerTable()
     * - Construct and set finger table based on the node's current id.
     */
    void initFingerTable();

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
     * join()
     * - Send join request to successor.
     * @param fqdn : fqdn of target
     * @param port : port of target
     */
    void join(const std::string& fqdn, uint16_t port);

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

};
