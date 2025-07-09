#include "proxy_parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
#include <unistd.h>
#include <winsock2.h>
// #include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
// #include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct cache_element cache_element;
#define MAX_CLIENTS 10

struct cache_element{
    char* data;
    int len;
    char* url;
    time_t lru_time_track;
    cache_element* next;
};

cache_element* find(char* url);
int add_cache_element(char* data, int size, char* url);
void remove_cache_element();

int port_number = 8080;
int proxy_socketId; //socket id for all the client so that we can send the data to the client

pthread_t tid[MAX_CLIENTS]; // max people that can connect to the server
sem_t semaphore; //its a variable and type of mutex lock so that we can lock the cache so that only one thread can access it at a time

pthread_mutex_t lock;  // lock because there are multiple thread but only one cache so it has to be a shared resource so there might be a race condition so we have to mutex lock it

cache_element* head;
int cache_size;

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

int main(int argc, char* argv[]){
    int client_socketId , client_len;
    struct sockaddr_in server_addr, client_addr;
    sem_init(&semaphore,0, MAX_CLIENTS);
    pthread_mutex_init(&lock, NULL);

    if(argv == 2){
        port_number= atoi(argv[1]);
    }else{
        printf('Too few arguments\n');
        exit(1);
    }

    printf("starting Proxy server at port: %d\n", port_number);
    proxy_socketId = socket(AF_INET, SOCK_STREAM, 0);
    if(proxy_socketId<0){
        printf("Failed to create a socket\n");
        exit(1);
    }
    int reuse = 1;
    if(setsockopt(proxy_socketId, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse))){
        perror("setSocketOpt failed\n");
    }

    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port= htons(port_number);
    server_addr.sin_addr.s_addr =INADDR_ANY;

    if(bind(proxy_socketId,(struct socketaddr*)&server_addr, sizeof(server_addr)<0)){
        perror("port is not available\n");
        exit(1);
    }
    printf("Binding on port %d\n", port_number );
    int listen_status = listen(proxy_socketId, MAX_CLIENTS);
    if(listen_status<0){
        perror("error in listening\n");
        exit(1);
    }
    int i= 0;
    int Connected_socketId[MAX_CLIENTS];

    while(1){
        bzero((char*)&client_addr, sizeof(client_addr));
        client_len = sizeof(client_addr);
        client_socketId = accept(proxy_socketId, (struct sockaddr*)&client_addr, &client_len);

        if(client_socketId<0){
            printf("not able to connect");
            exit(1);
        }else{
            Connected_socketId[i] = client_socketId;
        }

        struct sockaddr_in * client_pt = (struct sockaddr_in *)&client_addr;
        struct in_addr ip_addr = client_pt->sin_addr;
        printf("Client is connected with IP: %s\n", inet_ntoa(ip_addr));
    }

    

}




