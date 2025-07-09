#include "proxy_parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
// #pragma comment(lib, "ws2_32.lib")
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <stdarg.h>

typedef struct ParsedRequest ParsedRequest;

#define MAX_BYTES 4096    //max allowed size of request/response
#define MAX_CLIENTS 400     //max number of client requests served at a time
#define MAX_SIZE 200*(1<<20)     //size of the cache
#define MAX_ELEMENT_SIZE 10*(1<<20)     //max size of an element in cache

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

typedef struct cache_element cache_element;

struct cache_element{
    char* data;         //data stores response
    int len;          //length of data i.e.. sizeof(data)...
    char* url;        //url stores the request
	time_t lru_time_track;    //lru_time_track stores the latest time the element is  accesed
    cache_element* next;    //pointer to next element
};

cache_element* find(char* url);
int add_cache_element(char* data,int size,char* url);
void remove_cache_element();

int port_number = 8080;                // Default Port
SOCKET proxy_socketId;                    // socket descriptor of proxy server
pthread_t tid[MAX_CLIENTS];         //array to store the thread ids of clients
sem_t seamaphore;                    //if client requests exceeds the max_clients this seamaphore puts the
                                    //waiting threads to sleep and wakes them when traffic on queue decreases
//sem_t cache_lock;                   
pthread_mutex_t lock;               //lock is used for locking the cache


cache_element* head;                //pointer to the cache
int cache_size;             //cache_size denotes the current size of the cache

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

#define LOG_RESET   "\x1b[0m"
#define LOG_BOLD    "\x1b[1m"
#define LOG_RED     "\x1b[31m"
#define LOG_GREEN   "\x1b[32m"
#define LOG_YELLOW  "\x1b[33m"
#define LOG_BLUE    "\x1b[34m"
#define LOG_MAGENTA "\x1b[35m"
#define LOG_CYAN    "\x1b[36m"
#define LOG_WHITE   "\x1b[37m"

void log_event(const char* color, const char* tag, const char* fmt, ...) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", t);
    printf("%s[%s] %s%-12s%s ", LOG_BOLD, timebuf, color, tag, LOG_RESET);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

int sendErrorMessage(SOCKET socket, int status_code)
{
    char str[1024];
    char currentTime[50];
    time_t now = time(0);

    struct tm data = *gmtime(&now);
    strftime(currentTime,sizeof(currentTime),"%a, %d %b %Y %H:%M:%S %Z", &data);

    switch(status_code)
    {
        case 400: snprintf(str, sizeof(str), "HTTP/1.1 400 Bad Request\r\nContent-Length: 95\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n<BODY><H1>400 Bad Rqeuest</H1>\n</BODY></HTML>", currentTime);
                  log_event(LOG_RED, "[ERROR]", "400 Bad Request");
                  send(socket, str, strlen(str), 0);
                  break;

        case 403: snprintf(str, sizeof(str), "HTTP/1.1 403 Forbidden\r\nContent-Length: 112\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n<BODY><H1>403 Forbidden</H1><br>Permission Denied\n</BODY></HTML>", currentTime);
                  log_event(LOG_RED, "[ERROR]", "403 Forbidden");
                  send(socket, str, strlen(str), 0);
                  break;

        case 404: snprintf(str, sizeof(str), "HTTP/1.1 404 Not Found\r\nContent-Length: 91\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n<BODY><H1>404 Not Found</H1>\n</BODY></HTML>", currentTime);
                  log_event(LOG_RED, "[ERROR]", "404 Not Found");
                  send(socket, str, strlen(str), 0);
                  break;

        case 500: snprintf(str, sizeof(str), "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 115\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\n<BODY><H1>500 Internal Server Error</H1>\n</BODY></HTML>", currentTime);
                  //printf("500 Internal Server Error\n");
                  send(socket, str, strlen(str), 0);
                  break;

        case 501: snprintf(str, sizeof(str), "HTTP/1.1 501 Not Implemented\r\nContent-Length: 103\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>404 Not Implemented</TITLE></HEAD>\n<BODY><H1>501 Not Implemented</H1>\n</BODY></HTML>", currentTime);
                  log_event(LOG_RED, "[ERROR]", "501 Not Implemented");
                  send(socket, str, strlen(str), 0);
                  break;

        case 505: snprintf(str, sizeof(str), "HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Length: 125\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>505 HTTP Version Not Supported</TITLE></HEAD>\n<BODY><H1>505 HTTP Version Not Supported</H1>\n</BODY></HTML>", currentTime);
                  log_event(LOG_RED, "[ERROR]", "505 HTTP Version Not Supported");
                  send(socket, str, strlen(str), 0);
                  break;

        default:  return -1;

    }
    return 1;
}

