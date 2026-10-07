#ifndef PACKET_PARSER_H
#define PACKET_PARSER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// Packet format configuration 
#define PACKET_MAGIC1 0xAA
#define PACKET_MAGIC2 0x55
#define PACKET_MAX_PAYLOAD 256

// State machine states
typedef enum{
    STATE_WAIT_MAGIC1,
    STATE_WAIT_MAGIC2,
    STATE_WAIT_VERSION,
    STATE_WAIT_TYPE,
    STATE_WAIT_LEN_H,
    STATE_WAIT_LEN_L,
    STATE_WAIT_LEN_PAYLOAD,
    STATE_WAIT_CRC
}parser_state_t;

// Structure to provide once parsing is complete
typedef struct{
    uint8_t version;
    uint8_t type;
    uint8_t length;
    uint8_t payload[PACKET_MAX_PAYLOAD];
    uint8_t crc;
}parsed_packet_t;

// to mantain states between incomplete chuncks of data
typedef struct{
    parser_state_t state;
    parsed_packet_t current_packet;
    uint16_t payload_bytes_received;
    uint8_t calculated_crc;

    // Debugging
    uint32_t valid_packets;
    uint32_t crc_errors;
    uint32_t oversize_errors;
}packet_parser_t;

/**
 * @brief Function pointer type for the callback.
 * When the parser completes a valid packet, it calls this function.
 */
typedef void (*packet_handler_t)(const parsed_packet_t *packet, void *user_context);

/**
 * @brief Initializes the parser context.
 */
void packet_parser_init(packet_parser_t *parser);

/**
 * @brief Feeds incoming data to the parser state machine.
 * 
 * @param parser Pointer to the parser context.
 * @param data Incoming byte array.
 * @param len Number of bytes in this chunk.
 * @param handler Callback to trigger when a packet is successfully verified.
 * @param user_context Arbitrary pointer passed back to the handler.
 */
void packet_parser_feed(packet_parser_t *parser, const uint8_t *data, size_t len, packet_handler_t handler, void *user_context);

#endif // PACKET_PARSER