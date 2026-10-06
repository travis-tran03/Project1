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

 /* Wraps the dlopen handle for a loaded JIT-compiled shared library. Callers
    only see a pointer to this struct, not its contents. */
 struct jitc {
     void *handle;
 };

 /* Compiles a C source file into a shared library by forking a child process
    that runs gcc via execv, then waits for it to finish. Returns 0 on success,
    -1 on failure. */
 int
 jitc_compile(const char *input, const char *output)
 {
     char *argv[8];
     pid_t pid, child;
     int status;

     /* Fork a child process, because execv replaces the calling process and
        would never return to this program. */
     child = fork();
     if (0 > child) {
         TRACE("fork()");
         return -1;
     }
     if (0 == child) {
         /* gcc -fpic -shared -o output input. NULL ends the argument list. */
         argv[0] = "/usr/bin/gcc";
         argv[1] = "-fpic";
         argv[2] = "-shared";
         argv[3] = "-o";
         argv[4] = (char *)output;
         argv[5] = (char *)input;
         argv[6] = NULL;
         /* Replace the child process with gcc. This only returns if exec
            fails, in which case we exit with status 127. */
         execv(argv[0], argv);
         TRACE("execv()");
         _exit(127);
     }
     /* Parent waits for the child to finish compiling. */
     pid = waitpid(child, &status, 0);
     if (0 > pid) {
         TRACE("waitpid()");
         return -1;
     }
     /* Make sure gcc exited normally with status 0. */
     if (!WIFEXITED(status) || WEXITSTATUS(status)) {
         TRACE("gcc");
         return -1;
     }
     return 0;
 }

 /* Loads a compiled shared library from disk using dlopen and returns a handle.
    If the name has no slash, it retries with "./" prepended so it is found in
    the current directory. */
 struct jitc *
 jitc_open(const char *pathname)
 {
     struct jitc *jitc;
     char buf[4096];
     const char *path;
     void *handle;

     path = pathname;
     /* load the shared library */
     handle = dlopen(path, RTLD_NOW);
     /* retry with ./ if no slash in name */
     if (!handle && !strchr(pathname, '/')) {
         safe_sprintf(buf, sizeof (buf), "./%s", pathname);
         path = buf;
         handle = dlopen(path, RTLD_NOW);
     }
     /* library failed to load */
     if (!handle) {
         TRACE(dlerror());
         return NULL;
     }
     /* allocate the wrapper struct */
     if (!(jitc = malloc(sizeof (struct jitc)))) {
         TRACE("out of memory");
         dlclose(handle);
         return NULL;
     }
     /* store the library handle */
     jitc->handle = handle;
     return jitc;
 }

 /* Unloads the shared library with dlclose and frees the struct. */
 void
 jitc_close(struct jitc *jitc)
 {
     if (jitc) {
         /* unload the library */
         dlclose(jitc->handle);
     }
     FREE(jitc);
 }

 /* Looks up a named symbol (usually a function) in the loaded library using
    dlsym and returns its address as a long. Returns 0 if not found. */
 long
 jitc_lookup(struct jitc *jitc, const char *symbol)
 {
     void *addr;

     /* find the symbol's address */
     addr = dlsym(jitc->handle, symbol);
     /* symbol not found */
     if (!addr) {
         TRACE(dlerror());
         return 0;
     }
     return (long)(intptr_t)addr;
 }
