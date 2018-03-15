struct ipv4_hdr {
    uint8_t version_ihl;        /**< version and header length */
    uint8_t type_of_service;    /**< type of service */
    uint16_t total_length;        /**< length of packet */
    uint16_t packet_id;        /**< packet ID */
    uint16_t fragment_offset;    /**< fragmentation offset */
    uint8_t time_to_live;        /**< time to live */
    uint8_t next_proto_id;        /**< protocol ID */
    uint16_t hdr_checksum;        /**< header checksum */
    uint32_t src_addr;        /**< source address */
    uint32_t dst_addr;        /**< destination address */
} __attribute__((__packed__));

struct udp_hdr {
    uint16_t src_port;    /**< UDP source port. */
    uint16_t dst_port;    /**< UDP destination port. */
    uint16_t dgram_len;   /**< UDP datagram length */
    uint16_t dgram_cksum; /**< UDP datagram checksum */
} __attribute__((__packed__));


struct ether_hdr {
    struct ether_addr d_addr; /**< Destination address. */
    struct ether_addr s_addr; /**< Source address. */
    uint16_t ether_type;      /**< Frame type. */
} __attribute__((__packed__));


struct RequestBatchHeader {
    // 0
    //uint8_t header[sizeof(ether_hdr) + sizeof(ipv4_hdr) + sizeof(udp_hdr)];
    // 42
    uint8_t magic;  // 0x78 for requests; 0x79 for responses.
    uint8_t num_requests;
    // 44
    uint32_t reserved0;
    // 48
    // KeyValueRequestHeader
}__attribute__ ((packed));

enum class Operation {
    kReset = 0,
    kNoopRead,
    kNoopWrite,
    kAdd,
    kSet,
    kGet,
    kTest,
    kDelete,
    kIncrement,
};
enum class Result {
    kSuccess = 0,
    kError,
    kInsufficientSpace,
    kExists,
    kNotFound,
    kPartialValue,
    kNotProcessed,
    kNotSupported,
    kTimedOut,
    kRejected,
};

struct RequestHeader {
    // 0
    uint8_t operation;  // ::mica::processor::Operation
    uint8_t result;     // ::mica::table::Result
    // 2
    uint16_t reserved0;
    // 4
    uint32_t kv_length_vec;  // key_length: 8, value_length: 24
    // 8
    uint64_t key_hash;
    // 16
    uint32_t opaque;
    uint32_t reserved1;
    // 24
    // Key-value data
}__attribute__ ((packed));

