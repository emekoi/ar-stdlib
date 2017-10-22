/**
 * Copyright (c) 2017 emekoi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */


#include <stdlib.h>
#include <aria/aria.h>
#include "dyad/dyad.h"

#define UNUSED(x) ((void) x)

static ar_State *ar_net_state;
static ar_Value *ar_net_panic;

static ar_Value *ar_net_update(ar_State *S, ar_Value *args) {
  /* update dyad */
  UNUSED(S); UNUSED(args);
  dyad_update();
  return S->t;
}


static ar_Value *ar_net_getTime(ar_State *S, ar_Value *args) {
  UNUSED(args);
  return ar_new_number(S, dyad_getTime());
}


static ar_Value *ar_net_getStreamCount(ar_State *S, ar_Value *args) {
  UNUSED(args);
  return ar_new_number(S, dyad_getStreamCount());
}


static ar_Value *ar_net_setUpdateTimeout(ar_State *S, ar_Value *args) {
  dyad_setUpdateTimeout(ar_check_number(S, ar_nth(args, 0)));
  return S->t;
}


static ar_Value *ar_net_setTickInterval(ar_State *S, ar_Value *args) {
  dyad_setTickInterval(ar_check_number(S, ar_nth(args, 0)));
  return S->t;
}


static void panic(const char *message) {
  ar_call_global(ar_net_state, "net-panic",
    ar_new_string(ar_net_state, message));
}


static ar_Value *ar_net_setPanic(ar_State *S, ar_Value *args) {
  ar_net_panic = ar_check(S, ar_nth(args, 0), AR_TFUNC);
  return ar_net_panic;
}


static ar_Value *ar_net_stream_gc(ar_State *S, ar_Value *args) {
  dyad_end(ar_check_udata(S, ar_nth(args, 0)));
  return NULL;
}


static ar_Value *ar_net_stream_new(ar_State *S, ar_Value *args) {
  return ar_new_udata(S, dyad_newStream(), ar_net_stream_gc, NULL);
}


static ar_Value *ar_net_stream_end(ar_State *S, ar_Value *args) {
  dyad_end(ar_check_udata(S, ar_nth(args, 0)));
  return S->t;
}


static ar_Value *ar_net_stream_close(ar_State *S, ar_Value *args) {
  dyad_close(ar_check_udata(S, ar_nth(args, 0)));
  return S->t;
}


static ar_Value *ar_net_stream_listen(ar_State *S, ar_Value *args) {
  dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
  const char *host = ar_nth(args, 1) ? ar_check_string(S, ar_nth(args, 1)) : NULL;
  int port = ar_check_number(S, ar_nth(args, 2));
  int back = ar_nth(args, 3) ? ar_check_number(S, ar_nth(args, 3)) : 511;
  int res = dyad_listenEx(s, host, port, back);
  return res == 0 ? S->t : NULL;
}


static ar_Value *ar_net_stream_connect(ar_State *S, ar_Value *args) {
  dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
  const char *host = ar_check_string(S, ar_nth(args, 1));
  int port = ar_check_number(S, ar_nth(args, 2));
  int res = dyad_connect(s, host, port);
  return res == 0 ? S->t : NULL;
}


// static ar_Value *ar_net_stream_write(ar_State *S, ar_Value *args){
//   dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
//   dyad_write(s, const void *data, int size);
//   return S->t;
// }


// static ar_Value *ar_net_stream_vwritef(ar_State *S, ar_Value *args){
//   dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
//   dyad_vwritef(s, const char *fmt, va_list args);
//   return S->t;
// }


// static ar_Value *ar_net_stream_writef(ar_State *S, ar_Value *args){
//   dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
//   dyad_writef(s, const char *fmt, ...);
//   return S->t;
// }


static ar_Value *ar_net_stream_setTimeout(ar_State *S, ar_Value *args){
  dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
  dyad_setTimeout(s, ar_check_number(S, ar_nth(args, 1)));
  return S->t;
}


