#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<syslog.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<errno.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include <syslog.h>
#include "queue.h"
#include <time.h>
#include "aesdsocket.h"


volatile sig_atomic_t keepRunning = 1;
static pthread_mutex_t filewrite = PTHREAD_MUTEX_INITIALIZER;

void sig_handler(int sig){
    keepRunning = 0;
}

void appendTime()
{
    time_t now;
    int tfd = open("/var/tmp/aesdsocketdata.txt" , O_CREAT | O_RDWR  , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    time_t started = time(NULL);
    
    while (keepRunning){
        now = time(NULL);
        struct tm tm_now;
        localtime_r(&now, &tm_now);
        char buf[128];
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%z", &tm_now);
        pthread_mutex_lock(&filewrite);
        lseek(tfd, 0, SEEK_END);
        write(tfd, "timestamp:", strlen("timestamp:"));
        write(tfd, buf, strlen(buf));
        write(tfd, "\n", 1);
        lseek(tfd, 0, SEEK_SET);
        pthread_mutex_unlock(&filewrite);
        sleep(10);
    }
    close(tfd);

}
int main( int argc, char *argv[] ) {

    struct sockaddr_in clientAddr;
    socklen_t clientAddr_len = sizeof(clientAddr);
    char buffer[1024];

    struct sigaction sa;
    int option;
    int daemonize = 0;



    while ((option = getopt(argc, argv, "d")) != -1) 
    {
        switch (option) 
        {
            case 'd':
                daemonize = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
                break;
        }    
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sigaction(SIGINT | SIGTERM, &sa, NULL);   

    printf("Server Initialize\n");
    int sockFD = server_init();
    if(sockFD < 0)
    {
        syslog(LOG_ERR , "Error creating socket");
        return -1;
    }

    if(daemonize == 1)
    {
        pid_t pid = fork();
        if(pid < 0)
        {
            syslog(LOG_ERR , "Error forking process");
            return -1;
        }
        if (pid > 0)
        {
            printf("Parent process exiting\n");
            exit(0);
        }
        setsid();
        umask(0);
        chdir("/");
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        int fd = open("/dev/null", O_RDWR);
        if(fd < 0)
        {
            syslog(LOG_ERR , "Error opening /dev/null");
            return -1;
        }
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
    }

    printf("Server listening on %d\n", sockFD);
    if(listen(sockFD, 5) < 0){
        syslog(LOG_ERR , "Error listening on socket");
        close(sockFD);
        return -1;
    }

    // ensure old data file is removed
    remove( "/var/tmp/aesdsocketdata.txt" );

    int clientFD;
    struct clientInfo* clientInfo;

    head_t threads_queue;
    TAILQ_INIT(&threads_queue);
    uint8_t thread_count = 0;
        

    pthread_t timer_thread;

    if(pthread_create(&timer_thread , NULL , (void*)appendTime , NULL ) < 0){
        syslog(LOG_ERR , "Error creating thread");
        return -1;
    }

    while (keepRunning) 
    {
        

        clientFD =  accept(sockFD, (struct sockaddr*)&clientAddr,  &clientAddr_len);
        if (clientFD < 0) {
            syslog(LOG_ERR ,"Error accepting connection");
            perror ("Error accepting connection");
            close(sockFD);
            return -1;
        }
        clientInfo = malloc(sizeof(struct clientInfo));
        clientInfo->clientFD = clientFD;
        clientInfo->clientAddr = &clientAddr;
        
        syslog(LOG_INFO , "Accepted connection from %s", inet_ntoa(clientAddr.sin_addr));
        printf("accepting connection from %s", inet_ntoa(clientAddr.sin_addr));

        pthread_t clinthread;
        int* clientPtr = malloc(sizeof(int));
        *clientPtr = clientFD;

        
        if(pthread_create(&clinthread , NULL , (void*)processClientData , clientInfo ) < 0){
            syslog(LOG_ERR , "Error creating thread");
            free( clientPtr);
            return -1;
        }
        thread_count++;
        struct thread_node * clinthreadnode = malloc(sizeof(struct thread_node));
        clinthreadnode->tid = clinthread;
        clinthreadnode->id = thread_count;
        TAILQ_INSERT_TAIL(&threads_queue, clinthreadnode, next);
        
        printf("Handler assigned to thread. Waiting for next connection.\\n");

       
        printf("Handler assigned to thread. Waiting for next connection.\\n");
    }

    struct thread_node *cur , *nextt;
    TAILQ_FOREACH_SAFE(cur, &threads_queue, next,nextt)
    {
        pthread_join(cur->tid , NULL);
        TAILQ_REMOVE(&threads_queue , cur , next);
        free(cur);
        cur = NULL;
    }

    close(sockFD);
    close(clientFD);
    syslog(LOG_INFO , "Server closed");
    close(sockFD);
    remove("/var/tmp/aesdsocketdata.txt");
    

    return 0;
}

int server_init(){
    
    struct sockaddr_in serverAddr;
    int port = 9000;
    int sockFD = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFD < 0) {
        printf("Error creating socket\n");
        return -1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY ;

    if (bind(sockFD, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        syslog(LOG_ERR , "Error binding socket");
        return -1;
    }
    
    return sockFD;
}


void processClientData( struct clientInfo* clientInfo )
{
    char buffer[1024];
    char *ptr = buffer;
    char *packet_buffer = malloc(MAX_PACKET_SIZE);
    int fd = open("/var/tmp/aesdsocketdata.txt" , O_CREAT | O_RDWR  , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    if(fd < 0){
        perror("Error opening file");
        syslog(LOG_ERR , "Error opening file");
    }
    printf("Connection accepted\n");
    while((keepRunning == 1))
    {
        int bytes = recv(clientInfo->clientFD, buffer, sizeof(buffer), 0);
        if(bytes < 0){
            syslog(LOG_ERR , "Error receiving data");
            break;
        }
        if(bytes == 0){

            printf("Connection closed\n");
            syslog(LOG_INFO , "Closed connection from %s", inet_ntoa(clientInfo->clientAddr->sin_addr)); 
            break;
        }
        ptr = buffer;
        printf("Received %d bytes: %s\n", bytes, buffer);

        int bytes_written = 0;
        while ((bytes_written < bytes) && (keepRunning == 1)) {
            // SEARCH FOR NEW LINE IN THE RECIEVED DATA
            char *newline_pos = strchr(ptr, '\n');
            printf("buffer pointer: %p \n", (void *)(ptr));
            printf("newline pointer: %p \n", (void *)(newline_pos));
            if (newline_pos != NULL) {
                // COPY THE DATA UNTIL THE NEWLINE TO THE PACKET BUFFER
                int packet_size = newline_pos - ptr;
                memcpy(packet_buffer, ptr, (packet_size + 1) );
                // WRITE THE PACKET BUFFER TO THE FILE
                printf("Received %d bytes: %s\n", packet_size+1, packet_buffer);

                pthread_mutex_lock(&filewrite);

                //Seek to the end of the file
                lseek(fd, 0, SEEK_END);
                bytes_written +=  write(fd, packet_buffer, (packet_size+ 1));
                printf("Written %d bytes\n", packet_size+1);
                // bytes_written += packet_size ;
                // MOVE THE BUFFER POINTER TO THE NEXT LINE
                ptr = newline_pos + 1;

                printf("sending data\n");   
                // REWIND THE FILE TO THE BEGINING OF THE DATA
                lseek(fd, 0, SEEK_SET);

                // send the full content of  /var/tmp/aesdsocketdata
                char *buffer_rFile = malloc(MAX_PACKET_SIZE);
                int bytes_read = read(fd, buffer_rFile, MAX_PACKET_SIZE);

                pthread_mutex_unlock(&filewrite);
                if (bytes_read < 0) {
                    syslog(LOG_ERR , "Error reading file");
                    printf("Error reading file\n");
                }
                send(clientInfo->clientFD, buffer_rFile, bytes_read, 0);
                free(buffer_rFile);
            }
            else {
                // COPY THE REMAINING DATA TO THE PACKET BUFFER
                int packet_size = bytes - bytes_written;
                memcpy(packet_buffer, ptr, packet_size );
                pthread_mutex_lock(&filewrite);
                lseek(fd, 0, SEEK_END);
                // WRITE THE PACKET BUFFER TO THE FILE
                bytes_written += write(fd, packet_buffer, packet_size  );
                // bytes_written += packet_size ;
                // MOVE THE BUFFER POINTER TO THE END OF THE DATA
                ptr += packet_size;
                pthread_mutex_unlock(&filewrite);
            }
        }

    }
      
    close(fd);
    free(packet_buffer);
    close (clientInfo->clientFD);
    free((void*)(clientInfo));
    printf("Connection closed\n");
    
}