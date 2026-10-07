#include "packet_parser.h"

#include <string>

void packet_parser_init(packet_parser_t *parser){
    if(!parser) return;

    memset(parser,0,sizeof(packet_parser_t));
    parser->state = STATE_WAIT_MAGIC1;
} 

