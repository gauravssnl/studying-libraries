#include <stdlib.h>
#include <stdio.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../liblua_helpers.h"


#define PROGRAMM_DESCRIPTION \
    "Simple interpreter of lua code.\n"

#define PROGRAMM_USAGE \
    "Usage: interpreter\n" 

int main(void)
{
    printf("%s\n", PROGRAMM_DESCRIPTION);
    printf("%s\n", PROGRAMM_USAGE);

   
    // Start our Lua run-time state and load the standard libraries.
    lua_State *L = luaL_newstate();
    if (!L)
        SYS_LOG_ERROR_AND_EXIT("luaL_newstate failed");

    luaL_openlibs(L);
  
    printf("\nWrite some expressions:\n");
    // Read-parse(load)-evaluate(call). In a loop.
    char buff[2048];
    while ((fgets(buff, sizeof(buff), stdin))) {
        luaL_loadstring(L, buff);
        lua_pcall(L, 0, 0, 0);
    }
  
    // Clean up the lua run-time state and exit.
    lua_close(L);


    return 0;
}