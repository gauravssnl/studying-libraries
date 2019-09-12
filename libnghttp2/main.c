#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>


#include <nghttp2/nghttp2.h>

/* ---- HELPERS ----- */

#define LOG(msg) \
	fprintf(stderr, msg)

#define ERR_EXIT(msg) \
	do { \
		fprintf(stderr, msg); \
		exit(EXIT_FAILURE); \
	} while (0)

#define ARRLEN(x) (sizeof(x) / sizeof(x[0]))

#define MAKE_NV(NAME, VALUE, VALUELEN)                                         \
  {                                                                            \
    (uint8_t *)NAME, (uint8_t *)VALUE, sizeof(NAME) - 1, VALUELEN,             \
        NGHTTP2_NV_FLAG_NONE                                                   \
  }

#define MAKE_NV2(NAME, VALUE)                                                  \
  {                                                                            \
    (uint8_t *)NAME, (uint8_t *)VALUE, sizeof(NAME) - 1, sizeof(VALUE) - 1,    \
        NGHTTP2_NV_FLAG_NONE                                                   \
  }

static void 
print_header(FILE *f, const uint8_t *name, size_t namelen,
                         const uint8_t *value, size_t valuelen) {
fwrite(name, 1, namelen, f);
fprintf(f, ": ");
fwrite(value, 1, valuelen, f);
fprintf(f, "\n");
}

static void 
print_headers(FILE *f, nghttp2_nv *nva, size_t nvlen) {
	size_t i;
	for (i = 0; i < nvlen; ++i)
		print_header(f, nva[i].name, nva[i].namelen, nva[i].value, nva[i].valuelen);
	fprintf(f, "\n");
}


/* ---- DATA TYPES ----- */

typedef struct {
	int some_data;
	nghttp2_session *session;
} session_data_t;


/* ---- CODE ----- */

/* nghttp2_send_callback. Here we transmit the |data|, |length| bytes,
   to the network. */
static ssize_t 
send_callback(nghttp2_session *session, const uint8_t *data,
	size_t length, int flags, void *user_data) 
{
	LOG("send_callback called\n");
	session_data_t *session_data = (session_data_t *)user_data;

	//bufferevent_write(bev, data, length);
	return (ssize_t)length;
}

/* nghttp2_on_header_callback: Called when nghttp2 library emits
   single header name/value pair. */
static int 
on_header_callback(nghttp2_session *session,
	const nghttp2_frame *frame, const uint8_t *name,
	size_t namelen, const uint8_t *value,
	size_t valuelen, uint8_t flags, void *user_data) 
{
	LOG("send_callback called\n");
	session_data_t *session_data = (session_data_t *)user_data;
	return 0;
}

/* nghttp2_on_begin_headers_callback: Called when nghttp2 library gets
   started to receive header block. */
static int 
on_begin_headers_callback(nghttp2_session *session,
     const nghttp2_frame *frame,
     void *user_data) 
{
	LOG("on_begin_headers_callback called\n");
	return 0;
}

static ssize_t 
on_recv_callback(nghttp2_session *session, uint8_t *buf, size_t length, int flags, void *user_data)
{
	LOG("on_recv_callback called\n");
	return 0;
}

/* nghttp2_on_stream_close_callback: Called when a stream is about to
   closed. This example program only deals with 1 HTTP request (1
   stream), if it is closed, we send GOAWAY and tear down the
   session */
static int 
on_stream_close_callback(nghttp2_session *session, int32_t stream_id,
	uint32_t error_code, void *user_data) 
{
	LOG("on_stream_close_callback called\n");
  return 0;
}

static int on_frame_recv_callback(nghttp2_session *session,
	const nghttp2_frame *frame, void *user_data) 
{
	LOG("on_frame_recv_callback called\n");
	return 0;
}

static int 
on_data_chunk_recv_callback(nghttp2_session *session, uint8_t flags,
                                       int32_t stream_id, const uint8_t *data,
                                       size_t len, void *user_data) {
	LOG("on_data_chunk_recv_callback called\n");
  return 0;
}

#define TARGET_HOST "nghttp2.org"
#define TARGET_HOST_PORT "nghttp2.org:80"
#define TARGET_PATH "/httpbin/get"

int connect_to_server()
{
    struct addrinfo *result, *rp;
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
        ERR_EXIT("socket failed\n");

    if (getaddrinfo(TARGET_HOST, "http", NULL, &result))
        ERR_EXIT("getaddringo failed\n");

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) == -1) {
            fprintf(stderr, "%s\n", strerror(errno));
            struct sockaddr_in *sin = (struct sockaddr_in *)rp->ai_addr;
            fprintf(stderr, "Failed to connect to %s:%d\n",
                    inet_ntoa(sin->sin_addr), (int)ntohs(sin->sin_port));
            continue;
        }

        LOG("Successfully connected to " TARGET_HOST "\n");
        freeaddrinfo(result);
        return sfd;
    }

    ERR_EXIT("Failed to connect to " TARGET_HOST);
}

int main()
{
    int fd = connect_to_server();
    int status = 0;
	// Initialize library callbacks
	nghttp2_session_callbacks *callbacks;
	nghttp2_session_callbacks_new(&callbacks);

	nghttp2_session_callbacks_set_send_callback(callbacks, send_callback);
	nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, on_frame_recv_callback);
	nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, on_data_chunk_recv_callback);
	nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, on_stream_close_callback); 
       	nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback);
	nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, on_begin_headers_callback);
       	nghttp2_session_callbacks_set_recv_callback(callbacks, on_recv_callback);
	nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback); 

	// Create http2 session
	session_data_t data;
	status = nghttp2_session_client_new(&data.session, callbacks, &data);
    if (status)
        ERR_EXIT("nghttp2_session_client_new failed\n");


	// send_client_connection_header
	// client 24 bytes magic string will be sent by nghttp2 library 
	nghttp2_settings_entry iv[1] = {{NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}};
	if (nghttp2_submit_settings(data.session, NGHTTP2_FLAG_NONE, iv, ARRLEN(iv)))
		ERR_EXIT("nghttp2_submit_settings failed\n");
	

	// submit_request
	// curl -v --http2 http://nghttp2.org/httpbin/get
	nghttp2_nv hdrs[] = {
		MAKE_NV2(":method", "GET"),
		MAKE_NV2(":scheme", "http"),
		MAKE_NV2(":authority", TARGET_HOST_PORT),
		MAKE_NV2(":path", TARGET_PATH)
	};
	fprintf(stderr, "Request headers:\n");
	print_headers(stderr, hdrs, ARRLEN(hdrs));
	int32_t stream_id = nghttp2_submit_request(data.session, NULL, hdrs, ARRLEN(hdrs), NULL, NULL);
	if (stream_id < 0) 
		ERR_EXIT("Could not submit HTTP request");

	if (nghttp2_session_send(data.session))
		ERR_EXIT("Fatal error");

	// Clean up
	nghttp2_session_del(data.session);
	nghttp2_session_callbacks_del(callbacks);
    close(fd);
}