static ar_Value *ar_net_stream_setNoDelay(ar_State *S, ar_Value *args){
  dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
  dyad_setNoDelay(s, ar_nth(args, 2) == S->t);
  return S->t;
}


static ar_Value *ar_net_stream_getState(ar_State *S, ar_Value *args) {
  dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
  switch (dyad_getState(s)) {
    case DYAD_STATE_CLOSED:     return ar_new_string(S, "closed");
    case DYAD_STATE_CLOSING:    return ar_new_string(S, "closing");
    case DYAD_STATE_CONNECTING: return ar_new_string(S, "connecting");
    case DYAD_STATE_CONNECTED:  return ar_new_string(S, "connected");
    case DYAD_STATE_LISTENING:  return ar_new_string(S, "listening");
  }
  return ar_new_string(S, "?");
}


static ar_Value *ar_net_stream_getAddress(ar_State *S, ar_Value *args) {
  dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
  return ar_new_string(S, dyad_getAddress(s));
}


static ar_Value *ar_net_stream_getPort(ar_State *S, ar_Value *args) {
  dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
  return ar_new_number(S, dyad_getPort(s));
}


static ar_Value *ar_net_stream_getBytesSent(ar_State *S, ar_Value *args) {
  dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
  return ar_new_number(S, dyad_getBytesSent(s));
}


static ar_Value *ar_net_stream_getBytesReceived(ar_State *S, ar_Value *args) {
  dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
  return ar_new_number(S, dyad_getBytesReceived(s));
}




ar_Value *ar_open_net(ar_State *S, ar_Value* args) {
  UNUSED(args);
  ar_net_state = S;
  /* list of functions to register */
  struct { const char *name; ar_CFunc fn; } funcs[] = {
    { "net-update",             ar_net_update                    },
    { "net-getTime",            ar_net_getTime                   },
    { "net-getStreamCount",     ar_net_getStreamCount            },
    { "net-setUpdateTimeout",   ar_net_setUpdateTimeout          },
    { "net-setTickInterval",    ar_net_setTickInterval           },
    { "net-setPanic",           ar_net_setPanic                  },
    { "net-newStream",          ar_net_stream_new                },
    { "net-listen",             ar_net_stream_listen             },
    { "net-connect",            ar_net_stream_connect            },
    // { "net-addListener",        ar_net_stream_addListener        },
    // { "net-removeListener",     ar_net_stream_removeListener     },
    // { "net-removeAllListeners", ar_net_stream_removeAllListeners },
    { "net-end",                ar_net_stream_end                },
    { "net-close",              ar_net_stream_close              },
    // { "net-write",              ar_net_stream_write              },
    // { "net-vwritef",            ar_net_stream_vwritef            },
    // { "net-writef",             ar_net_stream_vwritef            },
    { "net-setTimeout",         ar_net_stream_setTimeout         },
    { "net-setNoDelay",         ar_net_stream_setNoDelay         },
    { "net-getState",           ar_net_stream_getState           },
    { "net-getAddress",         ar_net_stream_getAddress         },
    { "net-getPort",            ar_net_stream_getPort            },
    { "net-getBytesSent",       ar_net_stream_getBytesSent       },
    { "net-getBytesReceived",   ar_net_stream_getBytesReceived   },
    { NULL, NULL }
  };

  /* register functions */
  for (int i = 0; funcs[i].name; i++) {
    ar_bind_global(S, funcs[i].name, ar_new_cfunc(S, funcs[i].fn));
  }

  /* initialize dyad for the user */
  dyad_init();
  /* set the panic handler */
  ar_eval(S, ar_parse(S,
    "(= net-panic (fn (err) (print err) (exit)))",
    "net_panic"), S->global);
  dyad_atPanic(panic);
  /* run dyad_shutdown when the user is done */
  atexit(dyad_shutdown);
  return NULL;
}
