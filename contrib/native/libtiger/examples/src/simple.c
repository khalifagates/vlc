/*
  */

#include <stdio.h>
#include <stdlib.h>
#include <ogg/ogg.h>
#include <kate/kate.h>
#include "common.h"

/* All the libtiger API is available from the main tiger header file: */

#include <tiger/tiger.h>

#define FPS ((kate_float)25.0)

static void step_render(tiger_renderer *tr,kate_float *t,kate_float t1)
{
  for (;*t<=t1;*t+=(kate_float)1.0/FPS) {
    tiger_renderer_update(tr,*t,1);
    if (tiger_renderer_is_dirty(tr)) {
      tiger_renderer_render(tr);
    }
  }
}

int main()
{
  ogg_sync_state oy;
  ogg_stream_state os;
  int init=0;
  ogg_packet op;
  kate_state k;
  const kate_event *ev;
  kate_packet kp;
  unsigned char *image;
  tiger_renderer *tr;
  kate_float render_time=(kate_float)0;
  kate_float end_time=(kate_float)0;

  set_binary_file(stdin);

  /* create a buffer to draw to */
  image=(unsigned char*)malloc(640*480*4);

  /* we initialize ogg, kate, and tiger */
  ogg_sync_init(&oy);
  kate_high_decode_init(&k);
  tiger_renderer_create(&tr);
  tiger_renderer_set_buffer(tr,image,640,480,640*4,0);
  tiger_renderer_set_surface_clear_color(tr,1,0,0,0,0);

  /*
    We then read packets and feed them to the libkate high level API. When
    kate_high_decode_packetin returns a positive number, this signals the
    end of the stream.
    */
  while (1) {
    if (get_packet(&oy,&os,&init,&op)) break;
    kate_packet_wrap(&kp,op.bytes,op.packet);
    if (kate_high_decode_packetin(&k,&kp,&ev)>0) break;
    /* if the event is non NULL, we have an event */
    if (ev) {
      step_render(tr,&render_time,ev->start_time);
      if (ev->end_time>end_time) end_time=ev->end_time;
      tiger_renderer_add_event(tr,k.ki,ev);
    }
  }
  step_render(tr,&render_time,end_time);

  /* That's it, we can now cleanup */
  tiger_renderer_destroy(tr);
  free(image);

  ogg_stream_clear(&os);
  ogg_sync_clear(&oy);

  kate_high_decode_clear(&k);

  return 0;
}

