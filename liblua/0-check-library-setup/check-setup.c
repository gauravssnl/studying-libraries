#include <stdlib.h>
#include <stdio.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../liblua_helpers.h"


#define PROGRAMM_DESCRIPTION \
    "Program to check that liblua is installed on the system \n" \
    " and print some general info about it.\n"

#define PROGRAMM_USAGE \
    "Usage: check-setup\n" 

#define SCRIPT_FILE "script.lua"


int main(void)
{
    printf("%s\n", PROGRAMM_DESCRIPTION);
    printf("%s\n", PROGRAMM_USAGE);

    // printf("Current lua versionn", _VERSION);

    int status;
    double script_result;
    lua_State *L;

    /*
     * All Lua contexts are held in this structure. We work with it almost
     * all the time.
     */
    L = luaL_newstate();
    if (!L)
        SYS_LOG_ERROR_AND_EXIT("luaL_newstate failed");

    /* Load standard Lua libraries: */
    luaL_openlibs(L);

    /* Load file with the script */
    status = luaL_loadfile(L, SCRIPT_FILE);
    if (status) {
        /* If something went wrong, error message is at the top of */
        /* the stack */
        SYS_LOG_ERROR_AND_EXIT("Couldn't load file %s: %s\n",
            SCRIPT_FILE, lua_tostring(L, -1));
    }
        
    /* Run script */
    status = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (status) {
        SYS_LOG_ERROR_AND_EXIT("Failed to run script %s: %s\n",
            SCRIPT_FILE, lua_tostring(L, -1));
    }

    /* Get the returned value at the top of the stack (index -1) */
    script_result = lua_tonumber(L, -1);
    LOG_INFO("Lua script returned: %.0f\n", script_result);

    lua_pop(L, 1);  /* Take the returned value out of the stack */
    lua_close(L);   /* Exit, Lua */


    return 0;
}