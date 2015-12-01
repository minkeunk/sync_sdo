#include <glob.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

pid_t get_pid_by_name(char *process_name)
{

    pid_t pid = -1;
    glob_t pglob;
    char *procname, *readbuf;
    int buflen = strlen(process_name) + 2;
    unsigned i;


    buflen = strlen(process_name) + 2;

    // Get a list of all comm files. man 5 proc
    
    if (glob("/proc/*/comm", 0, NULL, &pglob) != 0)
       return pid;
    // The comm files include trailing newlines, so...
    
    procname = (char*)malloc(buflen);
    
    strcpy(procname, process_name);
    procname[buflen - 2] = '\n';
    procname[buflen - 1] = 0;
    
    // readbuff will hold the contents of the comm files.
    
    readbuf = (char*)malloc(buflen);
    for (i = 0; i < pglob.gl_pathc; ++i) {
        FILE *comm;
        char *ret;
     // Read the contents of the file.
    
        if ((comm = fopen(pglob.gl_pathv[i], "r")) == NULL)
            continue;

        ret = fgets(readbuf, buflen, comm);
        fclose(comm);
        if (ret == NULL)
            continue;
    //If comm matches our process name, extract the process ID from the
    //path, convert it to a pid_t, and return it.
        if (strcmp(readbuf, procname) == 0) {
            pid = (pid_t)atoi(pglob.gl_pathv[i] + strlen("/proc/"));
    
        if (pid == getpid())
            continue;
        break;
    
        }
    
    }
    
    // // Clean up.
    free(procname);
    free(readbuf);
    globfree(&pglob);
    if (pid == 0)
       return -1;
    
    return pid;
}
