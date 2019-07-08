#include <clang-c/Index.h>
#include <stdbool.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define INDEX_METATABLE  "Clang.Index"
#define TU_METATABLE     "Clang.TU"
#define CURSOR_METATABLE "Clang.Cursor"
#define TYPE_METATABLE   "Clang.Type"

static CXIndex * new_CXIndex(lua_State *L) 
{
        CXIndex *idx = (CXIndex*) lua_newuserdata(L, sizeof(CXIndex));
        luaL_getmetatable(L, INDEX_METATABLE);
        lua_setmetatable(L, -2);
        return idx;
}

static CXIndex to_CXIndex(lua_State *L, int n)
{
        CXIndex *idx = (CXIndex*) luaL_checkudata(L, n, INDEX_METATABLE);
        return *idx;
}

static CXTranslationUnit * new_CXTU(lua_State *L)
{
        CXTranslationUnit *tu = (CXTranslationUnit*) lua_newuserdata(L, sizeof(CXTranslationUnit));
        luaL_getmetatable(L, TU_METATABLE);
        lua_setmetatable(L, -2);
        return tu;
}

static CXTranslationUnit to_CXTU(lua_State *L, int n) 
{
        CXTranslationUnit *tu = (CXTranslationUnit*) luaL_checkudata(L, n, TU_METATABLE);
        return *tu;
}

static CXCursor * new_CXCursor(lua_State *L) 
{
        CXCursor *cur = (CXCursor*) lua_newuserdata(L, sizeof(CXCursor));
        luaL_getmetatable(L, CURSOR_METATABLE);
        lua_setmetatable(L, -2);
        return cur;
}

static CXCursor to_CXCursor(lua_State *L, int n) 
{
        CXCursor *c = (CXCursor*) luaL_checkudata(L, n, CURSOR_METATABLE);
        return *c;
}

static CXType * new_CXType(lua_State *L)
{
        CXType *t = (CXType*) lua_newuserdata(L, sizeof(CXType));
        luaL_getmetatable(L, TYPE_METATABLE);
        lua_setmetatable(L, -2);
        return t;
}


/* Clang function */

static int create_CXIndex(lua_State *L) 
{
        int exclude_pch = lua_toboolean(L, 1);
        int diagnostics = lua_toboolean(L, 2);
        CXIndex *idx = new_CXIndex(L);
        *idx = clang_createIndex(exclude_pch, diagnostics);
        return 1;
}


/* Index functions */

static int dispose_CXIndex(lua_State *L) 
{
        CXIndex idx = to_CXIndex(L, 1);
        clang_disposeIndex(idx);
        return 0;
}

static int parse_TU(lua_State *L)
{
        CXIndex idx = to_CXIndex(L, 1);
        const char *file_name = lua_tostring(L, 2);
        const char *args[] = {file_name};
        CXTranslationUnit *tu = new_CXTU(L);
        *tu = clang_parseTranslationUnit(idx, 0, args, 1, 0, 0, CXTranslationUnit_None);
        return 1;
}

static luaL_Reg index_functions[] = {
        {"disposeIndex", dispose_CXIndex},
        {"parseTU", parse_TU},
        {NULL, NULL}
};


/* Translation unit functions */

static int dispose_CXTU(lua_State *L) 
{
        CXTranslationUnit tu = to_CXTU(L, 1);
        clang_disposeTranslationUnit(tu);
        return 0;
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

static luaL_Reg tu_functions[] = {
        {"disposeTU", dispose_CXTU},
        {"getTUCursor", get_TU_cursor},
        {NULL, NULL}
};

/* Cursor functions */

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
        lua_settop(L, 0);
        lua_pushstring(L, clang_getCString(type_spelling));
        clang_disposeString(type_spelling);
        return 1;
}

static const char * cursor_kind_str(enum CXCursorKind kind) 
{
    switch (kind) {
        case CXCursor_StructDecl: 
                return "StructDecl";
        case CXCursor_UnionDecl: 
                return "UnionDecl";
        case CXCursor_EnumDecl: 
                return "EnumDecl";
        case CXCursor_FieldDecl: 
                return "FieldDecl";
        case CXCursor_EnumConstantDecl: 
                return "EnumConstantDecl";
        case CXCursor_FunctionDecl: 
                return "FunctionDecl";
        case CXCursor_VarDecl: 
                return "VarDecl";
        case CXCursor_ParmDecl: 
                return "ParmDecl";
        case CXCursor_TypedefDecl: 
                return "TypedefDecl";
        case CXCursor_IntegerLiteral: 
                return "IntegerLiteral";
        default: 
                return "Unknown";
    }
}

static int get_cursor_kind(lua_State *L)
{
        CXCursor cur = to_CXCursor(L, 1);
        lua_pushstring(L, cursor_kind_str(clang_getCursorKind(cur)));
        return 1;
}

static int get_num_args(lua_State *L)
{
        CXCursor cur = to_CXCursor(L, 1);
        int num_args = clang_Cursor_getNumArguments(cur);
        lua_pushnumber(L, num_args);
        return 1;
}

static int is_function_inline(lua_State *L)
{
        CXCursor cur = to_CXCursor(L, 1);
        CINDEX_LINKAGE bool is_inline;
        is_inline = clang_Cursor_isFunctionInlined(cur);
        lua_pushboolean(L, is_inline);
        return 1;
}

static luaL_Reg cursor_functions[] = {
        {"getCursorSpelling", get_cursor_spelling},
        {"getCursorType", get_cursor_type},
        {"getNumArgs", get_num_args},
        {"isFunctionInline", is_function_inline},
        {NULL, NULL}
};

void new_metatable(lua_State *L, const char *name, luaL_Reg *reg) 
{
        luaL_newmetatable(L, name);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
}

static const struct luaL_Reg luaclang [] = {
        {"createIndex", create_CXIndex},
        {"parseTU", parse_TU}, 
        {"getTUCursor", get_TU_cursor},
        {"disposeTU", dispose_CXTU},
        {"disposeIndex", dispose_CXIndex},
        {"getCursorSpelling", get_cursor_spelling},
        {"getCursorType", get_cursor_type},
        {"getCursorKind", get_cursor_kind},
        {"getNumArgs", get_num_args},
        {"isFunctionInline", is_function_inline},
        {NULL, NULL}  
};

int luaopen_luaclang(lua_State *L) 
{
        new_metatable(L, INDEX_METATABLE, index_functions);
        new_metatable(L, TU_METATABLE, tu_functions);
        new_metatable(L, CURSOR_METATABLE, cursor_functions);

        luaL_newlib(L, luaclang);
        return 1;
}
