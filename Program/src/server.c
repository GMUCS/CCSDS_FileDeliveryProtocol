/*------------------------------------------------------------------------------
CMPT-361-AS50(1) - 2017 Fall - Introduction to Networks
Assignment #2
Evan Giese 1689223

This is my file for server.c. It develops a udp server for select.
------------------------------------------------------------------------------*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <sys/select.h>
#include <sys/time.h>
#include "utils.h"
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <time.h>
#include "port.h"

#ifdef CSP_NETWORK
    #include "csp.h"
#endif

static int exit_now;
 
 
//if conn_typ == 1, tcp/ bind_to_host == 1 for binding local, 2 for connect
static int prepareHost(char *host_name, void *addr, size_t *size_of_addr, char *port, int conn_type, int bind_to_host)
{

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_V4MAPPED;
    hints.ai_socktype = conn_type;
    
    
    int err = getaddrinfo(host_name, port, &hints, &res);

    if (err != 0) {
        ssp_error("get addr info");
        return -1;
    }

    int sfd;
    struct addrinfo *cur;

    for (cur = res; cur != NULL; cur = cur->ai_next)
    {

        sfd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);

        if (sfd < 0) {
            ssp_error("socket");
        }

        int val = 1;
        err = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
        if (err == -1) {
            ssp_error("set sock opt");
            ssp_close(sfd);
            continue;
        }
        
        if (bind_to_host == 1) {
            err = bind(sfd, cur->ai_addr, cur->ai_addrlen);
            if (err == -1) {
                ssp_error("bind");
                ssp_close(sfd);
                continue;
            }
        } else {
            err = connect(sfd, cur->ai_addr, cur->ai_addrlen);
            if (err == -1) { 
                ssp_error("connection with the server failed...\n"); 
                ssp_close(sfd);
                continue;
            }
        }


        

        freeaddrinfo(cur);
        break;
    }

    if (cur == NULL)
    {
        ssp_printf("error could not create server\n");
        freeaddrinfo(res);
        return -1;
    }

    return sfd;
}
/*------------------------------------------------------------------------------
    This function is the inturpt handler for sigaction, right now i just 
    handle sigint, so we can exit nicely.
------------------------------------------------------------------------------*/

static void interuptHandler(int signum)
{
    switch (signum)
    {
    case SIGINT:
        exit_now = 1;
        break;
    }
}


//see header file
int *prepareSignalHandler()
{
    struct sigaction actionData;
    sigemptyset(&actionData.sa_mask);
    actionData.sa_handler = interuptHandler;
    actionData.sa_flags = 0;

    if (sigaction(SIGINT, &actionData, NULL) == -1)
    {
        perror("sigaction sigint failed\n");
        exit(EXIT_FAILURE);
    }
    return &exit_now;
}

