#ifndef _KATE_EXAMPLES_COMMON_H_
#define _KATE_EXAMPLES_COMMON_H_

#include <stdio.h>
#include <ogg/ogg.h>

int get_packet(ogg_sync_state *oy,ogg_stream_state *os,int *init,ogg_packet *op);
void set_binary_file(FILE *f);

#endif

