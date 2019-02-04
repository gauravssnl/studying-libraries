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
    "Simple http-server that sends back \n" \
    " information about the response."

#define PROGRAMM_USAGE \
    "Usage: http-server <server-address> <PORT>\n" \

const char *RESPONCE = "<H1>Hello there</H1><BR/>";
const char *SERVER_NAME = "Simple HTTP Server";

void on_request(struct evhttp_request *req, void *arg)
{
    (void)arg;
    // Create responce buffer
    struct evbuffer *evb = evbuffer_new();
    if (!evb) 
        return;

    LOG_INFO("Processing request: %s from %s:%d", req->uri, req->remote_host, req->remote_port);

    // Add heading text
    evbuffer_add_printf(evb, "<HTML><HEAD><TITLE>%s Page</TITLE></HEAD><BODY>\n", SERVER_NAME);
    // Add response
    evbuffer_add(evb, RESPONCE, strlen(RESPONCE) + 1);
    // Add formatted text
    evbuffer_add_printf(evb, "Your request is <B>%s</B> from <B>%s</B>.<BR/>Your user agent is '%s'\n", 
        req->uri, req->remote_host,
        evhttp_find_header(req->input_headers, "User-Agent"));
    // Add footer
    evbuffer_add_printf(evb, "</BODY></HTML>");

    // Set HTTP headers
    evhttp_add_header(req->output_headers, "Server", SERVER_NAME);
    evhttp_add_header(req->output_headers, "Connection", "close");

    // Send reply
    evhttp_send_reply(req, HTTP_OK, "OK", evb);

    // Free memory
    evbuffer_free(evb);
}

int main(int argc, char **argv)
{
    printf("%s\n", PROGRAMM_DESCRIPTION);
    printf("%s\n", PROGRAMM_USAGE);
    if (argc < 3) {
        LOG_ERROR("Incorrect arguments");
        return 1;
    }

    unsigned short port = atol(argv[2]);
    const char *host  = argv[1];

    struct event_base *ev_base = event_base_new();
    struct evhttp *http_server = evhttp_new(ev_base);

    if (evhttp_bind_socket(http_server, host, port))
        SYS_LOG_ERROR_AND_EXIT("evhttp_bind_socket failed");


    // Set HTTP request callback                               
    evhttp_set_gencb(http_server, on_request, NULL);
    // Dispatch events
    event_base_dispatch(ev_base);

    evhttp_free(http_server);
    event_base_free(ev_base);
    return 0;
}