static int resizeBuff(char **buffer, uint32_t *newBufferSize, uint32_t *prev_buff_size) {

     if (*newBufferSize != *prev_buff_size) {
            *buffer = realloc(*buffer, *newBufferSize);

            if(checkAlloc(*buffer, 0)){
                *prev_buff_size = *newBufferSize;
                return 1;
            } 
            return 0;
    }
    return 1;
}
//see header file
void connection_server(char* port, int initial_buff_size, int connection_limit, 
    int (*onRecv)(int sfd, char *packet, uint32_t packet_len,  uint32_t *buff_size, void *addr, size_t size_of_addr, void *other), 
    int (*onTimeOut)(void *other),
    int (*onStdIn)(void *other),
    int (*checkExit)(void *other),
    void (*onExit)(void *other),
    void *other)
{
    uint32_t size_of_addr = 0;
    void *addr = ssp_init_sockaddr_struct((size_t*)&size_of_addr);

    int sfd = prepareHost(NULL, addr, &size_of_addr, port, SOCK_STREAM, 1);
    if (sfd < 0)
        exit_now = 1;

    int err = listen(sfd, connection_limit);

    if (err == -1)
        ssp_error("listen failed\n");

    size_t size_of_socket_struct = 0;

    void *socket_set = ssp_init_socket_set(&size_of_socket_struct);
    void *read_socket_set = ssp_init_socket_set(&size_of_socket_struct);

    ssp_fd_zero(socket_set);
    ssp_fd_set(sfd, socket_set);
    ssp_fd_set(STDIN_FILENO, socket_set);

    uint32_t *buff_size = ssp_alloc(1, sizeof(uint32_t));
    checkAlloc(buff_size, 1);

    *buff_size = initial_buff_size;
    uint32_t prev_buff_size = *buff_size;

    char *buff = ssp_alloc(sizeof(char), *buff_size);
    checkAlloc(buff, 1);

    for (;;)
    {
        
        if (exit_now || checkExit(other)){
            ssp_printf("exiting server thread\n");
            break;
        }
    
        memcpy(read_socket_set, socket_set, size_of_socket_struct);
        int nrdy = ssp_select(connection_limit + 1, read_socket_set, NULL,  NULL, 100e3);

        if(!resizeBuff(&buff, buff_size, &prev_buff_size)){
            ssp_printf("packet too large, cannot resize buffer\n");
        }

        if (nrdy == -1) {
            ssp_error("select");
            continue;
        }
        //timeout
        if (nrdy == 0) {
            if (onTimeOut(other) == -1)
                ssp_printf("timeout failed\n");

            continue;
        }

        for(int i = 0; i < connection_limit + 1; i++) {

            if (ssp_fd_is_set(i, read_socket_set)) {

                if (i == STDIN_FILENO) {
                    onStdIn(other);
                    break;
                }

                if (i == sfd) {
                    int new_socket = accept(i, (struct sockaddr*) addr, &size_of_addr);
                    if (new_socket < 0)
                        ssp_error ("accept failed");
                    
                    ssp_fd_set(new_socket, socket_set);
                    break;
                }

                int count = ssp_recvfrom(i, buff, *buff_size, 0, NULL, NULL);

                if (count < 0) {
                    ssp_error("recv failed server");
                    ssp_close(i);
                    ssp_fd_clr(i, socket_set);
                }

                else {
                    if (onRecv(i, buff, count, buff_size, addr, size_of_addr, other) == -1) {
                        ssp_printf("recv failed, closing socket\n");
                        ssp_fd_clr(i, socket_set);
                        ssp_close(i);
                    }
                }
            }
        }
    }
    ssp_free(addr);
    ssp_free(read_socket_set);
    ssp_free(socket_set);
    ssp_free(buff_size);
    ssp_free(buff);
    ssp_close(sfd);
    onExit(other);
}




