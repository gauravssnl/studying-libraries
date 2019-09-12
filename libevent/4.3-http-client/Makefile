
APP_NAME=http-client

all: ${APP_NAME}

${APP_NAME}: ${APP_NAME}.c ../libevent_helpers.c ../libevent_helpers.h
	gcc ${APP_NAME}.c ../libevent_helpers.c -Wall -Wextra -g3 -fsanitize=address -o ${APP_NAME} -levent -lasan

clean:
	rm -f ${APP_NAME}
