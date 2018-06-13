#include <stddef.h>
#include <stdio.h>
#include <string.h>

#if defined WIN32 || defined _WIN32 || defined MSDOS || defined __CYGWIN__ || defined __EMX__ || defined OS2
#include <io.h>
#include <fcntl.h>
#endif

#include <ogg/ogg.h>

#include "common.h"

/*
   This function returns the next packet, reading data as necessary
  */

int get_packet(ogg_sync_state *oy,ogg_stream_state *os,int *init,ogg_packet *op)
{
  char *buffer;
  size_t bytes;
  ogg_page og;

  /* try to get a packet from the stream */
  if (*init && ogg_stream_packetout(os,op)) return 0;

  /* read data and feed the pages to ogg */
  buffer=ogg_sync_buffer(oy,4096);
  bytes=fread(buffer,1,4096,stdin);
  if (bytes==0) return 1; /* we're done */
  ogg_sync_wrote(oy,bytes);
  while (ogg_sync_pageout(oy,&og)>0) {
    if (!*init && ogg_page_bos(&og)) {
      ogg_stream_init(os,ogg_page_serialno(&og));
    }
    ogg_stream_pagein(os,&og);
    if (!*init && ogg_page_bos(&og)) {
      ogg_packet op;
      ogg_stream_packetpeek(os,&op);
      if (op.bytes>=8 && !memcmp(op.packet,"\200kate\0\0\0",8)) {
        *init=1;
      }
      else {
        ogg_stream_clear(os);
      }
    }
  }

  /* try again with the new data */
  return get_packet(oy,os,init,op);
}

void set_binary_file(FILE *f)
{
#if defined WIN32 || defined _WIN32
  _setmode(_fileno(f),_O_BINARY);
#else
#if defined MSDOS || defined __CYGWIN__ || defined __EMX__ || defined OS2
    setmode(fileno(f),_O_BINARY);
#endif
#endif
}

