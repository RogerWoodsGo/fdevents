#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include"fdevent.h"

/**
 * @file testFdevent.c
 * @brief test FDevent from lighttpd
 * @author wuwenchao <wuwenchao@miaozhen.com>
 * @version 1.0
 * @date 2015-07-27
 */

typedef struct context{
    int fd;
}ctx_t;
handler_t accept_handler(void *ctx, int revents){
    ctx_t* context = (ctx_t*)ctx;
    int sock_fd = context->fd;
    int new_fd;
    struct sockaddr_in client_addr; // connector's address information
    socklen_t sin_size;
    printf("Get the client connected!"); 
    new_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &sin_size);
    if (new_fd <= 0) {
        perror("accept");
        return HANDLER_ERROR;
    }
    else{
        printf("new connection client %s\n", inet_ntoa(client_addr.sin_addr));
    }

    return HANDLER_FINISHED;
}
int main(){

    //Socket初始化
    int sock_fd;  // listen on sock_fd, new connection on new_fd
    struct sockaddr_in server_addr;    // server address information
    int yes = 1;

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    server_addr.sin_family = AF_INET;         // host byte order
    server_addr.sin_port = htons(10001);     // short, network byte order
    server_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(sock_fd, 4) == -1) {
        perror("listen");
        exit(1);
    }

    printf("listen port %d\n", 10001);
    //初始化event
    int fd_ndx = -1;
    int n = -1;
    int revents, fd;
    ctx_t* ctx = (ctx_t*)malloc(sizeof(ctx));
    ctx->fd = sock_fd; 
    fdevents *ev = fdevent_init(4096, FDEVENT_HANDLER_SELECT);
    printf("socket fd is %d\n", sock_fd);
    fdevent_register(ev, sock_fd, accept_handler, ctx);
    fdevent_event_set(ev, &fd_ndx, sock_fd, FDEVENT_IN);
    if((n = fdevent_poll(ev, 3600000)) > 0 ){

        printf("event number is %d\n", n);
        //TODO 
        fd_ndx = -1;
        do {
            fdevent_handler handler;
            void *context;
            handler_t r;

            fd_ndx  = fdevent_event_next_fdndx (ev, fd_ndx);
            if (-1 == fd_ndx) break; /* not all fdevent handlers know how many fds got an event */

            revents = fdevent_event_get_revent (ev, fd_ndx);
            fd      = fdevent_event_get_fd     (ev, fd_ndx);
            printf("fd is excuted %d\n", fd);
            handler = fdevent_get_handler(ev, fd);
            context = fdevent_get_context(ev, fd);

            /* connection_handle_fdevent needs a joblist_append */
#if 0
            log_error_write(srv, __FILE__, __LINE__, "sdd",
                    "event for", fd, revents);
#endif
            switch (r = (*handler)(context, revents)) {
                printf("while is excuted\n");
                case HANDLER_FINISHED:
                case HANDLER_GO_ON:
                case HANDLER_WAIT_FOR_EVENT:
                case HANDLER_WAIT_FOR_FD:
                printf("connect happened");
                break;
                case HANDLER_ERROR:
                /* should never happen */
                printf("connect error");
                SEGFAULT();
                break;
                default:
                printf("Error");
                break;
            }
        } while (--n > 0);
    }

    free(ctx);
    fdevent_free(ev);
    return 0;
}