//see header file
void connectionless_server(char* port, int initial_buff_size, 
    int (*onRecv)(int sfd, char *packet, uint32_t packet_len,  uint32_t *buff_size, void *addr, size_t size_of_addr, void *other), 
    int (*onTimeOut)(void *other),
    int (*onStdIn)(void *other),
    int (*checkExit)(void *other),
    void (*onExit)(void *other),
    void *other)
{

    uint32_t size_of_addr = 0;
    void *addr = ssp_init_sockaddr_struct((size_t*)&size_of_addr);

    int sfd = prepareHost(NULL, addr, &size_of_addr, port, SOCK_DGRAM, 1);
    if (sfd < 0)
        exit_now = 1;

    size_t size_of_socket_struct = 0;


    void *socket_set = ssp_init_socket_set(&size_of_socket_struct);
    void *read_socket_set = ssp_init_socket_set(&size_of_socket_struct);

    ssp_fd_zero(socket_set);
    ssp_fd_set(sfd, socket_set);
    ssp_fd_set(STDIN_FILENO, socket_set);

    uint32_t *buff_size = ssp_alloc(1, sizeof(uint32_t));
    checkAlloc(buff_size, 1);

    *buff_size = initial_buff_size + 10;
    uint32_t prev_buff_size = *buff_size;

    char *buff = ssp_alloc(sizeof(char), *buff_size);
    checkAlloc(buff, 1);



    for (;;)
    {
        if (exit_now || checkExit(other)){
            ssp_printf("exiting server thread\n");
            break;
        }
    
        memcpy(read_socket_set, socket_set, size_of_socket_struct);
        int nrdy = ssp_select(sfd + 1, read_socket_set, NULL,  NULL, 100e3);

        if(!resizeBuff(&buff, buff_size, &prev_buff_size)){
            ssp_printf("packet too large, cannot resize buffer\n");
        }

        if (nrdy == -1) {
            ssp_error("select");
            continue;
        }
        
        if (nrdy == 0) {
            if (onTimeOut(other) == -1)
                ssp_printf("timeout failed\n");
            continue;
        }
        
        if (ssp_fd_is_set(STDIN_FILENO, read_socket_set)) {
            onStdIn(other);
            continue;
        }

        if (ssp_fd_is_set(sfd, read_socket_set)) {
            int count = ssp_recvfrom(sfd, buff, *buff_size, 0, addr, &size_of_addr);
           
            if (count == -1)
                continue;

            else if (count >= *buff_size) {   
                ssp_printf("packet too large\n");
            }
            else {
                if (onRecv(sfd, buff, count, buff_size, addr, size_of_addr, other) == -1)
                    ssp_printf("recv failed\n");
            }
        }
    }
    ssp_free(addr);
    ssp_free(read_socket_set);
    ssp_free(socket_set);
    ssp_free(buff_size);
    ssp_free(buff);
    ssp_close(sfd);
    onExit(other);
    
}




//https://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/udpclient.c
void connectionless_client(char *hostname, char*port, int packet_len, void *onSendParams, void *onRecvParams, void *checkExitParams, void *onExitParams,
    int (*onSend)(int sfd, void *addr, size_t size_of_addr, void *onSendParams),
    int (*onRecv)(int sfd, char *packet, uint32_t packet_len, uint32_t *buff_size, void *addr, size_t size_of_addr, void *onRecvParams) ,
    int (*checkExit)(void *checkExitParams),
    void (*onExit)(void *params))
{

    int sfd, count;

    uint32_t size_of_addr = 0;
    void *addr = ssp_init_sockaddr_struct((size_t*)&size_of_addr);

    sfd = prepareHost(hostname, addr, &size_of_addr, port, SOCK_DGRAM, 0);
    if (sfd < 0)
        exit_now = 1;

    uint32_t *buff_size = ssp_alloc(1, sizeof(uint32_t));
    checkAlloc(buff_size, 1);

    *buff_size = packet_len + 10;

    uint32_t prev_buff_size = *buff_size;

    char *buff = ssp_alloc(sizeof(char), prev_buff_size);
    checkAlloc(buff, 1);


    for (;;) {
        if (exit_now || checkExit(checkExitParams))
             break;
        
        if(!resizeBuff(&buff, buff_size, &prev_buff_size)){
            ssp_error("packet too large, cannot resize buffer\n");
        }

        if (onSend(sfd, addr, size_of_addr, onSendParams)) 
            ssp_error("send failed\n");

        count = ssp_recvfrom(sfd, buff, packet_len, MSG_DONTWAIT, addr, &size_of_addr);
       
        if (count == -1)
            continue;

        else if (count >= *buff_size){   
            ssp_error("packet too large\n");
            continue;
        }
        else{
            if (onRecv(sfd, buff, count, buff_size, addr, size_of_addr, onRecvParams) == -1)
                ssp_error("recv failed\n");
        }
        
    }

    free(addr);
    free(buff_size);
    free(buff);
    ssp_close(sfd);
    onExit(onExitParams);
}


