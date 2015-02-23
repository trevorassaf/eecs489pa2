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
#include "dht_packets.h"

#define FINGER_TABLE_SIZE 8
#define NUM_IDS 0x100 // 2^FINGER_TABLE_SIZE
#define SIZE_OF_ADDR_PORT 6

// DhtType Strings
#define JOIN_STR "JOIN"
#define REDRT_STR "REDRT"
#define REID_STR "REID"
#define WLCM_STR "WLCM"
#define SRCH_STR "SRCH"

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
     * Predecessor data stored in host-byte-order.
     */
    struct predecessor_t {
      uint8_t node_id;
      uint8_t port;
      uint32_t ipv4;
    };

    /**
     * Predecessor node.
     */
    predecessor_t predecessor_;

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
     * handleJoinAndCloseCxn()
     * - Read and process join message request
     * @param msg : packet from the network (network-byte-order)
     * @param connection : connection to requesting node
     */
    void handleJoinAndCloseCxn(const dhtmsg_t& msg, const Connection& connection);

    /**
     * handleJoinCollision()
     * - Join request collided -- instruct remote to regenerate its id.
     * @param msg : join request
     */
    void handleJoinCollision(const dhtmsg_t& join_msg);

    /**
     * handleJoinAcceptance()
     * - Accept join request by making the requesting remote our
     *   new predecessor.
     * @param join_msg : join request
     */
    void handleJoinAcceptance(const dhtmsg_t& join_msg);

    /**
     * handleUnexpectedJoinFailure()
     * - Handle join failure that occurred contrary to 
     *   sender's expectations.
     * @param join_msg : join message
     * @param dead_cxn : closed connection with sender
     */
    void handleUnexpectedJoinFailure(
        const dhtmsg_t& join_msg, 
        const Connection& dead_cxn
    ) const;

    /**
     * forwardJoin()
     * - Forward join message to best finger.
     * @param join_msg : join message payload 
     */
    void forwardJoin(const dhtmsg_t& join_msg) const;

    /**
     * reloadDb()
     * - Reload images in db to reflect the current identifer interval.
     */
    void reloadDb();

    /**
     * handleRedrt()
     * - Read and process join message request
     * @param msg : packet from the network (network-byte-order)
     * @param connection : connection to requesting node
     */
    void handleRedrt(const dhtmsg_t& msg, const Connection& connection);

    /**
     * handleReid()
     * - Read and process join message request
     * @param msg : packet from the network (network-byte-order)
     * @param connection : connection to requesting node
     */
    void handleReid(const dhtmsg_t& msg, const Connection& connection);

    /**
     * handleWlcmAndCloseCxn()
     * - Read and process join message request
     * @param msg : packet from the network (network-byte-order)
     * @param connection : connection to requesting node
     */
    void handleWlcmAndCloseCxn(const dhtmsg_t& msg, const Connection& connection);

    /**
     * handleSrch()
     * - Read and process join message request
     * @param msg : packet from the network (network-byte-order)
     * @param connection : connection to requesting node
     */
    void handleSrch(const dhtmsg_t& msg, const Connection& connection);

    /**
     * doesJoinCollide()
     * - Test if the id of the joining node collides with the
     *   id of the current node or that of its predecessor.
     * @param join_msg : join message 
     */
    bool doesJoinCollide(const dhtmsg_t& join_msg) const;

    /**
     * canJoin()
     * - Test if the id of the joining node falls in the range between
     *   the current node and its predecessor.
     * @param join_msg : join message 
     */
    bool canJoin(const dhtmsg_t& join_msg) const;

    /**
     * senderExpectedJoin()
     * - Test if sender expected this join to succeed.
     * @param join_msg : join message
     */
    bool senderExpectedJoin(const dhtmsg_t& join_msg) const;

    /**
     * reportDhtMsg()
     * - Report dht message packet received.
     * @param dhtmsg : dht message packet
     * @param connection : connection to sending node
     */
    void reportDhtMsg(const dhtmsg_t& dhtmsg, const Connection& connection) const;
   
    /**
     * getDhtTypeStriing()
     * - Return string representation of dht type.
     * @param type : type of dhtmsg 
     */
    std::string getDhtTypeString(DhtType type) const;

    /**
     * fixUp()
     * - Fix up the finger table.
     * @param  
     */
    void fixUp(size_t idx);

    /**
     * fixDown()
     * - Fix down the finger table.
     * @param  
     */
    void fixDown(size_t idx);

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
