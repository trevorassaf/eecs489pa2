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
#define JOIN_ATLOC_STR "JOIN_ATLOC"
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
     * FQDN of target, if specified.
     */
    std::string targetFqdn_;

    /**
     * Port of target, if specified. Host-byte-order.
     */
    uint16_t targetPort_;

    /**
     * Indicates whether or not a target has been specified.
     * False iff this node is first node in the network.
     */
    bool hasTarget_;

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
     * handleUnexpectedJoinFailureAndClose()
     * - Handle join failure that occurred contrary to 
     *   sender's expectations.
     * @param join_msg : join message
     * @param dead_cxn : closed connection with sender
     */
    void handleUnexpectedJoinFailureAndClose(
        const dhtmsg_t& join_msg, 
        const Connection& dead_cxn);

    /**
     * forwardJoin()
     * - Forward join message to best candidate finger.
     * @param join_msg : message to forward 
     */
    void forwardJoin(dhtmsg_t join_msg);

    /**
     * reloadDb()
     * - Reload images in db to reflect the current identifer interval.
     */
    void reloadDb();

    /**
     * handleRedrt()
     * - Make provded node our new successor and forward the original
     *   join request to the successor.
     * @param redrt_pkt : redirect packet that we received (network-byte-order)
     * @param join_pkt : join packet that we sent (network-byte-order)
     * @param finger_idx : index of finger that we sent join to
     */
    void handleRedrt(
        const dhtmsg_t& redrt_pkt,
        const dhtmsg_t& join_pkt,
        size_t finger_idx);

    /**
     * handleReid()
     * - An ID collision has occurred and we, the newly joining node,
     *   have been instructed to generate a new ID and reconnect.
     */
    void handleReid();

    /**
     * handleWlcmAndCloseCxn()
     * - Read one more dhtnode_t off of the wire and incorporate
     *   provided predecessor/successor into our finger table.
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
     * inOurPurview()
     * - Test if the id of the joining node falls in our range.
     * @param join_msg : join message 
     */
    bool inOurPurview(const dhtmsg_t& join_msg) const;
    
    /**
     * inSuccessorsPurview()
     * - Test if the id of the joining node falls in the range between
     *   use and our sucessor.
     * @param join_msg : join message 
     */
    bool inSuccessorsPurview(const dhtmsg_t& join_msg) const;

    /**
     * senderExpectedJoin()
     * - Test if sender expected this join to succeed.
     * @param join_msg : join message
     */
    bool senderExpectedJoin(const dhtmsg_t& join_msg) const;

    /**
     * reportDhtMsgReceived()
     * - Report dht packet type and sender.
     * @param dhtmsg : dht packet
     */
    void reportDhtMsgReceived(const dhtmsg_t& dhtmsg) const;
   
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

    /**
     * sendJoinRequest()
     * - Send request to target for join.
     */
    void sendJoinRequest();

    /**
     * getPredecessor()
     * - Return reference to predecessor finger.
     * @return predecessor finger. 
     */
    finger_t& getPredecessor();
    
    /**
     * getPredecessor()
     * - Return reference to predecessor finger. Here to allow const reads.
     * @return predecessor finger. 
     */
    const finger_t& getPredecessor() const;

    /**
     * findFingerForForwarding()
     * - Find the finger to forward the request to.
     * - That is, of the fingers in our finger table with IDs less than
     *   the target object, select the finger with the greatest node-id.
     * @param object_id : id of target object
     */
    size_t findFingerForForwarding(uint8_t object_id) const;

    /**
     * forwardJoinToSuccessor()
     * - Send join to successor. Indicate that we expect the join to
     *   succeed by setting the type to JOIN_ATLOC.
     * @param join_pkt : join message
     */
    void forwardJoinToSuccessor(dhtmsg_t join_pkt);

    /**
     * forwardJoinToFinger()
     * - Send join request to provided finger. Type must be JOIN, not JOIN_ATLOC.
     * @param join_pkt : join request
     * @param finger : finger to forward the request to
     */
    void forwardJoinToFinger(dhtmsg_t join_pkt, const finger_t& finger) const;

    /**
     * stringifyIpv4()
     * - Transform numerical ipv4 to dotted decimal.
     * @param ipv4 : numerical version of ipv4 address (network-byte-order)
     * @return ipv4 address in dotted decimal form
     */
    const std::string stringifyIpv4(uint32_t ipv4) const;
    
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
