
APP_NAME=signals

all: ${APP_NAME}

${APP_NAME}: signals.c
	gcc signals.c -Wall -Wextra -g3 -o ${APP_NAME} -levent

clean:
	rm -f ${APP_NAME}