//https://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/udpclient.c
void connection_client(char *hostname, char*port, int packet_len, void *onSendParams, void *onRecvParams, void *checkExitParams, void *onExitParams,
    int (*onSend)(int sfd, void *addr, size_t size_of_addr, void *onSendParams),
    int (*onRecv)(int sfd, char *packet, uint32_t packet_len, uint32_t *buff_size, void *addr, size_t size_of_addr, void *onRecvParams) ,
    int (*checkExit)(void *checkExitParams),
    void (*onExit)(void *params))
{

    int sfd, count;

    uint32_t size_of_addr = 0;
    void *addr = ssp_init_sockaddr_struct((size_t*)&size_of_addr);

    sfd = prepareHost(hostname, addr, &size_of_addr, port, SOCK_STREAM, 0);
    if (sfd < 0)
        exit_now = 1;

    uint32_t *buff_size = ssp_alloc(1, sizeof(uint32_t));
    checkAlloc(buff_size, 1);

    *buff_size = packet_len;
    uint32_t prev_buff_size = *buff_size;

    char *buff = ssp_alloc(prev_buff_size, sizeof(char));
    checkAlloc(buff, 1);


    for (;;) {
        
        if (exit_now || checkExit(checkExitParams))
             break;
        
        if(!resizeBuff(&buff, buff_size, &prev_buff_size)){
            ssp_printf("packet too large, cannot resize buffer\n");
        }

        if (onSend(sfd, addr, size_of_addr, onSendParams)) 
            ssp_error("send failed here\n");

        count = ssp_recvfrom(sfd, buff, packet_len, MSG_DONTWAIT, NULL, &size_of_addr);
       
        if (count < 0)
            continue;

        if (onRecv(sfd, buff, count, buff_size, addr, size_of_addr, onRecvParams) == -1) {
            ssp_error("recv failed\n");
            exit_now = 1;
        }
        
    }
    free(addr);
    free(buff_size);
    free(buff);
    ssp_close(sfd);
    onExit(onExitParams);
}

/*------------------------------------------------------------------------------
                                    
                                    CSP STUFF!

------------------------------------------------------------------------------*/

#ifdef CSP_NETWORK

//https://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/udpclient.c
void csp_connectionless_client(uint8_t my_id, uint8_t my_port, void *onSendParams, void *onRecvParams, void *checkExitParams, void *onExitParams,
    int (*onSend)(int sfd, void *addr, void *onSendParams),
    int (*onRecv)(int sfd, char *packet, uint32_t packet_len, uint32_t *buff_size, void *addr, size_t size_of_addr, void *onRecvParams) ,
    int (*checkExit)(void *checkExitParams),
    void (*onExit)(void *params)) 
{

}


void csp_connectionless_server(uint8_t my_id, uint8_t my_port,
    int (*onRecv)(int sfd, char *packet, uint32_t packet_len,  uint32_t *buff_size, void *addr, size_t size_of_addr, void *other), 
    int (*onTimeOut)(void *other),
    int (*onStdIn)(void *other),
    int (*checkExit)(void *other),
    void (*onExit)(void *other),
    void *other)
{
    int err = csp_init(my_id);
    if (err < 0) {
        ssp_error("ERROR: couldn't init csp\n");
        return; 
    }
    //csp_socket_t *socket = csp_socket(CSP_SO_XTEAREQ | CSP_SO_HMACREQ | CSP_SO_CRC32REQ);
    csp_socket_t *socket = csp_socket(0);
    
    err = csp_bind(socket, my_port);
    if (err < 0) {
        ssp_error("ERROR: couldn't bind csp\n");
        return; 
    }

    for (;;) {
        csp_packet_t *packet = csp_recvfrom(socket, 10);
        
        //timout
        if (packet == NULL) {

        }
        else {
            //do stuff
            ssp_printf("%s\n", packet->data);
            csp_buffer_free(packet);
        }
        
    }
}
#endif