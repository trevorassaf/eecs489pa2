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
#include "ImageDb.h"
#include "netimg_packets.h"
#include "ltga.h"

#define FINGER_TABLE_SIZE 8
#define SUCCESSOR_IDX 0
#define PREDECESSOR_IDX FINGER_TABLE_SIZE

#define NUM_IDS 0x100 // 2^FINGER_TABLE_SIZE
#define SIZE_OF_ADDR_PORT 6

// DhtType Strings
#define JOIN_STR "JOIN"
#define JOIN_ATLOC_STR "JOIN_ATLOC"
#define REDRT_STR "REDRT"
#define REID_STR "REID"
#define WLCM_STR "WLCM"
#define SRCH_STR "SRCH"
#define SRCH_ATLOC_STR "SRCH_ATLOC"
#define RPLY_STR "RPLY"
#define MISS_STR "MISS"

class DhtNode {

  private:
    /**
     * Image database handle.
     */
    ImageDb* imageDb_;

    /**
     * Connection to the netimg client that we're proxying.
     */
    const Connection * imageClient_;
    
    /**
     * Socket for receiving dht and image traffic.
     */
    const Service * dhtReceiver_, * imageReceiver_; 

    /**
     * Specifies whether or not we're currently servicing and image query.
     */
    bool servicingImageQuery_;

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
     * initDhtReceiver()
     * - Construct and set listening socket for monitoring dht traffic. 
     *   Prints out the address.
     */
    void initDhtReceiver();

    /**
     * initImageReceiver()
     * - Construct and set listening socket for monitoring image traffic.
     *   Print out the address.
     */
    void initImageReceiver();

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
     * handleImageTraffic()
     * - Read data from image socket and process it.
     */
    void handleImageTraffic();

    /**
     * handleLocalQuerySuccess()
     * - Found image in local db. Stream down to client.
     * @param file_name : name of file to search for
     */
    void handleLocalQuerySuccess(const std::string& file_name);

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

    void sendImageNotFound();

    /**
     * sendRedrt()
     * - Handle atloc failure that occurred contrary to 
     *   sender's expectations.
     * @param join_msg : join message
     * @param dead_cxn : closed connection with sender
     */
    void sendRedrt(const Connection& dead_cxn);

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
     * handleJoinRedrt()
     * - Make provded node our new successor and forward the original
     *   join request to the successor.
     * @param redrt_pkt : redirect packet that we received (network-byte-order)
     * @param join_pkt : join packet that we sent (network-byte-order)
     * @param finger_idx : index of finger that we sent join to
     */
    void handleJoinRedrt(
        const dhtmsg_t& redrt_pkt,
        const dhtmsg_t& join_pkt,
        size_t finger_idx);

    /**
     * handleRedrt()
     * - Assimilate provided finger into finger table.
     * @param redrt_pkt : redirect packet received from peer finger
     * @param finger_idx : index of finger to replace
     */
    void handleRedrt(const dhtmsg_t& redrt_pkt, size_t finger_idx);
    
    /**
     * handleSrchRedrt()
     * - Make provded node our new successor and forward the original
     *   join request to the successor.
     * @param redrt_pkt : redirect packet that we received (network-byte-order)
     * @param srch_pkt : search packet that we sent (network-byte-order)
     * @param finger_idx : index of finger that we sent join to
     */
    void handleSrchRedrt(
        const dhtmsg_t& redrt_pkt,
        const dhtsrch_t& srch_pkt,
        size_t finger_idx);

    /**
     * handleRemoteImageQuerySuccess()
     * - Found imaage at remote finger. Now we need to notify the
     *   original dht image proxy.
     * @param srch_pkt : search packet 
     */
    void handleRemoteImageQuerySuccess(const dhtsrch_t& srch_pkt);

    /**
     * handleReid()
     * - An ID collision has occurred and we, the newly joining node,
     *   have been instructed to generate a new ID and reconnect.
     */
    void handleReid();
    
    /**
     * handleSrchAndCloseCxn()
     * - Read the remainder of the dhtsrch_t packet off of the wire and 
     *   try to service image query. If image exists in db, then return
     *   to querying node. If not, forward the query to the dht.
     * @param msg : packet from the network (network-byte-order)
     * @param connection : connection to requesting node
     */
    void handleSrchAndCloseCxn(const dhtmsg_t& msg, const Connection& connection);

    /**
     * handleRplyAndCloseCxn()
     * - Read the remainder of the dhtsrch_t packet off of the wire and then
     *   stream the image back to the netimg client.
     * @param msg : packet from the network (network-byte-order)
     * @param connection : connection to requesting node
     */
    void handleRplyAndCloseCxn(const dhtmsg_t& msg, const Connection& connection);

    /**
     * handleMissAndCloseCxn()
     * - Read the remainder of the dhtsrch_t packet off of the wire
     *   and notify the netimg client that the search failed.
     * @param msg : packet from the network (network-byte-order)
     * @param connection : connection to requesting node
     */
    void handleMissAndCloseCxn(const dhtmsg_t& msg, const Connection& connection);

    /**
     * handleWlcmAndCloseCxn()
     * - Read one more dhtnode_t off of the wire and incorporate
     *   provided predecessor/successor into our finger table.
     * @param msg : packet from the network (network-byte-order)
     * @param connection : connection to requesting node
     */
    void handleWlcmAndCloseCxn(const dhtmsg_t& msg, const Connection& connection);

