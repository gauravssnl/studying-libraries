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
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

/* ---- HELPERS ----- */

#define LOG_INFO(...) fprintf(stderr, __VA_ARGS__)

#define ERR_EXIT(...) \
	do { \
		fprintf(stderr, __VA_ARGS__); \
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

const char *
frame_type_to_str(nghttp2_frame_type t)
{
	switch (t) {
	case NGHTTP2_DATA: return "NGHTTP2_DATA";
	case NGHTTP2_HEADERS: return "NGHTTP2_HEADERS";
	case NGHTTP2_PRIORITY: return "NGHTTP2_PRIORITY";
	case NGHTTP2_RST_STREAM: return "NGHTTP2_RST_STREAM";
	case NGHTTP2_SETTINGS: return "NGHTTP2_SETTINGS";
	case NGHTTP2_PUSH_PROMISE: return "NGHTTP2_PUSH_PROMISE";
	case NGHTTP2_PING: return "NGHTTP2_PING";
	case NGHTTP2_GOAWAY: return "NGHTTP2_GOAWAY";
	case NGHTTP2_WINDOW_UPDATE: return "NGHTTP2_WINDOW_UPDATE";
	case NGHTTP2_CONTINUATION: return "NGHTTP2_CONTINUATION";
	case NGHTTP2_ALTSVC: return "NGHTTP2_ALTSVC";
	default: return "undefined";
	}
}

static void 
print_header(FILE *f, int padding, const uint8_t *name, size_t namelen,
	const uint8_t *value, size_t valuelen) 
{
	while (padding--)
		fputc(' ', f);
	fwrite(name, 1, namelen, f);
	fprintf(f, ": ");
	fwrite(value, 1, valuelen, f);
	fprintf(f, "\n");
}

static void 
print_headers(FILE *f, nghttp2_nv *nva, size_t nvlen) {
	size_t i;
	for (i = 0; i < nvlen; ++i)
		print_header(f, 2, nva[i].name, nva[i].namelen, nva[i].value, nva[i].valuelen);
	fprintf(f, "\n");
}


/* ---- DATA TYPES ----- */

typedef struct {
	int stream_id;
} stream_data_t;

typedef struct {
	int some_data;
	nghttp2_session *session;
	int con_fd;
	stream_data_t stream_data;
	SSL *ssl;
} session_data_t;


/* ---- CODE ----- */

/* nghttp2_send_callback. Here we transmit the |data|, |length| bytes,
   to the network. */
static ssize_t 
on_send_callback(nghttp2_session *session, const uint8_t *data,
	size_t length, int flags, void *user_data) 
{
	LOG_INFO("on_send_callback called (data = %p; length = %u)\n", data, (unsigned) length);
	session_data_t *session_data = (session_data_t *)user_data;
	int written = 0;
	int w = 0;
	while (length - written > 0) {
		w = SSL_write(session_data->ssl, data + written, length - written);
		if (w < 0)
			ERR_EXIT("SSL_write failed\n");
		written += w;
	}
	LOG_INFO("  sent %d bytes\n", written);

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
	LOG_INFO("  on_header_callback called\n");
	session_data_t *session_data = (session_data_t *)user_data;

	switch (frame->hd.type) {
	case NGHTTP2_HEADERS:
		if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE &&
			session_data->stream_data.stream_id == frame->hd.stream_id) {
			/* Print response headers for the initiated request. */
			print_header(stderr, 4, name, namelen, value, valuelen);
			break;
		}
	}
	return 0;
}

/* nghttp2_on_begin_headers_callback: Called when nghttp2 library gets
   started to receive header block. */
static int 
on_begin_headers_callback(nghttp2_session *session,
     const nghttp2_frame *frame,
     void *user_data) 
{
	LOG_INFO("  on_begin_headers_callback called\n");
	session_data_t *session_data = (session_data_t *)user_data;

	switch (frame->hd.type) {
	case NGHTTP2_HEADERS:
		if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE &&
			session_data->stream_data.stream_id == frame->hd.stream_id) {
			LOG_INFO("    Response headers for stream ID=%d:\n", frame->hd.stream_id);
		}
		break;
	}
	return 0;
}

static ssize_t 
on_recv_callback(nghttp2_session *session, uint8_t *buf, size_t length, int flags, void *user_data)
{
	LOG_INFO("on_recv_callback called\n");
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
	LOG_INFO("  on_stream_close_callback called\n");
	session_data_t *session_data = (session_data_t *)user_data;
	if (session_data->stream_data.stream_id == stream_id) {
		LOG_INFO("    Stream %d closed with error_code=%u\n", stream_id, error_code);
		int rv = nghttp2_session_terminate_session(session, NGHTTP2_NO_ERROR);
		if (rv != 0) 
			return NGHTTP2_ERR_CALLBACK_FAILURE;
	}

	LOG_INFO("Shutting down tls session\n");
	while (1) {
		int r = SSL_shutdown(session_data->ssl);
		if (r < 0)
			ERR_EXIT("SSL_shutdown failed\n");
		if (r == 1)
			break;
	}
	LOG_INFO("Tls session successfully shut down\n");
	close(session_data->con_fd);
	session_data->con_fd = -1;
	return 0;
}

static int on_frame_recv_callback(nghttp2_session *session,
	const nghttp2_frame *frame, void *user_data) 
{
	LOG_INFO("  on_frame_recv_callback called(type = %s)\n", frame_type_to_str(frame->hd.type));
	session_data_t *session_data = (session_data_t *)user_data;
	int session_stream_id = session_data->stream_data.stream_id;
	switch (frame->hd.type) {
	case NGHTTP2_HEADERS:
		if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE && session_stream_id == frame->hd.stream_id) 
			LOG_INFO("    All headers received\n");
		break;
	case NGHTTP2_DATA:
		LOG_INFO("    DATA FRAME\n");
		break;
	}
	return 0;
}