int connectRemoteServer(char* host_addr, int port_num)
{
    // Creating Socket for remote server ---------------------------

    SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, 0);

    if( remoteSocket == INVALID_SOCKET)
    {
        log_event(LOG_RED, "[ERROR]", "Error in Creating Socket.");
        return -1;
    }
    
    // Get host by the name or ip address provided

    struct hostent *host = gethostbyname(host_addr);    
    if(host == NULL)
    {
        log_event(LOG_RED, "[ERROR]", "No such host exists: %s", host_addr);
        return -1;
    }

    // inserts ip address and port number of host in struct `server_addr`
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);

    memcpy((char *)&server_addr.sin_addr.s_addr, (char *)host->h_addr, host->h_length);

    // Connect to Remote server ----------------------------------------------------

    if( connect(remoteSocket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR )
    {
        log_event(LOG_RED, "[ERROR]", "Error in connecting to remote server %s:%d!", host_addr, port_num);
        return -1;
    }
    // free(host_addr);
    return remoteSocket;
}


int handle_request(SOCKET clientSocket, ParsedRequest *request, char *tempReq)
{
    char *buf = (char*)malloc(sizeof(char)*MAX_BYTES);
    strcpy(buf, "GET ");
    strcat(buf, request->path);
    strcat(buf, " ");
    strcat(buf, request->version);
    strcat(buf, "\r\n");

    size_t len = strlen(buf);

    if (ParsedHeader_set(request, "Connection", "close") < 0){
        log_event(LOG_MAGENTA, "[INFO]", "set header key not work");
    }

    if(ParsedHeader_get(request, "Host") == NULL)
    {
        if(ParsedHeader_set(request, "Host", request->host) < 0){
            log_event(LOG_MAGENTA, "[INFO]", "Set \"Host\" header key not working");
        }
    }

    if (ParsedRequest_unparse_headers(request, buf + len, (size_t)MAX_BYTES - len) < 0) {
        log_event(LOG_MAGENTA, "[INFO]", "unparse failed");
        //return -1;                // If this happens Still try to send request without header
    }

    int server_port = 80;                // Default Remote Server Port
    if(request->port != NULL)
        server_port = atoi(request->port);

    log_event(LOG_CYAN, "[FLOW]", "Forwarding request to remote server: %s%s", request->host, request->path);
    SOCKET remoteSocketID = connectRemoteServer(request->host, server_port);

    if(remoteSocketID == INVALID_SOCKET)
        return -1;

    int bytes_send = send(remoteSocketID, buf, strlen(buf), 0);

    memset(buf, 0, MAX_BYTES);

    bytes_send = recv(remoteSocketID, buf, MAX_BYTES-1, 0);
    char *temp_buffer = (char*)malloc(sizeof(char)*MAX_BYTES); //temp buffer
    int temp_buffer_size = MAX_BYTES;
    int temp_buffer_index = 0;

    while(bytes_send > 0)
    {
        send(clientSocket, buf, bytes_send, 0);
        
        for(int i=0;i<bytes_send/sizeof(char);i++){
            temp_buffer[temp_buffer_index] = buf[i];
            temp_buffer_index++;
        }
        temp_buffer_size += MAX_BYTES;
        temp_buffer=(char*)realloc(temp_buffer,temp_buffer_size);

        if(bytes_send < 0)
        {
            perror("Error in sending data to client socket.\n");
            break;
        }
        memset(buf, 0, MAX_BYTES);

        bytes_send = recv(remoteSocketID, buf, MAX_BYTES-1, 0);

    } 
    temp_buffer[temp_buffer_index]='\0';
    free(buf);
    add_cache_element(temp_buffer, strlen(temp_buffer), tempReq);
    log_event(LOG_GREEN, "[FLOW]", "Response sent to client and cached.");
    free(temp_buffer);
    
    
    closesocket(remoteSocketID);
    return 0;
}

int checkHTTPversion(char *msg)
{
    int version = -1;

    if(strncmp(msg, "HTTP/1.1", 8) == 0)
    {
        version = 1;
    }
    else if(strncmp(msg, "HTTP/1.0", 8) == 0)            
    {
        version = 1;                                        // Handling this similar to version 1.1
    }
    else
        version = -1;

    return version;
}


