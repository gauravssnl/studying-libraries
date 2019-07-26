#include <errno.h>
#include <event.h>
#include <evhttp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>

#include "../libevent_helpers.h"


#define PROGRAMM_DESCRIPTION \
    "Simple http-client."

#define PROGRAMM_USAGE \
    "Usage: http-client <server-address> <PORT> <url>\n" \

void on_request_done(struct evhttp_request *req, void *arg)
{
    char buf[1024];
    int s = evbuffer_remove(req->input_buffer, &buf, sizeof(buf) - 1);
    buf[s] = '\0';
    LOG_INFO("Recieved response(from %s:%d):\n%s", req->remote_host, req->remote_port, buf);
    LOG_INFO("Recieved response(%d %s):\n%s", req->response_code, req->response_code_line, buf);

    // terminate event_base_dispatch()
    event_base_loopbreak((struct event_base *)arg);
}

int main(int argc, char **argv)
{
    printf("%s\n", PROGRAMM_DESCRIPTION);
    printf("%s\n", PROGRAMM_USAGE);
    if (argc != 4) {
        LOG_ERROR("Incorrect arguments");
        return 1;
    }

    unsigned short port = atol(argv[2]);
    const char *serv_addr  = argv[1];

    struct event_base *ev_base = event_base_new();
    if (!ev_base)
        SYS_LOG_ERROR_AND_EXIT("event_base_new failed");

    struct evhttp_connection *conn = evhttp_connection_base_new(ev_base, NULL, serv_addr, port);
    if (!conn)
        SYS_LOG_ERROR_AND_EXIT("evhttp_connection_base_new failed");
    struct evhttp_request *req = evhttp_request_new(on_request_done, ev_base);
    if (!req)
        SYS_LOG_ERROR_AND_EXIT("evhttp_request_new failed");

    evhttp_add_header(req->output_headers, "Host", serv_addr);
    evhttp_add_header(req->output_headers, "Connection", "close");
    evhttp_add_header(req->output_headers, "User-Agent", "simple-http-client");

    evhttp_make_request(conn, req, EVHTTP_REQ_GET, argv[3]);
    evhttp_connection_set_timeout(req->evcon, 600);

    // Dispatch events
    event_base_dispatch(ev_base);

    event_base_free(ev_base);
    return 0;
}


