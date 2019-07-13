#include <clang-c/Index.h>
#include <stdbool.h>
#include <unistd.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define CLANG_METATABLE  "Clang.Parser"
#define CURSOR_METATABLE "Clang.Cursor"
#define TYPE_METATABLE   "Clang.Type"

typedef struct clang_parser {
        CXIndex idx;
        CXTranslationUnit tu;
} clang_parser;

/* Create clang_parser as userdata */
static clang_parser *new_clangparser(lua_State *L) 
{
        clang_parser *parser = (clang_parser*) lua_newuserdata(L, sizeof(clang_parser));
        luaL_getmetatable(L, CLANG_METATABLE);
        lua_setmetatable(L, -2);
        return parser;
}

/* Convert userdata type to clang_parser object */
static clang_parser *to_clangparser(lua_State *L, int n) 
{
        clang_parser *parser = (clang_parser*) luaL_checkudata(L, n, CLANG_METATABLE);
        return parser;
}

/*      
        Format - newParser(file_name)
        Parameter - file_name - The name of the source file to load 
        More info - 1. https://clang.llvm.org/doxygen/group__CINDEX.html#ga51eb9b38c18743bf2d824c6230e61f93
                    2. https://clang.llvm.org/doxygen/group__CINDEX__TRANSLATION__UNIT.html#ga2baf83f8c3299788234c8bce55e4472e
        Returns clang object whose translation unit cursor can be obtained.
*/
static int create_clangparser(lua_State *L)
{
        const char *file_name = lua_tostring(L, 1);
        if (access(file_name, F_OK) == -1) {
             return luaL_error(L, "file doesn't exist");    
        }
        clang_parser *parser = new_clangparser(L);
        parser->idx = clang_createIndex(1, 0);
        const char *args[] = {file_name};
        parser->tu = clang_parseTranslationUnit(parser->idx, 0, args, 1, 0, 0, CXTranslationUnit_None);
        return 1;
}

/*      
        Format - parser:disposeParser()
        Parameter - parser - Clang object to be disposed
        More info - 1. https://clang.llvm.org/doxygen/group__CINDEX.html#ga51eb9b38c18743bf2d824c6230e61f93
                    2. https://clang.llvm.org/doxygen/group__CINDEX__TRANSLATION__UNIT.html#ga2baf83f8c3299788234c8bce55e4472e
        Returns nothing
*/
static int dispose_clangobject(lua_State *L)
{
        clang_parser *parser = to_clangparser(L, 1);
        if (parser->idx == NULL) return 0;
        clang_disposeIndex(parser->idx);
        clang_disposeTranslationUnit(parser->tu);
        parser->idx = NULL;
        parser->tu = NULL;
        return 0;
}

/* Create CXCursor as userdata */
static inline CXCursor *new_CXCursor(lua_State *L) 
{
        CXCursor *cur = (CXCursor*) lua_newuserdata(L, sizeof(CXCursor));
        luaL_getmetatable(L, CURSOR_METATABLE);
        lua_setmetatable(L, -2);
        return cur;
}

/* Convert userdata type to CXCursor */
static CXCursor to_CXCursor(lua_State *L, int n) 
{
        CXCursor *c = (CXCursor*) luaL_checkudata(L, n, CURSOR_METATABLE);
        return *c;
}

/*      
        Format - parser:getCursor()
        Parameter - parser - Clang object whose translation unit cursor is to be obtained 
        More info - https://clang.llvm.org/doxygen/group__CINDEX__CURSOR__MANIP.html#gaec6e69127920785e74e4a517423f4391
        Returns
*/
static int get_cursor(lua_State *L) 
{
        clang_parser *parser = to_clangparser(L, 1);
        CXCursor* cur = new_CXCursor(L);
        luaL_argcheck(L, parser->tu != NULL, 1, "parser object was disposed");
        *cur = clang_getTranslationUnitCursor(parser->tu);
        if (clang_Cursor_isNull(*cur)) {
                lua_pushnil(L);
        }
        return 1;
}


static luaL_Reg clang_functions[] = {
    {"newParser", create_clangparser},
    {"disposeParser", dispose_clangobject},
    {"getCursor", get_cursor},
    {"__gc", dispose_clangobject},
    {NULL, NULL}
};

/* --Cursor functions-- */

/*      
        Format - cur:getSpelling()
        Parameter - cur - Cursor whose name is to be obtained    
        More info - https://clang.llvm.org/doxygen/group__CINDEX__CURSOR__XREF.html#gaad1c9b2a1c5ef96cebdbc62f1671c763
*/
static int get_cursor_spelling(lua_State *L) 
{
        CXCursor cur = to_CXCursor(L, 1);
        CXString name = clang_getCursorSpelling(cur);
        lua_pushstring(L, clang_getCString(name));
        clang_disposeString(name);
        return 1;
}


static luaL_Reg cursor_functions[] = {
        {"getSpelling", get_cursor_spelling},
        {NULL, NULL}
};

void new_metatable(lua_State *L, const char *name, luaL_Reg *reg)
{
        luaL_newmetatable(L, name);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        luaL_setfuncs(L, reg, 0);
        lua_pop(L, 1);
}

int luaopen_luaclang(lua_State *L) 
{
        new_metatable(L, CLANG_METATABLE, clang_functions);
        new_metatable(L, CURSOR_METATABLE, cursor_functions);
        lua_newtable(L);
        luaL_setfuncs(L, clang_functions, 0);
        return 1;
}