void* thread_fn(void* socketNew)
{
    sem_wait(&seamaphore);
    int* t= (int*)(socketNew);
    SOCKET socket=(SOCKET)(*t);
    int bytes_send_client,len;
    char *buffer = (char*)calloc(MAX_BYTES,sizeof(char));
    memset(buffer, 0, MAX_BYTES);
    bytes_send_client = recv(socket, buffer, MAX_BYTES, 0);
    while(bytes_send_client > 0)
    {
        len = strlen(buffer);
        if(strstr(buffer, "\r\n\r\n") == NULL)
        {
            bytes_send_client = recv(socket, buffer + len, MAX_BYTES - len, 0);
        }
        else{
            break;
        }
    }
    char *tempReq = (char*)malloc(strlen(buffer)*sizeof(char)+1);
    for (int i = 0; i < strlen(buffer); i++)
    {
        tempReq[i] = buffer[i];
    }
    log_event(LOG_YELLOW, "[CLIENT]", "Received request from client");
    struct cache_element* temp = find(tempReq);
    if( temp != NULL){
        int size=temp->len/sizeof(char);
        int pos=0;
        char response[MAX_BYTES];
        while(pos<size){
            memset(response,0,MAX_BYTES);
            for(int i=0;i<MAX_BYTES;i++){
                response[i]=temp->data[pos];
                pos++;
            }
            send(socket,response,MAX_BYTES,0);
        }
        log_event(LOG_GREEN, "[CACHE HIT]", "Response sent from cache.");
    }
    else if(bytes_send_client > 0)
    {
        len = strlen(buffer);
        ParsedRequest* request = ParsedRequest_create();
        if (ParsedRequest_parse(request, buffer, len) < 0)
        {
            log_event(LOG_RED, "[ERROR]", "Parsing failed");
        }
        else
        {
            memset(buffer, 0, MAX_BYTES);
            if(!strcmp(request->method,"GET"))
            {
                if( request->host && request->path && (checkHTTPversion(request->version) == 1) )
                {
                    log_event(LOG_CYAN, "[FLOW]", "Handling GET request for %s%s", request->host, request->path);
                    bytes_send_client = handle_request(socket, request, tempReq);
                    if(bytes_send_client == -1)
                    {
                        sendErrorMessage(socket, 500);
                    }
                }
                else
                    sendErrorMessage(socket, 500);
            }
            else
            {
                log_event(LOG_MAGENTA, "[INFO]", "This code doesn't support any method other than GET");
            }
        }
        ParsedRequest_destroy(request);
    }
    else if( bytes_send_client < 0)
    {
        log_event(LOG_RED, "[ERROR]", "Error in receiving from client.");
    }
    else if(bytes_send_client == 0)
    {
        log_event(LOG_YELLOW, "[INFO]", "Client disconnected!");
    }
    shutdown(socket, SD_BOTH);
    closesocket(socket);
    free(buffer);
    sem_post(&seamaphore);
    free(tempReq);
    return NULL;
}


