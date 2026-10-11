#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "../include/packet_parser.h"

// Call back handler
void test_callback(const parsed_packet_t *packet, void *user_context){
    int *packet_count = (int *)user_context;
    (*packet_count)++;
    
    // verify payload
    if(packet->length == 3){
        assert(packet->payload[0] == 0x11);
        assert(packet->payload[1] == 0x22);
        assert(packet->payload[2] == 0x33);
    }
}

// tests
void test_clean_packet(){
    packet_parser_t parser;
    packet_parser_init(&parser);
    int received_count = 0;

    // packet details
    // length = 3 -- 0x00 0x03
    // payload -- 0x11 0x22 0x33
    // CRC: 0xAA ^ 0x55 ^ 0x01 ^ 0x02 ^ 0x00 ^ 0x03 ^ 0x11 ^ 0x22 ^ 0x33 = 0xFF
    uint8_t data[] = {0xAA, 0x55, 0x01, 0x02, 0x00, 0x03, 0x11, 0x22, 0x33, 0xFF};

    packet_parser_feed(&parser, data, sizeof(data), test_callback, &received_count);
    assert(received_count == 1);
    assert(parser.valid_packets == 1);
    printf("Clean packet test passed.\n");
}

void test_fragmented_packet() {
    packet_parser_t parser;
    packet_parser_init(&parser);
    int received_count = 0;

    uint8_t data[] = {0xAA, 0x55, 0x01, 0x02, 0x00, 0x03, 0x11, 0x22, 0x33, 0xFF};

    // Feed the parser 1 single byte at a time to simulate a slow UART connection
    for (size_t i = 0; i < sizeof(data); i++) {
        packet_parser_feed(&parser, &data[i], 1, test_callback, &received_count);
    }
    
    assert(received_count == 1);
    printf("Fragmented packet test passed.\n");
}

void test_hostile_input() {
    packet_parser_t parser;
    packet_parser_init(&parser);
    int received_count = 0;

    // 1. Oversize Attack: length is 0x01 0x01 (257), which exceeds MAX_PAYLOAD (256)
    uint8_t oversize[] = {0xAA, 0x55, 0x01, 0x02, 0x01, 0x01, 0x00};
    packet_parser_feed(&parser, oversize, sizeof(oversize), test_callback, &received_count);
    
    assert(received_count == 0);
    assert(parser.oversize_errors == 1);

    // 2. Bad CRC: Valid header and payload, but bad CRC byte at the end (0x00 instead of 0xFF)
    uint8_t bad_crc[] = {0xAA, 0x55, 0x01, 0x02, 0x00, 0x03, 0x11, 0x22, 0x33, 0x00};
    packet_parser_feed(&parser, bad_crc, sizeof(bad_crc), test_callback, &received_count);
    
    assert(received_count == 0);
    assert(parser.crc_errors == 1);

    // 3. Noise Recovery: A random 0xAA before the real start sequence
    uint8_t noisy_start[] = {0xAA, 0xAA, 0x55, 0x01, 0x02, 0x00, 0x03, 0x11, 0x22, 0x33, 0xFF};
    packet_parser_feed(&parser, noisy_start, sizeof(noisy_start), test_callback, &received_count);
    
    assert(received_count == 1); // Recovered and found the packet!

    printf("Hostile input test passed.\n");
}

int main() {
    printf("Running Packet Parser Tests...\n");
    test_clean_packet();
    test_fragmented_packet();
    test_hostile_input();
    printf("All tests passed successfully!\n");
    return 0;
}