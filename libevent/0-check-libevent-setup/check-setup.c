#include <event2/event.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <unistd.h>
#include <fcntl.h>

#include "../libevent_helpers.h"


#define PROGRAMM_DESCRIPTION \
    "Program to check that libevent is installed on the system \n" \
    " and print some general info about it.\n"

#define PROGRAMM_USAGE \
    "Usage: check-setup\n" \

int main()
{
    printf("%s\n", PROGRAMM_DESCRIPTION);
    printf("%s\n", PROGRAMM_USAGE);

    printf("\nCurrent libevent version: %s\n", event_get_version());

    const char **sup_methods = event_get_supported_methods();
    printf("\nLibevent supported methods:\n");
    for (int i = 0; sup_methods[i] != NULL; ++i) {
        printf("    %s\n", sup_methods[i]);
    }

    /* Create event base and default info */
    struct event_base *base = event_base_new();
    if (!base) {
        puts("Couldn't get an event_base!");
    } else {
        printf("\nUsing Libevent with default backend method %s.\n",
            event_base_get_method(base));
        enum event_method_feature f = event_base_get_features(base);
        if ((f & EV_FEATURE_ET))
            printf("  Edge-triggered events are supported.\n");
        if ((f & EV_FEATURE_O1))
            printf("  O(1) event notification is supported.\n");
        if ((f & EV_FEATURE_FDS))
            printf("  All FD types are supported.\n");
        puts("");
    }



    return 0;
}