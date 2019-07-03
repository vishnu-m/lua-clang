#include <clang-c/Index.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

static const struct luaL_Reg luaclang [] = {
      {NULL, NULL}  /* sentinel */
};

int luaopen_luaclang (lua_State *L)
{
    luaL_newlib(L, luaclang);
    return 1;
}