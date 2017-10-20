/**
 * Copyright (c) 2017 emekoi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */


#include <string.h>
#include <stdlib.h>
#include <aria/aria.h>

#define UNUSED(x) ((void) x)

static ar_Value *os_system(ar_State *S, ar_Value *args) {
  char *command = (char *)ar_to_string(S, ar_check(S, ar_nth(args, 0), AR_TSTRING));
  return ar_new_number(S, (double)system(command));
}


static void *read_stream(FILE *fp, size_t *len) {
  size_t len_ = 0;
  if (!len) len = &len_;
  if (!fp) goto end;
  /* Get file size */
  fseek(fp, 0, SEEK_END);
  *len = ftell(fp);
  /* Load file */
  fseek(fp, 0, SEEK_SET);
  char *res = malloc(*len + 1);
  if (!res) return NULL;
  res[*len] = '\0';
  if (fread(res, 1, *len, fp) != *len) {
    free(res); return NULL;
  } else return res;
  end:
    return NULL;
}


static ar_Value *os_popen(ar_State *S, ar_Value *args) {
  char *command = (char *)ar_to_string(S, ar_check(S, ar_nth(args, 0), AR_TSTRING));
  char *mode = (char *)ar_to_string(S, ar_check(S, ar_nth(args, 1), AR_TSTRING));

  if (!(strcmp(mode, "w") == 0 || strcmp(mode, "r") == 0))
    ar_error_str(S, "unknown mode %s", mode);

  FILE *fp = popen(command, mode);
  if (!fp) ar_error_str(S, "could not open pipe for %s", command);

  if (!strcmp(mode, "r")) {
    size_t len = 0; char *data = read_stream(stdout, &len);
    ar_Value *res = ar_new_string(S, data);
    pclose(fp); return res;
  } else {
    size_t len = 0;
    char *data = (char *)ar_to_stringl(S, ar_check(S, ar_nth(args, 2), AR_TSTRING), &len);
    int res = fwrite(data, strlen(data), 1, fp); pclose(fp);
    if (res == -1) ar_error_str(S, "error writing to pipe");
    return NULL;
  }
}


ar_Value *ar_open_os(ar_State *S, ar_Value* args) {
	// UNUSED(args);
  /* list of functions to register */
  struct { const char *name; ar_CFunc fn; } funcs[] = {
		{ "os-system", os_system },
		{ "os-popen",  os_popen  },
    { NULL, NULL }
  };

  /* register functions */
  for (int i = 0; funcs[i].name; i++) {
    ar_bind_global(S, funcs[i].name, ar_new_cfunc(S, funcs[i].fn));
  }

	return NULL;
}
