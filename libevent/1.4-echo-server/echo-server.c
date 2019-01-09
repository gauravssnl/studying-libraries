#include <event2/event.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <unistd.h>
#include <fcntl.h>

#include "../libevent_helpers.h"

#define MAX_MSG_LEN 128

struct connection
{
    int fd;
    struct event *read_event;
    struct event *write_event;
    char data_to_send[MAX_MSG_LEN + 1];
    int n_data_to_send;
};


#define SERVER_PORT 12345
#define PROGRAMM_DESCRIPTION \
    "Echo server written with libevent.\n" \
    " Create event on each connection and wait till there \n" \
    " will be data to read and add write event to the loop \n" \
    " to write data back when socket will be ready for it.\n"

#define PROGRAMM_USAGE \
    "Usage: echo-server\n" \


struct connection *create_connection()
{
    struct connection *con = calloc(1, sizeof(struct connection));
    if (!con)
        SYS_LOG_ERROR_AND_EXIT("create_connection failed");
    return con;
}

void free_connection_from_callback(struct connection* conn)
{
    close(conn->fd);
    event_del(conn->read_event);
    event_del(conn->write_event);
    event_free(conn->read_event);
    event_free(conn->write_event);
    free(conn);
    return;
}

void write_cb(evutil_socket_t sock, short what, void *arg)
{
    struct connection* conn = arg;
    LOG_INFO("write_cb callback: what - %s (fd = %d)\n", what_to_string(what), sock);

    if (what & EV_WRITE) {
        if (conn->n_data_to_send <= 0) {
            LOG_ERROR("\tTriggered EV_WRITE event but no data to send\n");
        } else {
            int k = write(sock, &conn->data_to_send, conn->n_data_to_send);
            if (k < 0) {
                LOG_ERROR("\tClosing connection with socket %d because of write error\n", sock);
                goto close_conn_and_del_event;
            }
            memmove(conn->data_to_send, conn->data_to_send + k, conn->n_data_to_send - k);
            conn->n_data_to_send -= k;
            LOG_INFO("\tSend %d bytes to client (%d bytes remained to send)\n", k, conn->n_data_to_send);
        }

        if (conn->n_data_to_send == 0) {
            event_del(conn->write_event);
            LOG_INFO("\tRemoving write event for socket %d from loop because all data have been sent\n", conn->fd);
        }
    }

    return;

close_conn_and_del_event:
    free_connection_from_callback(conn);
    return;
}

void read_cb(evutil_socket_t sock, short what, void *arg)
{
    struct connection* conn = arg;
    LOG_INFO("read_cb callback: what - %s (fd = %d)\n", what_to_string(what), sock);

    if (what & EV_READ) {
        int old_n_data_to_send = conn->n_data_to_send;
        int n = read((int)sock, &conn->data_to_send + conn->n_data_to_send, MAX_MSG_LEN);
        if (n == 0) {
            LOG_INFO("\tClosing connection with socket %d\n", sock);
            goto close_conn_and_del_event;
        } else if (n < 0) {
            LOG_ERROR("\tClosing connection with socket %d because of read error\n", sock);
            goto close_conn_and_del_event;
        }
        conn->n_data_to_send += n;
        conn->data_to_send[conn->n_data_to_send] = '\0';

        LOG_INFO("\tRecieved message(%d bytes) from socket %d: %s\n", n, sock,
            conn->data_to_send + old_n_data_to_send);
        if (event_add(conn->write_event, NULL))
            SYS_LOG_ERROR_AND_EXIT("event_add failed");
        LOG_INFO("\tAdding write event for socket %d to loop\n", conn->fd);
    }

    return;

close_conn_and_del_event:
    free_connection_from_callback(conn);
    return;
}



void accept_cb(evutil_socket_t sock, short what, void *arg)
{
    struct connection* accept_conn = arg;
    struct event *accept_ev = accept_conn->read_event;
    struct event_base *ev_base = event_get_base(accept_ev);

    LOG_INFO("accept_cb: New event: %s (fd = %d)\n", what_to_string(what), sock);

    int fd = accept(sock, NULL, NULL);
    if (fd < 0)
        SYS_LOG_ERROR_AND_EXIT("accept failed");

    if (set_nonblock(fd))
        SYS_LOG_ERROR_AND_EXIT("set_nonblock failed");
    
    struct connection *conn = create_connection();
    conn->fd = fd;
    conn->read_event = event_new(ev_base, fd, EV_READ | EV_PERSIST, &read_cb, conn);
    if (!conn->read_event)
        SYS_LOG_ERROR_AND_EXIT("event_new failed");

    conn->write_event = event_new(ev_base, fd, EV_WRITE | EV_PERSIST, &write_cb, conn);
    if (!conn->write_event)
        SYS_LOG_ERROR_AND_EXIT("event_new failed");

    if (event_add(conn->read_event, NULL))
        SYS_LOG_ERROR_AND_EXIT("event_add failed");

    LOG_INFO("\tNew connection created (socket %d)\n", fd);
    LOG_INFO("\tAdding read event for socket %d to loop\n", conn->fd);
}



int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    printf("%s\n", PROGRAMM_DESCRIPTION);
    printf("%s\n", PROGRAMM_USAGE);

    /*============  Starting server ================*/
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 1)
        SYS_LOG_ERROR_AND_EXIT("socket failed");

    struct sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(SERVER_PORT);
    addr_in.sin_addr.s_addr = htonl(INADDR_ANY);

    int enable_reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable_reuse, sizeof(int)) < 0)
        SYS_LOG_ERROR_AND_EXIT("setsockopt failed");

    if (bind(sock, (struct sockaddr*)&addr_in, sizeof(addr_in)))
        SYS_LOG_ERROR_AND_EXIT("bind failed");

    if (set_nonblock(sock))
        SYS_LOG_ERROR_AND_EXIT("set_nonblock failed");

    if (listen(sock, SOMAXCONN))
        SYS_LOG_ERROR_AND_EXIT("listen failed");


    LOG_INFO("Server is running on port %d (fd = %d)\n"
            "\tTry to make connections and put some data(e.g.: telnet 0 %d).\n",
            (int)SERVER_PORT,
            (int)sock,
            (int)SERVER_PORT);
    

    /*========= Creating event_base and adding event (new connection on listening socket) to it ======*/
    struct event_base *ev_base = event_base_new();
    if (!ev_base)
        SYS_LOG_ERROR_AND_EXIT("event_base_new failed");

    struct connection *conn = create_connection();
    conn->fd = sock;
    conn->read_event = event_new(ev_base, sock, EV_READ | EV_PERSIST, &accept_cb, conn);
    if (!conn->read_event)
        SYS_LOG_ERROR_AND_EXIT("event_new failed");

    if (event_add(conn->read_event, NULL))
        SYS_LOG_ERROR_AND_EXIT("event_add failed");

    event_base_dispatch(ev_base);
    event_base_free(ev_base);
    return 0;
}