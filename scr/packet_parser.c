#include "packet_parser.h"

void packet_parser_init(packet_parser_t *parser){
    if(!parser) return;

    memset(parser,0,sizeof(packet_parser_t));
    parser->state = STATE_WAIT_MAGIC1;
} 

// internal helper function to reset machine on failure
static void reset_parser(packet_parser_t *parser){
    parser->state = STATE_WAIT_MAGIC1;
    parser->calculated_crc = 0;
    parser->payload_bytes_received = 0;
}

void packet_parser_feed(packet_parser_t *parser, const uint8_t *data, size_t len, packet_handler_t handler, void *user_context){
    if(!parser || !data) return;

    for(size_t i = 0; i < len; i++){
        uint8_t b = data[i];

        if(parser->state != STATE_WAIT_MAGIC1 && parser->state != STATE_WAIT_CRC){
            parser->calculated_crc ^= b;
        }

        switch(parser->state){
            case STATE_WAIT_MAGIC1:
                if(b == PACKET_MAGIC1){
                    parser->state = STATE_WAIT_MAGIC2;
                    parser->calculated_crc = b;
                }
                break;
            case STATE_WAIT_MAGIC2:
                if(b == PACKET_MAGIC2){
                    parser->state = STATE_WAIT_VERSION;
                }else if(b == PACKET_MAGIC1){
                    // in case of recovery
                    parser->calculated_crc = b;
                }else{
                    reset_parser(parser);
                }
                break;
            case STATE_WAIT_VERSION:
                parser->current_packet.version = b;
                parser->state = STATE_WAIT_TYPE;
                break;
            case STATE_WAIT_TYPE:
                parser->current_packet.type = b;
                parser->state = STATE_WAIT_LEN_H;
                break;
            case STATE_WAIT_LEN_H:
                // big endian
                parser->current_packet.length = (uint16_t)(b<<8);
                parser->state = STATE_WAIT_LEN_L;
                break;
            case STATE_WAIT_LEN_L:
                // big endian
                parser->current_packet.length |= (uint16_t)(b);
                
                // bounds check
                if(parser->current_packet.length > PACKET_MAX_PAYLOAD){
                    parser->oversize_errors++;
                    reset_parser(parser);

                    if(b == PACKET_MAGIC1){
                        parser->state = STATE_WAIT_MAGIC2;
                        parser->calculated_crc = b;
                    }else if(parser->current_packet.length == 0){
                        parser->state = STATE_WAIT_CRC;
                    }else{
                        parser->state = STATE_WAIT_LEN_PAYLOAD;
                        parser->payload_bytes_received = 0;
                    }
                    break;
                }
            case STATE_WAIT_LEN_PAYLOAD:
                parser->current_packet.payload[parser->payload_bytes_received++] = b;
                if(parser->payload_bytes_received == parser->current_packet.length){
                    parser->state = STATE_WAIT_CRC;
                }
                break;
            case STATE_WAIT_CRC:
                parser->current_packet.crc = b;

                // validation
                if(b == parser->calculated_crc){
                    parser->valid_packets++;
                    if(handler){
                        // trigger callback
                        handler(&parser->current_packet, user_context);
                    }
                }else{
                    parser->crc_errors++;
                }
                reset_parser(parser);

                // recovery 
                if(b == PACKET_MAGIC1){
                    parser->state = STATE_WAIT_MAGIC2;
                    parser->calculated_crc = b;
                }
                break;
        }
    }
}