int main(int argc, char * argv[]) {
    printf("\n========================================\n");
    printf("||      DANGER PROXY SERVER 9000!     ||\n");
    printf("========================================\n\n");

    SOCKET client_socketId;
    int client_len; // client_socketId == to store the client socket id
    struct sockaddr_in server_addr, client_addr; // Address of client and server to be assigned

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

    sem_init(&seamaphore,0,MAX_CLIENTS); // Initializing seamaphore and lock
    pthread_mutex_init(&lock,NULL); // Initializing lock for cache
    

    if(argc == 2)        //checking whether two arguments are received or not
    {
        port_number = atoi(argv[1]);
    }
    else
    {
        log_event(LOG_RED, "[ERROR]", "Too few arguments");
        WSACleanup();
        exit(1);
    }
    log_event(LOG_BLUE, "[PROXY]", "Setting Proxy Server Port : %d", port_number);

    //creating the proxy socket
    proxy_socketId = socket(AF_INET, SOCK_STREAM, 0);

    if( proxy_socketId == INVALID_SOCKET)
    {
        log_event(LOG_RED, "[ERROR]", "Failed to create socket.");
        WSACleanup();
        exit(1);
    }

    int reuse =1;
    if (setsockopt(proxy_socketId, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) == SOCKET_ERROR) 
        perror("setsockopt(SO_REUSEADDR) failed\n");

    memset(&server_addr, 0, sizeof(server_addr));  
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number); // Assigning port to the Proxy
    server_addr.sin_addr.s_addr = INADDR_ANY; // Any available adress assigned

    // Binding the socket
    if( bind(proxy_socketId, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR )
    {
        log_event(LOG_RED, "[ERROR]", "Port is not free");
        closesocket(proxy_socketId);
        WSACleanup();
        exit(1);
    }
    log_event(LOG_BLUE, "[PROXY]", "Binding on port: %d", port_number);

    // Proxy socket listening to the requests
    int listen_status = listen(proxy_socketId, MAX_CLIENTS);

    if(listen_status == SOCKET_ERROR )
    {
        log_event(LOG_RED, "[ERROR]", "Error while Listening !");
        closesocket(proxy_socketId);
        WSACleanup();
        exit(1);
    }

    int i = 0; // Iterator for thread_id (tid) and Accepted Client_Socket for each thread
    int Connected_socketId[MAX_CLIENTS];   // This array stores socket descriptors of connected clients

    // Infinite Loop for accepting connections
    while(1)
    {
        
        memset(&client_addr, 0, sizeof(client_addr));            // Clears struct client_addr
        client_len = sizeof(client_addr); 

        // Accepting the connections
        client_socketId = accept(proxy_socketId, (struct sockaddr*)&client_addr,&client_len);    // Accepts connection
        if(client_socketId == INVALID_SOCKET)
        {
            log_event(LOG_RED, "[ERROR]", "Error in Accepting connection !");
            closesocket(proxy_socketId);
            WSACleanup();
            exit(1);
        }
        else{
            Connected_socketId[i] = client_socketId; // Storing accepted client into array
        }

        // Getting IP address and port number of client
        struct sockaddr_in* client_pt = (struct sockaddr_in*)&client_addr;
        struct in_addr ip_addr = client_pt->sin_addr;
        char *str = inet_ntoa(ip_addr);                                        // INET_ADDRSTRLEN: Default ip address size
        log_event(LOG_YELLOW, "[CLIENT]", "[CONNECT] Port: %d, IP: %s", ntohs(client_addr.sin_port), str);
        pthread_create(&tid[i],NULL,thread_fn, (void*)&Connected_socketId[i]); // Creating a thread for each client accepted
        i++; 
    }
    closesocket(proxy_socketId);                                    // Close socket
    WSACleanup();
    return 0;
}

cache_element* find(char* url){
    cache_element* site=NULL;
    pthread_mutex_lock(&lock);
    if(head!=NULL){
        site = head;
        while (site!=NULL)
        {
            if(!strcmp(site->url,url)){
                log_event(LOG_GREEN, "[CACHE HIT]", "%s", url);
                site->lru_time_track = time(NULL);
                break;
            }
            site=site->next;
        }
    }
    if(site == NULL) {
        log_event(LOG_RED, "[CACHE MISS]", "%s", url);
    }
    pthread_mutex_unlock(&lock);
    return site;
}

void remove_cache_element(){
    cache_element * p ;
    cache_element * q ;
    cache_element * temp;
    pthread_mutex_lock(&lock);
    if( head != NULL) {
        for (q = head, p = head, temp =head ; q -> next != NULL; 
            q = q -> next) {
            if(( (q -> next) -> lru_time_track) < (temp -> lru_time_track)) {
                temp = q -> next;
                p = q;
            }
        }
        if(temp == head) {
            head = head -> next;
        } else {
            p->next = temp->next;    
        }
        log_event(LOG_YELLOW, "[CACHE EVICT]", "%s", temp->url);
        cache_size = cache_size - (temp -> len) - sizeof(cache_element) - 
        strlen(temp -> url) - 1;
        free(temp->data);           
        free(temp->url);
        free(temp);
    }
    pthread_mutex_unlock(&lock);
}

int add_cache_element(char* data,int size,char* url){
    pthread_mutex_lock(&lock);
    int element_size=size+1+strlen(url)+sizeof(cache_element);
    if(element_size>MAX_ELEMENT_SIZE){
        pthread_mutex_unlock(&lock);
        return 0;
    }
    else
    {   while(cache_size+element_size>MAX_SIZE){
            remove_cache_element();
        }
        cache_element* element = (cache_element*) malloc(sizeof(cache_element));
        element->data= (char*)malloc(size+1);
        strcpy(element->data,data); 
        element -> url = (char*)malloc(1+( strlen( url )*sizeof(char)  ));
        strcpy( element -> url, url );
        element->lru_time_track=time(NULL);
        element->next=head; 
        element->len=size;
        head=element;
        cache_size+=element_size;
        log_event(LOG_CYAN, "[CACHE STORE]", "%s", url);
        pthread_mutex_unlock(&lock);
        return 1;
    }
    return 0;
}