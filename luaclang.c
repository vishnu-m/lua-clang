#include <clang-c/Index.h>
#include <stdbool.h>
#include <unistd.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define PARSER_METATABLE  "Clang.Parser"
#define CURSOR_METATABLE "Clang.Cursor"
#define TYPE_METATABLE   "Clang.Type"

#define new_object(L, ptr, mt) {\
	ptr = (typeof(ptr)) lua_newuserdata(L, sizeof(*ptr)); \
	luaL_getmetatable(L, mt); \
	lua_setmetatable(L, -2); }

#define to_object(L, ptr, mt, n) {\
        ptr = (typeof(ptr)) luaL_checkudata(L, n, mt); }

typedef struct clang_parser {
        CXIndex idx;
        CXTranslationUnit tu;
} clang_parser;

/* --Clang functions-- */

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
        clang_parser *parser;
        new_object(L, parser, PARSER_METATABLE);
        parser->idx = clang_createIndex(1, 0);
        luaL_argcheck(L, parser->idx != NULL, 1, "index wasn't created");
        const char *args[] = {file_name};
        parser->tu = clang_parseTranslationUnit(parser->idx, 0, args, 1, 0, 0, CXTranslationUnit_None);
        luaL_argcheck(L, parser->tu != NULL, 1, "translation unit wasn't created");
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
        clang_parser *parser;
        to_object(L, parser, PARSER_METATABLE, 1);
        if (parser->idx == NULL) return 0;
        clang_disposeIndex(parser->idx);
        clang_disposeTranslationUnit(parser->tu);
        parser->idx = NULL;
        parser->tu = NULL;
        return 0;
}

/*      
        Format - parser:getCursor()
        Parameter - parser - Clang object whose translation unit cursor is to be obtained 
        More info - https://clang.llvm.org/doxygen/group__CINDEX__CURSOR__MANIP.html#gaec6e69127920785e74e4a517423f4391
        Returns
*/
static int get_cursor(lua_State *L) 
{
        clang_parser *parser;
        to_object(L, parser, PARSER_METATABLE, 1);
        luaL_argcheck(L, parser->tu != NULL, 1, "parser object was disposed");
        CXCursor *cur;
        new_object(L, cur, CURSOR_METATABLE);
        *cur = clang_getTranslationUnitCursor(parser->tu);
        if (clang_Cursor_isNull(*cur)) {
                lua_pushnil(L);
        }
        return 1;
}

/* --Cursor functions-- */

/*      
        Format - cur:getSpelling()
        Parameter - cur - Cursor whose name is to be obtained    
        More info - https://clang.llvm.org/doxygen/group__CINDEX__CURSOR__XREF.html#gaad1c9b2a1c5ef96cebdbc62f1671c763
*/
static int get_cursor_spelling(lua_State *L) 
{
        CXCursor *cur;
        to_object(L, cur, CURSOR_METATABLE, 1);
        CXString name = clang_getCursorSpelling(*cur);
        lua_pushstring(L, clang_getCString(name));
        clang_disposeString(name);
        return 1;
}

void new_metatable(lua_State *L, const char *name, luaL_Reg *reg)
{
        luaL_newmetatable(L, name);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        luaL_setfuncs(L, reg, 0);
        lua_pop(L, 1);
}

static luaL_Reg clang_functions[] = {
        {"newParser", create_clangparser},
        {NULL, NULL}
};

static luaL_Reg parser_functions[] = {
        {"disposeParser", dispose_clangobject},
        {"getCursor", get_cursor},
        {"__gc", dispose_clangobject},
        {NULL, NULL}
};

static luaL_Reg cursor_functions[] = {
        {"getSpelling", get_cursor_spelling},
        {NULL, NULL}
};

int luaopen_luaclang(lua_State *L) 
{
        new_metatable(L, PARSER_METATABLE, parser_functions);
        new_metatable(L, CURSOR_METATABLE, cursor_functions);
        lua_newtable(L);
        luaL_setfuncs(L, clang_functions, 0);
        return 1;
}