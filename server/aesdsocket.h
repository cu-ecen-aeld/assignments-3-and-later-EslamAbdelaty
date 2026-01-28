
#define MAX_PACKET_SIZE 65535 


struct clientInfo{
    int clientFD;
    struct sockaddr_in* clientAddr;
};

typedef struct thread_node {
  pthread_t tid;
  int id;
  TAILQ_ENTRY(thread_node) next;
} thread_node_t;

typedef TAILQ_HEAD(head_s, thread_node)head_t;

int server_init();
void processClientData(struct clientInfo* clientInfo );


