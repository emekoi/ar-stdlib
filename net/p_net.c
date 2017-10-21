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
#define AR_GET_ARG(idx, type) S, ar_check(S, ar_nth(args, idx), type)
#define AR_GET_STRING(idx) (char *)ar_to_string(AR_GET_ARG(idx, AR_TSTRING))
#define AR_GET_STRINGL(idx, len) (char *)ar_to_stringl(AR_GET_ARG(idx, AR_TSTRING), &len)

static ar_State *ar_net_state;
static ar_Value *ar_net_panic;

static ar_Value *ar_net_update(ar_State *S, ar_Value *args) {
  /* update dyad */
  UNUSED(S); UNUSED(args);
  dyad_update();
  return NULL;
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
  dyad_setUpdateTimeout(ar_to_number(AR_GET_ARG(0, AR_TNUMBER)));
  return NULL;
}


static ar_Value *ar_net_setTickInterval(ar_State *S, ar_Value *args) {
  dyad_setTickInterval(ar_to_number(AR_GET_ARG(0, AR_TNUMBER)));
  return NULL;
}


static void panic(const char *message) {
  ar_call_global(ar_net_state, "net-panic",
    ar_new_string(ar_net_state, message));
}


static ar_Value *ar_net_setPanic(ar_State *S, ar_Value *args) {
  ar_net_panic = ar_check(S, ar_nth(args, 0), AR_TFUNC);
  return NULL;
}


static ar_Value *ar_net_stream_gc(ar_State *S, ar_Value *args) {
  dyad_end(ar_check_udata(S, ar_nth(args, 0)));
  return NULL;
}


static ar_Value *ar_net_stream_new(ar_State *S, ar_Value *args) {
  dyad_Stream *s = dyad_newStream();
  return ar_new_udata(S, s, ar_net_stream_gc, NULL);
}


static ar_Value *ar_net_stream_listen(ar_State *S, ar_Value *args) {
  dyad_listen(ar_check_udata(S, ar_nth(args, 0)), ar_check_number(S, ar_nth(args, 1)));
  return S->t;
}


ar_Value *ar_open_net(ar_State *S, ar_Value* args) {
  UNUSED(args);
  ar_net_state = S;
  /* list of functions to register */
  struct { const char *name; ar_CFunc fn; } funcs[] = {
    { "net-update",           ar_net_update           },
    { "net-getTime",          ar_net_getTime          },
    { "net-getStreamCount",   ar_net_getStreamCount   },
    { "net-setUpdateTimeout", ar_net_setUpdateTimeout },
    { "net-setTickInterval",  ar_net_setTickInterval  },
    { "net-setPanic",         ar_net_setPanic         },

    { "net-newStream",        ar_net_stream_new },
    { "net-listen",        ar_net_stream_listen },


    { "net-update",        ar_net_update },
    { "net-update",        ar_net_update },
    { "net-update",        ar_net_update },
    { "net-update",        ar_net_update },
    { "net-update",        ar_net_update },
    { "net-update",        ar_net_update },

  //   { "fs-unmount",      ar_fs_unmount      },
  //   { "fs-setWritePath", ar_fs_setWritePath },
  //   { "fs-exists",       ar_fs_exists       },
  //   { "fs-getSize",      ar_fs_getSize      },
  //   { "fs-getModified",  ar_fs_getModified  },
  //   { "fs-read",         ar_fs_read         },
  //   { "fs-isDir",        ar_fs_isDir        },
  //   { "fs-isFile",       ar_fs_isFile       },
  //   { "fs-listDir",      ar_fs_listDir      },
  //   { "fs-write",        ar_fs_write        },
  //   { "fs-append",       ar_fs_append       },
  //   { "fs-delete",       ar_fs_delete       },
  //   { "fs-makeDirs",     ar_fs_makeDirs     },
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
