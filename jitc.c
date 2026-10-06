/**
 * Tony Givargis
 * Copyright (C), 2023-2026
 * University of California, Irvine
 *
 * CS 238P - Operating Systems
 * jitc.c
 */

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
    void *opaque_handle;
};

struct jitc *
jitc_open(const char *pathname)
{
    struct jitc *my_jitc;
    void* my_handle;
    char* err_msg;
 
    assert( safe_strlen(pathname) );
 
    if (!(my_jitc = malloc(sizeof (struct jitc)))) {
        TRACE("out of memory");
        return NULL;
    }
    memset(my_jitc, 0, sizeof (struct jitc));

    my_handle = dlopen(pathname, RTLD_LAZY);

    if (my_handle == NULL){
        err_msg = dlerror();
        TRACE(err_msg);
        FREE(my_jitc);
        return NULL;
    }

    my_jitc->opaque_handle = my_handle;
    return my_jitc;
}

void
jitc_close(struct jitc *jitc)
{
    if (jitc) {
        if (jitc -> opaque_handle) {
           dlclose(jitc->opaque_handle);
        }

        memset(jitc, 0, sizeof (struct jitc));
    }
    FREE(jitc);
}

long
jitc_lookup(struct jitc *jitc, const char *symbol)
{
    void *code_address;
    assert( safe_strlen(symbol) );
    assert( jitc );

    code_address = dlsym(jitc->opaque_handle, symbol);

    if (code_address == NULL){
        char* err_msg = dlerror();
        TRACE(err_msg);
        return 0;
    }
    return (long) code_address;
}

int
jitc_compile(const char *input, const char *output)
{
    

    pid_t child;
    pid_t pid;
    char *args[7];
    int status;

    assert( safe_strlen(input) );
    assert( safe_strlen(output) );

    args[0] = "/usr/bin/gcc";
    args[1] = "-fpic";
    args[2] = "-shared";
    args[3] = "-o";
    args[4] = (char *)output;
    args[5] = (char *)input;
    args[6] = NULL;


    child = fork();

    if (child < 0){
        TRACE("fork error");
        return -1;
    }

    if (child == 0){
        execv(args[0], args);
        TRACE("execv()");
        _exit(127);
    }

    do {
        pid = waitpid(child, &status, 0);
    } while ((pid < 0) && (EINTR == errno));

    if (pid < 0) {
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
