#include <event2/event.h>
#include <event2/buffer.h>
#include <stdio.h>
#include <assert.h>
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


#define PROGRAMM_DESCRIPTION \
    "Simple tests for evbuffer.\n" \
    " This application is a simple example of `evbuffer` usage.\n"

#define PROGRAMM_USAGE \
    "Usage: evbuffers\n" \

unsigned char strbuffer[1024];

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    printf("%s\n", PROGRAMM_DESCRIPTION);
    printf("%s\n", PROGRAMM_USAGE);

    struct evbuffer *buf = evbuffer_new();

    printf("\nAdding `foo` to the end of  evbuffer\n");
    evbuffer_add(buf, "foo", strlen("foo"));
    printf(" Evbuffer: length: %u, cont. space: %u\n",
        (unsigned)evbuffer_get_length(buf),
        (unsigned)evbuffer_get_contiguous_space(buf));
    assert(evbuffer_get_length(buf) == 3);

    printf("\nAdding 'Hello world 2.0.1' to the end of evbuffer\n");
    evbuffer_add_printf(buf, "Hello %s %d.%d.%d", "world", 2, 0, 1);
    printf(" Evbuffer: length: %u, cont. space: %u\n",
        (unsigned)evbuffer_get_length(buf),
        (unsigned)evbuffer_get_contiguous_space(buf));
    assert(evbuffer_get_length(buf) == 20);

    printf("\nAdding `Front msg` to the front of evbuffer\n");
    evbuffer_prepend(buf, "Front msg", strlen("Front msg"));
    printf(" Evbuffer: length: %u, cont. space: %u\n",
        (unsigned)evbuffer_get_length(buf),
        (unsigned)evbuffer_get_contiguous_space(buf));
    assert(evbuffer_get_length(buf) == 29);

    printf("\nMaking 'evbuffer pullup' - linearize content\n");
    unsigned char *content = evbuffer_pullup(buf, -1);
    memset(strbuffer, 0, sizeof(strbuffer));
    memcpy(strbuffer, content, 29);
    printf(" Evbuffer content: %s\n", strbuffer);
    printf(" Evbuffer: length: %u, cont. space: %u\n",
        (unsigned)evbuffer_get_length(buf),
        (unsigned)evbuffer_get_contiguous_space(buf));
    assert(evbuffer_get_length(buf) == 29);

    printf("\nRemove first 9 bytes\n");
    memset(strbuffer, 0, sizeof(strbuffer));
    evbuffer_remove(buf, strbuffer, 9);
    printf(" Removed content: %s\n", strbuffer);
    content = evbuffer_pullup(buf, -1);
    memset(strbuffer, 0, sizeof(strbuffer));
    memcpy(strbuffer, content, 29);
    printf(" Evbuffer content after remove: %s\n", strbuffer);
    printf(" Evbuffer: length: %u, cont. space: %u\n",
        (unsigned)evbuffer_get_length(buf),
        (unsigned)evbuffer_get_contiguous_space(buf));
    assert(evbuffer_get_length(buf) == 20);

    printf("\nCopy first 10 bytes to some memory\n");
    memset(strbuffer, 0, sizeof(strbuffer));
    evbuffer_copyout(buf, strbuffer, 10);
    printf(" Copied content: %s\n", strbuffer);
    printf(" Evbuffer: length: %u, cont. space: %u\n",
        (unsigned)evbuffer_get_length(buf),
        (unsigned)evbuffer_get_contiguous_space(buf));
    assert(evbuffer_get_length(buf) == 20);

    printf("\nSearch string 'Hello' within buffer\n");
    struct evbuffer_ptr pos = evbuffer_search(buf, "Hello", strlen("Hello"), NULL);
    printf("'Hello' pos %u\n", (unsigned)pos.pos);
    assert(pos.pos == 3);

    evbuffer_free(buf);




    printf("\n======  evbuffer - parsing line-oriented input==========\n");
    const char *HTTP_REQUEST = 
        "GET /hello.htm HTTP/1.1\r\n"
        "User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\n"
        "Host: www.tutorialspoint.com\r\n"
        "Accept-Language: en-us\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Connection: Keep-Alive\r\n";
    printf("Example of livevent line-oriented input parsing.\nData to parse:\n%s\n", HTTP_REQUEST);
    struct evbuffer *http_buffer = evbuffer_new();
    evbuffer_add(http_buffer, HTTP_REQUEST, strlen(HTTP_REQUEST));
    printf("Result of parsing:\n");
    int line_n = 0;
    char *request_line;
    size_t len;

    while ((request_line = evbuffer_readln(http_buffer, &len,   EVBUFFER_EOL_CRLF))) {
        printf("line %d: %s\n", line_n++, request_line);
    }
    evbuffer_free(http_buffer);

    return 0;
}