static int 
on_data_chunk_recv_callback(nghttp2_session *session, uint8_t flags,
                                       int32_t stream_id, const uint8_t *data,
                                       size_t len, void *user_data) 
{
	LOG_INFO("  on_data_chunk_recv_callback called\n");
	session_data_t *session_data = (session_data_t *)user_data;

	if (session_data->stream_data.stream_id == stream_id) {
		LOG_INFO("    received data: ");
		fwrite(data, 1, len, stderr);
		LOG_INFO("\n");
	}
	return 0;
}

#define TARGET_HOST "nghttp2.org"
#define TARGET_HOST "ya.ru"
#define TARGET_HOST_PORT TARGET_HOST ":443"
#define TARGET_PATH "/httpbin/get"
#define TARGET_SERVICE "https"

int connect_to_server()
{
    struct addrinfo *result, *rp;
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
        ERR_EXIT("socket failed\n");

    if (getaddrinfo(TARGET_HOST, TARGET_SERVICE, NULL, &result))
        ERR_EXIT("getaddringo failed\n");

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) == -1) {
            fprintf(stderr, "%s\n", strerror(errno));
            struct sockaddr_in *sin = (struct sockaddr_in *)rp->ai_addr;
            fprintf(stderr, "Failed to connect to %s:%d\n",
                    inet_ntoa(sin->sin_addr), (int)ntohs(sin->sin_port));
            continue;
        }

        LOG_INFO("Successfully connected to " TARGET_HOST "\n");
        freeaddrinfo(result);
        return sfd;
    }

    ERR_EXIT("Failed to connect to " TARGET_HOST);
}

int main()
{
	session_data_t data;
	int status = 0;
	SSL_CTX *ssl_ctx;

	// Connect to server
	memset(&data, '\0', sizeof(data));
	data.con_fd = connect_to_server();

	// Establish tls session
	SSL_load_error_strings();   
	SSL_library_init();
	OpenSSL_add_all_algorithms();  
	if (RAND_poll() == 0)
		ERR_EXIT("RAND_poll faile\n");
	ssl_ctx = SSL_CTX_new(SSLv23_method());
	if (!ssl_ctx)
		ERR_EXIT("SSL_CTX_new faile\n");
	//if (SSL_CTX_set_alpn_protos(ssl_ctx, (const unsigned char *)"\x02h2", 3))
	if (SSL_CTX_set_alpn_protos(ssl_ctx, (const unsigned char *)"\x02h2\x08http/1.1", 12))
		ERR_EXIT("SSL_CTX_set_alpn_protos faile\n");

	data.ssl = SSL_new(ssl_ctx);
	if (!data.ssl)
		ERR_EXIT("SSL_new failed\n");
	if (SSL_set_fd(data.ssl, data.con_fd) == 0)
		ERR_EXIT("SSL_set_fd failed\n");
	if (SSL_connect(data.ssl) == -1)   /* perform the connection */
		ERR_EXIT("SSL_connect failed\n");
	LOG_INFO("SSL session established\n");
	const unsigned char *alpn = NULL;
	unsigned int alpnlen = 0;
	SSL_get0_alpn_selected(data.ssl, &alpn, &alpnlen);
	if (alpn == NULL || alpnlen != 2 || memcmp("h2", alpn, 2) != 0) 
		ERR_EXIT("h2 is not negotiated (alpn = %p; alpnlen = %u)\n", alpn, alpnlen);
	LOG_INFO("Http2 negotiated during ssl handshake via alpn\n");


	// Initialize library callbacks
	nghttp2_session_callbacks *callbacks;
	nghttp2_session_callbacks_new(&callbacks);

	nghttp2_session_callbacks_set_send_callback(callbacks, on_send_callback);
	nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, on_frame_recv_callback);
	nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, on_data_chunk_recv_callback);
	nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, on_stream_close_callback); 
       	nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback);
	nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, on_begin_headers_callback);
       	nghttp2_session_callbacks_set_recv_callback(callbacks, on_recv_callback);
	nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback); 

	// Create http2 session
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
	data.stream_data.stream_id = nghttp2_submit_request(data.session, NULL, hdrs, ARRLEN(hdrs), NULL, NULL);
	if (data.stream_data.stream_id < 0) 
		ERR_EXIT("Could not submit HTTP request");
	LOG_INFO("Created http2 stream (id = %d)\n\n", data.stream_data.stream_id);

	LOG_INFO("Sending data to http2 stream\n");
	if (nghttp2_session_send(data.session))
		ERR_EXIT("Fatal error");

	// Reading response from server
	LOG_INFO("\nReading data from http2 stream\n");
#define BUF_SZ 256
	unsigned char buffer[BUF_SZ];
	int r = 0;
	int readlen;
	while (data.con_fd >= 0) { // we set con_fd = -1 in callbacks when we are done
		r = SSL_read(data.ssl, buffer, BUF_SZ);
		if (r < 0)
			ERR_EXIT("SSL_read error\n");
		if (r == 0)
			break;
		LOG_INFO(" Read %d bytes from http2 stream\n", (int)r);
		readlen = nghttp2_session_mem_recv(data.session, buffer, r);
		if (readlen < 0)
			ERR_EXIT("nghttp2_session_mem_recv failed\n");
	}

	// Clean up
	if (data.con_fd >= 0) {
		SSL_shutdown(data.ssl);
		close(data.con_fd);
	}
	SSL_free(data.ssl);
	SSL_CTX_free(ssl_ctx);
	nghttp2_session_del(data.session);
	nghttp2_session_callbacks_del(callbacks);
}
