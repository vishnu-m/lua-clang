#include <clang-c/Index.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define INDEX  "ClangIndex"
#define TU     "ClangTU"
#define CURSOR "ClangCursor"
#define STRING "ClangString"
#define TYPE "ClangType"

static CXIndex* new_CXIndex(lua_State *L) 
{
        CXIndex *idx = (CXIndex*) lua_newuserdata(L, sizeof(CXIndex));
        luaL_getmetatable(L, INDEX);
        lua_setmetatable(L, -2);
        return idx;
}

static CXTranslationUnit* new_CXTU(lua_State *L)
{
        CXTranslationUnit *tu = (CXTranslationUnit*) lua_newuserdata(L, sizeof(CXTranslationUnit));
        luaL_getmetatable(L, TU);
        lua_setmetatable(L, -2);
        return tu;
}

static CXCursor* new_CXCursor(lua_State *L) 
{
        CXCursor *cur = (CXCursor*) lua_newuserdata(L, sizeof(CXCursor));
        luaL_getmetatable(L, CURSOR);
        lua_setmetatable(L, -2);
        return cur;
}

static CXType* new_CXType(lua_State *L)
{
        CXType *t = (CXType*) lua_newuserdata(L, sizeof(CXType));
        luaL_getmetatable(L, TYPE);
        lua_setmetatable(L, -2);
        return t;
}

static CXIndex to_CXIndex(lua_State *L, int n)
{
        CXIndex * idx = (CXIndex*) luaL_checkudata(L, n, INDEX);
        return *idx;
}

static CXTranslationUnit to_CXTU(lua_State *L, int n) 
{
        CXTranslationUnit* tu = (CXTranslationUnit*) luaL_checkudata(L, n, TU);
        return *tu;
}

static int get_TU_cursor(lua_State *L) 
{
        CXTranslationUnit tu = to_CXTU(L, 1);
        CXCursor* cur = new_CXCursor(L);
        *cur = clang_getTranslationUnitCursor(tu);
        if (clang_Cursor_isNull(*cur)) {
                lua_pushnil(L);
        }
        return 1;
}

static int dispose_index(lua_State *L) 
{
        CXIndex idx = to_CXIndex(L, 1);
        clang_disposeIndex(idx);
        return 0;
}

static int dispose_TU(lua_State *L) 
{
        CXTranslationUnit tu = to_CXTU(L, 1);
        clang_disposeTranslationUnit(tu);
        return 0;
}

static int create_index(lua_State *L) 
{
        int exclude_pch = lua_toboolean(L, 1);
        int diagnostics = lua_toboolean(L, 2);
        CXIndex *idx = new_CXIndex(L);
        *idx = clang_createIndex(exclude_pch, diagnostics);
        return 1;
}

static int parse_TU(lua_State *L)
{
        CXIndex idx = to_CXIndex(L, 1);
        const char* file_name = lua_tostring(L, 2);
        const char* args[] = {file_name};
        CXTranslationUnit *tu = new_CXTU(L);
        *tu = clang_parseTranslationUnit(idx, 0, args, 1, 0, 0, CXTranslationUnit_None);
        return 1;
}

static CXCursor to_CXCursor(lua_State *L, int n) 
{
        CXCursor* c = (CXCursor*) luaL_checkudata(L, n, CURSOR);
        return *c;
}

static int get_cursor_spelling(lua_State *L) 
{
        CXCursor cur = to_CXCursor(L, 1);
        CXString name = clang_getCursorSpelling(cur);
        lua_pushstring(L, clang_getCString(name));
        clang_disposeString(name);
        return 1;
}

static int get_cursor_type(lua_State *L)
{
        CXCursor cur = to_CXCursor(L, 1);
        CXType *type = new_CXType(L);
        *type = clang_getCursorType(cur);
        CXString type_spelling = clang_getTypeSpelling(*type);
        lua_pushstring(L, clang_getCString(type_spelling));
        clang_disposeString(type_spelling);
        return 2;
}

static const struct luaL_Reg luaclang [] = {
        {"createIndex", create_index},
        {"parseTU", parse_TU}, 
        {"getTUCursor", get_TU_cursor},
        {"disposeTU", dispose_TU},
        {"disposeIndex", dispose_index},
        {"getCursorType", get_cursor_type},
        {"getCursorSpelling", get_cursor_spelling},
        {NULL, NULL}  
};

int luaopen_luaclang (lua_State *L)
{
        luaL_newlib(L, luaclang);
        return 1;
}