
APP_NAME=echo-server

all: ${APP_NAME}

${APP_NAME}: ${APP_NAME}.c ../libevent_helpers.c ../libevent_helpers.h
	gcc ${APP_NAME}.c ../libevent_helpers.c -Wall -Wextra -g3 -o ${APP_NAME} -levent 

clean:
	rm -f ${APP_NAME}