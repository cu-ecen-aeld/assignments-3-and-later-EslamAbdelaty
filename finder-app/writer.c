#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    
    // open User logging 
    openlog("writer", LOG_CONS , LOG_USER);

    //Check input Argc 
    if(argc < 3 )
    {
        syslog(LOG_ERR , "invalid numeber of args! Usage: writer <filename> <string> ");
        closelog();
        return 1;
    }

    const char *filename = argv[1];
    const char *writeString = argv[2];

    syslog(LOG_DEBUG , "writing \" %s \"  to \" %s \" ", writeString , filename );

    //open file and check the returned handler 
    FILE *fHandler = fopen(filename , "w"); 
    if(fHandler == NULL)
    {
        syslog(LOG_ERR , "Cannot open the file. \" %s \" \" %s \"  " , filename, strerror(errno));
        closelog();
        return 1;
    }

    //write string to the file 
    if(fputs(writeString , fHandler) == EOF)
    {
        syslog(LOG_ERR , "Cannot write to the file \" %s \"  returned \" %s \"  " , filename, strerror(errno));
        closelog();
        return 1;
    }
    if(fputs("\n" , fHandler) == EOF)
    {
        syslog(LOG_ERR , "Cannot write to the file \" %s \"  returned \" %s \"  " , filename, strerror(errno));
        closelog();
        return 1;
    }

    //close the file 
    if(fclose(fHandler) != 0 )
    {
        syslog(LOG_ERR , "Cannot close the file. \" %s \" \" %s \"  " , filename, strerror(errno));
        closelog();
        return 1;
    }
}