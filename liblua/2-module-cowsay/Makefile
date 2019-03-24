
APP_NAME=module-cowsay

LUA_INC_DIR=/usr/include/lua5.3 #redefine if necessary
LUA_LIB=lua5.3                  #redefine if necessary

all: ${APP_NAME}

${APP_NAME}: ${APP_NAME}.c ../liblua_helpers.c ../liblua_helpers.h
	gcc -shared -fPIC ${APP_NAME}.c ../liblua_helpers.c -Wall -Wextra -g3 -l${LUA_LIB} -I${LUA_INC_DIR} -o libcowsay.so

clean:
	rm -f libcowsay.so