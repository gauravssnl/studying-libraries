
APP_NAME=simple-close-server

all: ${APP_NAME}

${APP_NAME}: ${APP_NAME}.c
	gcc ${APP_NAME}.c -Wall -Wextra -g3 -o ${APP_NAME} -levent

clean:
	rm -f ${APP_NAME}