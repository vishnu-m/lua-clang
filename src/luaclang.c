#include <clang-c/Index.h>
#include <stdbool.h>
#include <stdlib.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define INDEX_METATABLE  "Clang.Index"
#define TU_METATABLE     "Clang.TU"
#define CURSOR_METATABLE "Clang.Cursor"
#define TYPE_METATABLE   "Clang.Type"

/* Create CXIndex as userdata */

static CXIndex * new_CXIndex(lua_State *L) 
{
        CXIndex *idx = (CXIndex*) lua_newuserdata(L, sizeof(CXIndex));
        luaL_getmetatable(L, INDEX_METATABLE);
        lua_setmetatable(L, -2);
        return idx;
}

/* Convert userdata type to CXIndex */

static CXIndex * to_CXIndex(lua_State *L, int n)
{
        CXIndex *idx = (CXIndex*) luaL_checkudata(L, n, INDEX_METATABLE);
        return idx;
}

/* Create CXTranslatioUnit as userdata */

static CXTranslationUnit * new_CXTU(lua_State *L)
{
        CXTranslationUnit *tu = (CXTranslationUnit*) lua_newuserdata(L, sizeof(CXTranslationUnit));
        luaL_getmetatable(L, TU_METATABLE);
        lua_setmetatable(L, -2);
        return tu;
}

/* Convert userdata type to CXTranslatioUnit */

static CXTranslationUnit * to_CXTU(lua_State *L, int n) 
{
        CXTranslationUnit *tu = (CXTranslationUnit*) luaL_checkudata(L, n, TU_METATABLE);
        return tu;
}

/* Create CXCursor as userdata */

static CXCursor * new_CXCursor(lua_State *L) 
{
        CXCursor *cur = (CXCursor*) lua_newuserdata(L, sizeof(CXCursor));
        luaL_getmetatable(L, CURSOR_METATABLE);
        lua_setmetatable(L, -2);
        return cur;
}

/* Convert userdata type to CXCursor */

static CXCursor * to_CXCursor(lua_State *L, int n) 
{
        CXCursor *c = (CXCursor*) luaL_checkudata(L, n, CURSOR_METATABLE);
        return c;
}

/* Create CXType as userdata */

static CXType * new_CXType(lua_State *L)
{
        CXType *t = (CXType*) lua_newuserdata(L, sizeof(CXType));
        luaL_getmetatable(L, TYPE_METATABLE);
        lua_setmetatable(L, -2);
        return t;
}


/* --Clang function-- */

/*      
        Format - createIndex(exclude_pch, diagnostics)
        Parameters - 1. exclude_pch - When non-zero, allows enumeration of "local" declarations
                     2. diagnostics - Display diagnostics
        More info - https://clang.llvm.org/doxygen/group__CINDEX.html#ga51eb9b38c18743bf2d824c6230e61f93
*/
static int create_CXIndex(lua_State *L) 
{
        int exclude_pch = lua_toboolean(L, 1);
        int diagnostics = lua_toboolean(L, 2);
        CXIndex *idx = new_CXIndex(L);
        *idx = clang_createIndex(exclude_pch, diagnostics);
        return 1;
}

static luaL_Reg clang_function[] = {
        {"createIndex", create_CXIndex},
        {NULL, NULL}
};


/* --Index functions-- */

/*      
        Format - idx:disposeIndex()
        Parameter -  idx - Index to be destroyed       
        More info - https://clang.llvm.org/doxygen/group__CINDEX.html#ga166ab73b14be73cbdcae14d62dbab22a
*/
static int dispose_CXIndex(lua_State *L) 
{
        CXIndex *idx = to_CXIndex(L, 1);
        clang_disposeIndex(*idx);
        return 0;
}

/*      
        Format - idx:parseTU(file_name)
        Parameters - 1. idx - The index object with which the translation unit will be associated  
                     2. file_name - The name of the source file to load    
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TRANSLATION__UNIT.html#ga2baf83f8c3299788234c8bce55e4472e
*/
static int parse_TU(lua_State *L)
{
        CXIndex *idx = to_CXIndex(L, 1);
        const char *file_name = lua_tostring(L, 2);
        const char *args[] = {file_name};
        CXTranslationUnit *tu = new_CXTU(L);
        *tu = clang_parseTranslationUnit(*idx, 0, args, 1, 0, 0, CXTranslationUnit_None);
        return 1;
}


static luaL_Reg index_functions[] = {
        {"disposeIndex", dispose_CXIndex},
        {"parseTU", parse_TU},
        {"__gc", dispose_CXIndex},
        {NULL, NULL}
};


/* --Translation unit functions-- */

/*      
        Format - tu:disposeTU()
        Parameter - tu - Translation unit to be destroyed       
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TRANSLATION__UNIT.html#gaee753cb0036ca4ab59e48e3dff5f530a
*/
static int dispose_CXTU(lua_State *L) 
{
        CXTranslationUnit *tu = to_CXTU(L, 1);
        clang_disposeTranslationUnit(*tu);
        return 0;
}

/*      
        Format - tu:getTUCursor()
        Parameter - tu - Translation unit of which the cursor represents     
        More info - https://clang.llvm.org/doxygen/group__CINDEX__CURSOR__MANIP.html#gaec6e69127920785e74e4a517423f4391
*/
static int get_TU_cursor(lua_State *L) 
{
        CXTranslationUnit *tu = to_CXTU(L, 1);
        CXCursor* cur = new_CXCursor(L);
        *cur = clang_getTranslationUnitCursor(*tu);
        if (clang_Cursor_isNull(*cur)) {
                lua_pushnil(L);
        }
        return 1;
}


static luaL_Reg tu_functions[] = {
        {"disposeTU", dispose_CXTU},
        {"getTUCursor", get_TU_cursor},
        {"__gc", dispose_CXTU},
        {NULL, NULL}
};


/* --Cursor functions-- */

/*      
        Format - cur:getCursorSpelling()
        Parameter - cur - Cursor whose name is to be obtained    
        More info - https://clang.llvm.org/doxygen/group__CINDEX__CURSOR__XREF.html#gaad1c9b2a1c5ef96cebdbc62f1671c763
*/
static int get_cursor_spelling(lua_State *L) 
{
        CXCursor *cur = to_CXCursor(L, 1);
        CXString name = clang_getCursorSpelling(*cur);
        lua_pushstring(L, clang_getCString(name));
        clang_disposeString(name);
        return 1;
}

static luaL_Reg cursor_functions[] = {
        {"getCursorSpelling", get_cursor_spelling},
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
        new_metatable(L, INDEX_METATABLE, index_functions);
        new_metatable(L, TU_METATABLE, tu_functions);
        new_metatable(L, CURSOR_METATABLE, cursor_functions);

        lua_newtable(L);
        luaL_setfuncs(L, clang_function, 0);

        return 1;
}