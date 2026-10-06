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
     char *argv[16];
     pid_t pid, child;
     int status;

     assert( safe_strlen(input) );
     assert( safe_strlen(output) );

     /* Fork a child process, because execv replaces the calling process and
        would never return to this program. */
     child = fork();
     if (0 > child) {
         TRACE("fork()");
         return -1;
     }
     if (0 == child) {
         /* Build the gcc command line (compiler path, warning and optimization
            flags, -shared, -o output, input file), terminated by NULL. */
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
         /* Replace the child process with gcc. This only returns if exec
            fails, in which case we exit with status 127. */
         execv(argv[0], argv);
         TRACE("execv()");
         _exit(127);
     }
     /* Parent waits for the child to finish compiling, retrying if interrupted
        by a signal (EINTR). */
     do {
         pid = waitpid(child, &status, 0);
     } while ((0 > pid) && (EINTR == errno));
     if (0 > pid) {
         TRACE("waitpid()");
         return -1;
     }
     /* Make sure gcc exited normally with status 0. If not, delete the output
        file and report failure. */
     if (!WIFEXITED(status) || WEXITSTATUS(status)) {
         file_delete(output);
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

     assert( safe_strlen(pathname) );

     path = pathname;
     /* load the shared library */
     handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
     /* retry with ./ if no slash in name */
     if (!handle && !strchr(pathname, '/')) {
         safe_sprintf(buf, sizeof (buf), "./%s", pathname);
         path = buf;
         handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
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
     memset(jitc, 0, sizeof (struct jitc));
     /* store the library handle */
     jitc->handle = handle;
     return jitc;
 }

 /* Unloads the shared library with dlclose and frees the struct. */
 void
 jitc_close(struct jitc *jitc)
 {
     if (jitc) {
         if (jitc->handle) {
             /* unload the library */
             dlclose(jitc->handle);
         }
         memset(jitc, 0, sizeof (struct jitc));
     }
     FREE(jitc);
 }

 /* Looks up a named symbol (usually a function) in the loaded library using
    dlsym and returns its address as a long. Returns 0 if not found. */
 long
 jitc_lookup(struct jitc *jitc, const char *symbol)
 {
     const char *err;
     void *addr;

     assert( jitc );
     assert( jitc->handle );
     assert( safe_strlen(symbol) );

     /* clear any old error */
     dlerror();
     /* find the symbol's address */
     addr = dlsym(jitc->handle, symbol);
     err = dlerror();
     /* symbol not found */
     if (err || !addr) {
         TRACE(err);
         return 0;
     }
     return (long)(intptr_t)addr;
 }
