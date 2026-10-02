/**
 * Tony Givargis
 * Copyright (C), 2023-2026
 * University of California, Irvine
 *
 * CS 238P - Operating Systems
 * jitc.c
 */

 #define _GNU_SOURCE

 #include <sys/types.h>
 #include <sys/wait.h>
 #include <unistd.h>
 #include <dlfcn.h>
 #include "system.h"
 #include "jitc.h"

 /**
  * Needs:
  *   fork()
  *   execv()
  *   waitpid()
  *   WIFEXITED()
  *   WEXITSTATUS()
  *   dlopen()
  *   dlclose()
  *   dlsym()
  */

 /* research the above Needed API and design accordingly */

 struct jitc {
     void *handle;
 };

 int
 jitc_compile(const char *input, const char *output)
 {
     char *argv[16];
     pid_t pid, child;
     int status;

     assert( safe_strlen(input) );
     assert( safe_strlen(output) );

     child = fork();
     if (0 > child) {
         TRACE("fork()");
         return -1;
     }
     if (0 == child) {
         argv[0] = "/usr/bin/gcc";
         argv[1] = "-ansi";
         argv[2] = "-pedantic";
         argv[3] = "-Wall";
         argv[4] = "-Wextra";
         argv[5] = "-Werror";
         argv[6] = "-Wfatal-errors";
         argv[7] = "-fpic";
         argv[8] = "-O3";
         argv[9] = "-shared";
         argv[10] = "-o";
         argv[11] = (char *)output;
         argv[12] = (char *)input;
         argv[13] = NULL;
         execv(argv[0], argv);
         TRACE("execv()");
         _exit(127);
     }
     do {
         pid = waitpid(child, &status, 0);
     } while ((0 > pid) && (EINTR == errno));
     if (0 > pid) {
         TRACE("waitpid()");
         return -1;
     }
     if (!WIFEXITED(status) || WEXITSTATUS(status)) {
         file_delete(output);
         TRACE("gcc");
         return -1;
     }
     return 0;
 }

 struct jitc *
 jitc_open(const char *pathname)
 {
     struct jitc *jitc;
     char buf[4096];
     const char *path;
     void *handle;

     assert( safe_strlen(pathname) );

     path = pathname;
     handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
     if (!handle && !strchr(pathname, '/')) {
         safe_sprintf(buf, sizeof (buf), "./%s", pathname);
         path = buf;
         handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
     }
     if (!handle) {
         TRACE(dlerror());
         return NULL;
     }
     if (!(jitc = malloc(sizeof (struct jitc)))) {
         TRACE("out of memory");
         dlclose(handle);
         return NULL;
     }
     memset(jitc, 0, sizeof (struct jitc));
     jitc->handle = handle;
     return jitc;
 }

 void
 jitc_close(struct jitc *jitc)
 {
     if (jitc) {
         if (jitc->handle) {
             dlclose(jitc->handle);
         }
         memset(jitc, 0, sizeof (struct jitc));
     }
     FREE(jitc);
 }

 long
 jitc_lookup(struct jitc *jitc, const char *symbol)
 {
     const char *err;
     void *addr;

     assert( jitc );
     assert( jitc->handle );
     assert( safe_strlen(symbol) );

     dlerror();
     addr = dlsym(jitc->handle, symbol);
     err = dlerror();
     if (err || !addr) {
         TRACE(err);
         return 0;
     }
     return (long)(intptr_t)addr;
 }
