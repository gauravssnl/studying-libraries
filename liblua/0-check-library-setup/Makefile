
APP_NAME=check-setup

LUA_INC_DIR=/usr/include/lua5.3 #redefine if necessary
LUA_LIB=lua5.3                  #redefine if necessary

all: ${APP_NAME}

${APP_NAME}: ${APP_NAME}.c ../liblua_helpers.c ../liblua_helpers.h
	gcc ${APP_NAME}.c ../liblua_helpers.c -Wall -Wextra -g3 -o ${APP_NAME} -l${LUA_LIB} -I${LUA_INC_DIR}

clean:
	rm -f ${APP_NAME}