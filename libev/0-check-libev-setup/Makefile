
APP_NAME=check-setup

all: ${APP_NAME}

${APP_NAME}: ${APP_NAME}.c ../libev_helpers.c ../libev_helpers.h
	gcc ${APP_NAME}.c ../libev_helpers.c -Wall -Wextra -g3 -o ${APP_NAME} -lev

clean:
	rm -f ${APP_NAME}