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

static const char *EVENT[] = {
  "null",
  "destroy",
  "accept",
  "listen",
  "connect",
  "close",
  "ready",
  "data",
  "line",
  "error",
  "timeout",
  "tick"
}; 


static ar_State *ar_net_state;
static ar_Value *ar_net_panic;
static ar_Value *ar_net_cb_env;


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


static void net_panic(const char *message) {
  ar_call_global(ar_net_state, "net-panic",
    ar_new_string(ar_net_state, message));
}


static ar_Value *ar_net_setPanic(ar_State *S, ar_Value *args) {
  ar_net_panic = ar_check(S, ar_nth(args, 0), AR_TFUNC);
  return ar_net_panic;
}


static ar_Value *ar_net_stream_gc(ar_State *S, ar_Value *args) {
  dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
  if (s) dyad_end(s);
  return NULL;
}


static ar_Value *ar_net_stream_new(ar_State *S, ar_Value *args) {
  UNUSED(args);
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

#define ar_get_env(S,x,env)    ar_eval(S, ar_new_symbol(S, x), env)

static void onEvent(dyad_Event *e) {
  ar_State *S = ar_net_state;
  const char *event = EVENT[e->type];
  ar_Value *udata = e->udata;
  ar_Value *stream = ar_new_udata(S, e->stream, ar_net_stream_gc, NULL);
  ar_Value *remote = ar_new_udata(S, e->remote, ar_net_stream_gc, NULL);
  ar_Value *msg = ar_new_string(S, e->msg);
  ar_Value *data = ar_new_string(S, e->data);
  ar_Value *size = ar_new_number(S, e->size);
  ar_set(S, ar_new_symbol(S, "udata"),  udata,  ar_get_env(S, event, ar_net_cb_env)->u.func.env);
  ar_set(S, ar_new_symbol(S, "stream"), stream, ar_get_env(S, event, ar_net_cb_env)->u.func.env);
  ar_set(S, ar_new_symbol(S, "remote"), remote, ar_get_env(S, event, ar_net_cb_env)->u.func.env);
  ar_set(S, ar_new_symbol(S, "msg"),    msg,    ar_get_env(S, event, ar_net_cb_env)->u.func.env);
  ar_set(S, ar_new_symbol(S, "data"),   data,   ar_get_env(S, event, ar_net_cb_env)->u.func.env);
  ar_set(S, ar_new_symbol(S, "size"),   size,   ar_get_env(S, event, ar_net_cb_env)->u.func.env);
  ar_call(S, ar_get_env(S, event, ar_net_cb_env), NULL);
}

#define strcmp strcasecmp

static ar_Value *ar_net_stream_addListener(ar_State *S, ar_Value *args) {
  dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
  const char *event = ar_check_string(S, ar_nth(args, 1));
  ar_Value *func = ar_check(S, ar_nth(args, 2), AR_TFUNC);
  ar_Value *udata = ar_nth(args, 3);
  size_t type = -1;
  if (!strcmp(event, "destroy"))      type = DYAD_EVENT_DESTROY;
  else if (!strcmp(event, "accept"))  type = DYAD_EVENT_ACCEPT;
  else if (!strcmp(event, "listen"))  type = DYAD_EVENT_LISTEN;
  else if (!strcmp(event, "connect")) type = DYAD_EVENT_CONNECT;
  else if (!strcmp(event, "close"))   type = DYAD_EVENT_CLOSE;
  else if (!strcmp(event, "ready"))   type = DYAD_EVENT_READY;
  else if (!strcmp(event, "data"))    type = DYAD_EVENT_DATA;
  else if (!strcmp(event, "line"))    type = DYAD_EVENT_LINE;
  else if (!strcmp(event, "error"))   type = DYAD_EVENT_ERROR;
  else if (!strcmp(event, "timeout")) type = DYAD_EVENT_TIMEOUT;
  else if (!strcmp(event, "tick"))    type = DYAD_EVENT_TICK;
  else ar_error_str(S, "unkown event '%s'", event);
  /* register callback in env */
  ar_set(S, ar_new_symbol(S, event), func, ar_net_cb_env);
  /* add our listener function */
  dyad_addListener(s, type, onEvent, udata);
  return S->t;
}


static ar_Value *ar_net_stream_removeListener(ar_State *S, ar_Value *args) {
  dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
  const char *event = ar_check_string(S, ar_nth(args, 1));
  ar_Value *udata = ar_nth(args, 2);
  size_t type = -1;
  if (!strcmp(event, "destroy"))      type = DYAD_EVENT_DESTROY;
  else if (!strcmp(event, "accept"))  type = DYAD_EVENT_ACCEPT;
  else if (!strcmp(event, "listen"))  type = DYAD_EVENT_LISTEN;
  else if (!strcmp(event, "connect")) type = DYAD_EVENT_CONNECT;
  else if (!strcmp(event, "close"))   type = DYAD_EVENT_CLOSE;
  else if (!strcmp(event, "ready"))   type = DYAD_EVENT_READY;
  else if (!strcmp(event, "data"))    type = DYAD_EVENT_DATA;
  else if (!strcmp(event, "line"))    type = DYAD_EVENT_LINE;
  else if (!strcmp(event, "error"))   type = DYAD_EVENT_ERROR;
  else if (!strcmp(event, "timeout")) type = DYAD_EVENT_TIMEOUT;
  else if (!strcmp(event, "tick"))    type = DYAD_EVENT_TICK;
  else ar_error_str(S, "unkown event '%s'", event);
  /* remove the callback */
  ar_set(S, ar_new_symbol(S, event), NULL, ar_net_cb_env);
  /* remove the event listener */
  dyad_removeListener(s, type, onEvent, udata);
  return S->t;
}


static ar_Value *ar_net_stream_removeAllListeners(ar_State *S, ar_Value *args) {
  dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
  const char *event = ar_nth(args, 1) ? ar_check_string(S, ar_nth(args, 1)) : NULL;
  size_t type = -1;
  if (!event) {
    dyad_removeAllListeners(s, DYAD_EVENT_NULL);
    return S->t;
  }
  if (!strcmp(event, "destroy"))      type = DYAD_EVENT_DESTROY;
  else if (!strcmp(event, "accept"))  type = DYAD_EVENT_ACCEPT;
  else if (!strcmp(event, "listen"))  type = DYAD_EVENT_LISTEN;
  else if (!strcmp(event, "connect")) type = DYAD_EVENT_CONNECT;
  else if (!strcmp(event, "close"))   type = DYAD_EVENT_CLOSE;
  else if (!strcmp(event, "ready"))   type = DYAD_EVENT_READY;
  else if (!strcmp(event, "data"))    type = DYAD_EVENT_DATA;
  else if (!strcmp(event, "line"))    type = DYAD_EVENT_LINE;
  else if (!strcmp(event, "error"))   type = DYAD_EVENT_ERROR;
  else if (!strcmp(event, "timeout")) type = DYAD_EVENT_TIMEOUT;
  else if (!strcmp(event, "tick"))    type = DYAD_EVENT_TICK;
  else ar_error_str(S, "unkown event '%s'", event);
  dyad_removeAllListeners(s, type);
  return S->t;
}


static ar_Value *ar_net_stream_write(ar_State *S, ar_Value *args) {
  dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
  size_t len;
  const char *str = ar_to_stringl(S, ar_nth(args, 1), &len);
  int sz = ar_nth(args, 2) ? ar_to_number(S, ar_nth(args, 2)) : -1;
  dyad_write(s, str, sz > 0 ? (size_t)sz : len);
  return S->t;
}


static int is_alpha(char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         (c == '_');
}


#define format(S, c, v) \
  char buf[2]; buf[0] = '%'; buf[1] = c; \
  return ar_new_stringf(S, buf, v)

static ar_Value *parse_format(ar_State *S, const char c, ar_Value *args) {
  int num = round(ar_to_number(S, ar_car(args)));
  switch (c) {
    case 'c': case 'u': {
      ar_check_number(S, ar_car(args));
      format(S, c, (unsigned int)num);
    } case 'i': case 'd': case 'x':
    case 'X': {
      ar_check_number(S, ar_car(args));
      format(S, c, num);
    } case 'f': case 'g': {
      ar_check_number(S, ar_car(args));
      format(S, c, ar_to_number(S, ar_car(args)));
    } case 'p': {
      format(S, c, ar_car(args));
    } case 'q': {
      format(S, c, ar_to_string_value(S, ar_car(args), 1)->u.str.s);
    } case 's': {
      format(S, c, ar_to_string_value(S, ar_car(args), 0)->u.str.s);
    } default:
      if (is_alpha(c))
        ar_error_str(S, "invalid option '%c'", c);
      else
        ar_error_str(S, "expected option");
  }
  return NULL;
}


static ar_Value *join_list_of_strings(ar_State *S, ar_Value *list) {
  ar_Value *res;
  /* Get combined length of strings */
  ar_Value *v = list;
  size_t len = 0;
  while (v) {
    len += v->u.pair.car->u.str.len;
    v = v->u.pair.cdr;
  }
  /* Join list of strings */
  res = ar_new_stringl(S, NULL, len);
  v = list;
  len = 0;
  while (v) {
    ar_Value *x = v->u.pair.car;
    memcpy(res->u.str.s + len, x->u.str.s, x->u.str.len);
    len += x->u.str.len;
    v = v->u.pair.cdr;
  }
  return res;
}


static ar_Value *ar_net_stream_writef(ar_State *S, ar_Value *args) {
  dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
  size_t len;
  const char *str = ar_to_stringl(S, ar_nth(args, 1), &len);
  args = ar_cdr(args);
  const char *str_end = str + len;
  ar_Value *res = NULL, **last = &res;
  while (str < str_end) {
    if (*str != '%') {
      char buf[2]; buf[0] = *str++; buf[1] = '\0';
      last = ar_append_tail(S, last, ar_new_string(S, buf));
    } else if (*++str == '%') {
      char buf[2]; buf[0] = *str++; buf[1] = '\0';
      last = ar_append_tail(S, last, ar_new_string(S, buf));
    } else {
      last = ar_append_tail(S, last, parse_format(S, *str++, ar_cdr(args)));
      args = ar_cdr(args);
    }
  }
  dyad_writef(s, join_list_of_strings(S, res)->u.str.s);
  return S->t;
}


static ar_Value *ar_net_stream_setTimeout(ar_State *S, ar_Value *args) {
  dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
  dyad_setTimeout(s, ar_check_number(S, ar_nth(args, 1)));
  return S->t;
}


static ar_Value *ar_net_stream_setNoDelay(ar_State *S, ar_Value *args) {
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

/* event handlers */

// static void onDestroy(dyad_Event *e);
// static void onAccept(dyad_Event *e);
// static void onListen(dyad_Event *e);
// static void onConnect(dyad_Event *e);
// static void onClose(dyad_Event *e);
// static void onReady(dyad_Event *e);
// static void onData(dyad_Event *e);
// static void onLine(dyad_Event *e);
// static void onError(dyad_Event *e);
// static void onTimeout(dyad_Event *e);
// static void onTick(dyad_Event *e);

// static void onDestroy(dyad_Event *e) {
//   const char *address = dyad_getAddress(e->stream);
//   const size_t port = dyad_getPort(e->stream);
//   printf("destroyed: %s:%ld -> %s\n", address, port, e->msg);
// }

// static void onAccept(dyad_Event *e) {
//   dyad_addListener(e->remote, DYAD_EVENT_DATA, onData, NULL);
//   dyad_addListener(e->remote, DYAD_EVENT_LINE, onLine, NULL);
//   const char *address = dyad_getAddress(e->remote);
//   const size_t port = dyad_getPort(e->remote);
//   printf("accepted: %s:%ld -> %s\n", address, port, e->msg);
// }

// static void onListen(dyad_Event *e) {
//   const char *address = dyad_getAddress(e->stream);
//   const size_t port = dyad_getPort(e->stream);
//   printf("listening: %s:%ld -> %s\n", address, port, e->msg);
// }

// static void onConnect(dyad_Event *e) {
//   const char *address = dyad_getAddress(e->stream);
//   const size_t port = dyad_getPort(e->stream);
//   printf("connected: %s:%ld -> %s\n", address, port, e->msg);
// }

// static void onClose(dyad_Event *e) {
//   const char *address = dyad_getAddress(e->stream);
//   const size_t port = dyad_getPort(e->stream);
//   printf("closed: %s:%ld -> %s\n", address, port, e->msg);
// }

// static void onReady(dyad_Event *e) {
//   const char *address = dyad_getAddress(e->stream);
//   const size_t port = dyad_getPort(e->stream);
//   printf("ready: %s:%ld -> %s\n", address, port, e->msg);
// }

// static void onData(dyad_Event *e) {
//   const char *address = dyad_getAddress(e->stream);
//   const size_t port = dyad_getPort(e->stream);
//   printf("data: %s:%ld -> %s\n", address, port, e->data);
// }

// static void onLine(dyad_Event *e) {
//   const char *address = dyad_getAddress(e->stream);
//   const size_t port = dyad_getPort(e->stream);
//   printf("line: %s:%ld -> %s\n",  address, port, e->data);
// }

// static void onError(dyad_Event *e) {
//   const char *address = dyad_getAddress(e->stream);
//   const size_t port = dyad_getPort(e->stream);
//   printf("error: %s:%ld -> %s\n", address, port, e->msg);
// }

// static void onTimeout(dyad_Event *e) {
//   const char *address = dyad_getAddress(e->stream);
//   const size_t port = dyad_getPort(e->stream);
//   printf("timeout: %s:%ld -> %s\n", address, port, e->msg);
// }

// static void onTick(dyad_Event *e) {
//   const char *address = dyad_getAddress(e->stream);
//   const size_t port = dyad_getPort(e->stream);
//   printf("tick: %s:%ld -> %s\n", address, port, e->msg);
// }

// static ar_Value *ar_net_stream_register(ar_State *S, ar_Value *args) {
//   dyad_Stream *s = ar_check_udata(S, ar_nth(args, 0));
//   dyad_addListener(s, DYAD_EVENT_DESTROY, onDestroy, NULL);
//   dyad_addListener(s, DYAD_EVENT_ACCEPT,  onAccept,  NULL);
//   dyad_addListener(s, DYAD_EVENT_LISTEN,  onListen,  NULL);
//   dyad_addListener(s, DYAD_EVENT_CONNECT, onConnect, NULL);
//   dyad_addListener(s, DYAD_EVENT_CLOSE,   onClose,   NULL);
//   dyad_addListener(s, DYAD_EVENT_READY,   onReady,   NULL);
//   dyad_addListener(s, DYAD_EVENT_DATA,    onData,    NULL);
//   dyad_addListener(s, DYAD_EVENT_LINE,    onLine,    NULL);
//   dyad_addListener(s, DYAD_EVENT_ERROR,   onError,   NULL);
//   dyad_addListener(s, DYAD_EVENT_TIMEOUT, onTimeout, NULL);
//   dyad_addListener(s, DYAD_EVENT_TICK,    onTick,    NULL);
//   return S->t;
// }


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
//   {  "net-register",            ar_net_stream_register              },
    { "net-addListener",        ar_net_stream_addListener        },
    { "net-removeListener",     ar_net_stream_removeListener     },
    { "net-removeAllListeners", ar_net_stream_removeAllListeners },
    { "net-end",                ar_net_stream_end                },
    { "net-close",              ar_net_stream_close              },
    { "net-write",              ar_net_stream_write              },
    { "net-writef",             ar_net_stream_writef             },
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
  /* initialize env for callbacks */
  ar_net_cb_env = ar_new_env(S, NULL);
  /* initialize dyad for the user */
  dyad_init();
  /* set the panic handler */
  ar_eval(S, ar_parse(S,
    "(= net-panic (fn (err) (print err) (exit)))",
    "net_panic"), S->global);
  dyad_atPanic(net_panic);
  /* run dyad_shutdown when the user is done */
  atexit(dyad_shutdown);
  return NULL;
}