    /**
     * doesJoinCollide()
     * - Test if the id of the joining node collides with the
     *   id of the current node or that of its predecessor.
     * @param join_msg : join message 
     */
    bool doesJoinCollide(const dhtmsg_t& join_msg) const;

    /**
     * inOurPurview()
     * - Test if the id of the object falls in our range.
     * @param id : id of object 
     */
    bool inOurPurview(uint8_t id) const;
    
    /**
     * inSuccessorsPurview()
     * - Test if the id of the joining node falls in the range between
     *   use and our sucessor.
     * @param object_id : id of object we're trying to place 
     */
    bool inSuccessorsPurview(uint8_t object_id) const;

    /**
     * senderExpectedJoin()
     * - Test if sender expected this join to succeed.
     * @param join_msg : join message
     */
    bool senderExpectedJoin(const dhtmsg_t& join_msg) const;

    /**
     * senderExpectedSearchSuccess()
     * - Test if sender expected the search query to succeed.
     * @param pkt : search packet
     */
    bool senderExpectedSearchSuccess(const dhtsrch_t& pkt) const;

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
    std::string stringifyDhtType(DhtType type) const;

    /**
     * fixUp()
     * - Fix up the finger table.
     * @param idx: index of finger to start at 
     */
    void fixUp(size_t idx);

    /**
     * fixDown()
     * - fix down the finger table.
     * @param idx: index of finger to start at 
     */
    void fixDown(size_t idx);

    /**
     * sendJoinRequest()
     * - Send request to target for join.
     */
    void sendJoinRequest();

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
     * expectToFindObject()
     * - Return true b/c the object-id is between the finger's fID
     *   and finger id value.
     * @param object_id : id of object
     * @param finger : finger we're forwarding the object to
     */
    bool expectToFindObject(uint8_t object_id, const finger_t& finger) const;

    /**
     * stringifyIpv4()
     * - Transform numerical ipv4 to dotted decimal.
     * @param ipv4 : numerical version of ipv4 address (network-byte-order)
     * @return ipv4 address in dotted decimal form
     */
    const std::string stringifyIpv4(uint32_t ipv4) const;

    /**
     * stringifyFinger()
     * - Transform finger to human-readable format.
     * @param finger : finger to stringify
     * @return string representation of finger
     */
    const std::string stringifyFinger(const finger_t& finger) const;

    /**
     * rejectNetimgQuery()
     * - Deny client query b/c we're already handling a query.
     * @param csn : connection to netimg client
     */
    void rejectNetimgQuery(const Connection* cxn) const;

    /**
     * loadImsgPacket()
     * - Inflate imsg packet with data from ltga img
     * @param ltga : image
     * @param imsg : packet to return to sender
     */
    size_t loadImsgPacket(LTGA& ltga, imsg_t& imsg) const;

    /**
     * forwardInitialImageQueryToDht()
     * - Send image query along fingers in dht.
     * - CAUTION: used for first image forward ONLY
     * @param file_name : name of image file
     */
    void forwardInitialImageQuery(const std::string& file_name);
    
    /**
     * forwardImageQuery()
     * - Send image query along fingers in dht. May be used for either
     *   initial forward or secondary forwards. Drop if ttl is too low.
     * @param srch_pkt : packet containing search query 
     */
    void forwardImageQuery(dhtsrch_t srch_pkt);

    /**
     * returnNotFoundToImageProxy()
     * - Image doesn't exist in DHT, so send a NFOUND packet to image proxy.
     * @param srch_pkt : search packet
     */
    void returnNotFoundToImageProxy(const dhtsrch_t& srch_pkt);
    
    /**
     * forwardImageQueryWithoutTtl()
     * - Send image query along fingers in dht. May be used for either
     *   initial forward or secondary forwards. Don't drop if ttl is too low.
     * @param srch_pkt : packet containing search query 
     */
    void forwardImageQueryWithoutTtl(dhtsrch_t srch_pkt);

    /**
     * stringifySrchPkt()
     * - Convert search packet to human-readable format.
     * @param pkt : search packet
     */
    const std::string stringifySrchPkt(const dhtsrch_t& pkt) const;
  
    /**
     * updatePredecessorAndImageDb()
     * - Change predecessor to provided node. Reload the image database.
     * @param id: id of new predecessor
     * @param port: port of new predecessor (host-byte-order)
     * @param ipv4: ipv4 address of new predecessor (host-byte-order)
     */
    void updatePredecessorAndImageDb(uint8_t id, uint16_t port, uint32_t ipv4);

    /**
     * updateSuccessor()
     * - Change successor to provided node.
     * @param id: id of new successor 
     * @param port: port of new successor (host-byte-order)
     * @param ipv4: ipv4 address of new successor (host-byte-order)
     */
    void updateSuccessor(uint8_t id, uint16_t port, uint32_t ipv4);

    /**
     * updateFinger()
     * - Update the finger table with the new entry. 
     * - Fixes the finger table, as needed.
     * @param idx: index in the finger to update
     * @param id: id of the new finger
     * @param port: port of the new finger (host-byte-order)
     * @param ipv4: ipv4 address of the new finger (host-byte-order)
     */
    void updateFinger(size_t idx, uint8_t id, uint16_t port, uint32_t ipv4);

    // TODO remove!
    void printFingers() const;
    void dumpSrchPacket(const dhtsrch_t& pkt);


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
     * close()
     * - Tear down this node.
     */
    void close();

};
