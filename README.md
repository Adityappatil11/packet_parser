# Binary Packet Parser

A robust, incremental, event-driven binary packet parser written in C. This project is designed for embedded systems and networking applications where data arrives over unreliable transports (like UART, SPI, or raw TCP/UDP sockets).

## Features
* **Incremental Parsing:** Uses a lightweight state machine to process data byte-by-byte. It gracefully handles fragmented data arriving in arbitrary chunk sizes.
* **Architecture Agnostic (No Struct Casting):** Safely parses Big-Endian payload lengths using bitwise shifts. Completely avoids struct casting to prevent unaligned memory access faults and compiler padding corruption.
* **Hostile Input Protection:** Strictly bounds-checks payload sizes to prevent buffer overflows.
* **Fault Recovery:** Automatically recovers from noise and malformed headers without dropping valid overlapping packets.

## Project Structure
```text
.
├── Makefile
├── README.md
├── include/
│   └── packet_parser.h    # API contract and state machine structs
├── src/
│   └── packet_parser.c    # Core state machine implementation
└── tests/
    └── test_parser.c      # Automated test suite (fragmentation, attacks)
```

## Packet Format
| Magic 1 | Magic 2 | Version | Type | Length (Big-Endian) | Payload | CRC (XOR) |
|---------|---------|---------|------|---------------------|---------|-----------|
| `0xAA`  | `0x55`  | 1 byte  | 1 byte | 2 bytes           | N bytes | 1 byte    |

## Quick Start / Usage

This parser uses an **Event-Driven Callback** architecture. You feed it raw bytes, and it only interrupts you when a fully valid, uncorrupted packet is assembled.

```c
#include <stdio.h>
#include "packet_parser.h"

// 1. Define your callback
void on_packet_received(const parsed_packet_t *packet, void *context) {
    printf("Received valid packet type: 0x%02X, length: %d\n", 
           packet->type, packet->length);
}

int main() {
    packet_parser_t parser;
    packet_parser_init(&parser);

    // Incoming raw stream (could be from a UART interrupt)
    uint8_t rx_buffer[] = {0xAA, 0x55, 0x01, 0xFF, 0x00, 0x02, 0xDE, 0xAD, 0x8C};

    // 2. Feed the parser
    // If the packet is complete and the CRC matches, on_packet_received is fired.
    packet_parser_feed(&parser, rx_buffer, sizeof(rx_buffer), on_packet_received, NULL);

    return 0;
}
```

## Building and Testing

To compile the codebase:
```bash
make
```

To run the automated test suite (verifies clean packets, 1-byte fragmentation, and oversize/CRC/noise attacks):
```bash
make test
```

## Design Decisions & Interview Notes

* **Avoiding Struct Casting:** Casting a byte array directly to a struct pointer is dangerous. It assumes the compiler hasn't added hidden padding bytes, assumes the CPU allows unaligned memory access (which causes HardFaults on many ARM chips), and ignores endianness. This parser uses bitwise operations to guarantee safe, deterministic deserialization on any architecture.
* **Big-Endian Reassembly:** The 16-bit length field is transmitted in Network Byte Order (Big-Endian). The parser reassembles this using `(b1 << 8) | b2`, ensuring correctness regardless of whether the host CPU is Little-Endian or Big-Endian.
* **Malicious Input Handling:** The parser protects against oversize attacks by immediately dropping payloads larger than `PACKET_MAX_PAYLOAD`. It also guards against cascading packet loss: if it encounters an invalid byte during parsing, it explicitly checks if that invalid byte is actually the `0xAA` start sequence of the *next* packet, preventing deadlocks in noisy streams.