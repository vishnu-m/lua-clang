#include <clang-c/Index.h>
#include <stdbool.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define CLANG_METATABLE  "Clang.Object"
#define CURSOR_METATABLE "Clang.Cursor"
#define TYPE_METATABLE   "Clang.Type"

typedef struct clang_object {
        CXIndex idx;
        CXTranslationUnit tu;
} clang_object;

/* Create clang_object as userdata */
static clang_object *new_clangobject(lua_State *L) 
{
        clang_object *clang_parser = (clang_object*) lua_newuserdata(L, sizeof(clang_object));
        luaL_getmetatable(L, CLANG_METATABLE);
        lua_setmetatable(L, -2);
        return clang_parser;
}

/* Convert userdata type to  */
static clang_object to_clangobject(lua_State *L, int n) 
{
        clang_object *clang_parser = (clang_object*) luaL_checkudata(L, n, CLANG_METATABLE);
        return *clang_parser;
}

/*      
        Format - newParser(file_name)
        Parameter - file_name - The name of the source file to load 
        More info - 1. https://clang.llvm.org/doxygen/group__CINDEX.html#ga51eb9b38c18743bf2d824c6230e61f93
                    2. https://clang.llvm.org/doxygen/group__CINDEX__TRANSLATION__UNIT.html#ga2baf83f8c3299788234c8bce55e4472e
        Returns clang object whose translation unit cursor can be obtained.
*/
static int create_clangobject(lua_State *L)
{
        const char *file_name = lua_tostring(L, 1);
        clang_object *clang_parser = new_clangobject(L);
        clang_parser->idx = clang_createIndex(1, 0);
        const char *args[] = {file_name};
        clang_parser->tu = clang_parseTranslationUnit(clang_parser->idx, 0, args, 1, 0, 0, CXTranslationUnit_None);
        return 1;
}

/*      
        Format - parserdisposeParser()
        Parameter - parser - Clang object to be disposed
        More info - 1. https://clang.llvm.org/doxygen/group__CINDEX.html#ga51eb9b38c18743bf2d824c6230e61f93
                    2. https://clang.llvm.org/doxygen/group__CINDEX__TRANSLATION__UNIT.html#ga2baf83f8c3299788234c8bce55e4472e
        Returns nothing
*/
static int dispose_clangobject(lua_State *L)
{
        clang_object clang_parser = to_clangobject(L, 1);
        if (clang_parser.idx == NULL) return 0;
        clang_disposeIndex(clang_parser.idx);
        clang_disposeTranslationUnit(clang_parser.tu);
        clang_parser.idx = NULL;
        clang_parser.tu = NULL;
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
static  CXCursor to_CXCursor(lua_State *L, int n) 
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
        clang_object clang_parser = to_clangobject(L, 1);
        CXCursor* cur = new_CXCursor(L);
        *cur = clang_getTranslationUnitCursor(clang_parser.tu);
        if (clang_Cursor_isNull(*cur)) {
                lua_pushnil(L);
        }
        return 1;
}

static luaL_Reg clang_functions[] = {
    {"newParser", create_clangobject},
    {"disposeParser", dispose_clangobject},
    {"getCursor", get_cursor},
    {"__gc", dispose_clangobject},
    {NULL, NULL}
};


/* Create CXType as userdata */

static CXType *new_CXType(lua_State *L)
{
        CXType *t = (CXType*) lua_newuserdata(L, sizeof(CXType));
        luaL_getmetatable(L, TYPE_METATABLE);
        lua_setmetatable(L, -2);
        return t;
}


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

/*      
        Format - cur:getType()
        Parameter - cur - Cursor whose type is to be obtained    
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#gaae5702661bb1f2f93038051737de20f4
*/
static int get_cursor_type(lua_State *L)
{
        CXCursor cur = to_CXCursor(L, 1);
        CXType *type = new_CXType(L);
        *type = clang_getCursorType(cur);
        CXString type_spelling = clang_getTypeSpelling(*type);
        lua_pushstring(L, clang_getCString(type_spelling));
        clang_disposeString(type_spelling);
        return 1;
}

/* Return the cursor kind as a string */
static const char *cursor_kind_str(enum CXCursorKind kind) 
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

/*      
        Format - cur:getKind()
        Parameter - cur - Cursor whose kind is to be obtained    
        More info - https://clang.llvm.org/doxygen/group__CINDEX__CURSOR__MANIP.html#ga018aaf60362cb751e517d9f8620d490c
*/
static int get_cursor_kind(lua_State *L)
{
        CXCursor cur = to_CXCursor(L, 1);
        lua_pushstring(L, cursor_kind_str(clang_getCursorKind(cur)));
        return 1;
}

/* Returns the storage class specifier as a string */
static const char *storage_class_str(enum CX_StorageClass sc_specifier) 
{
        switch (sc_specifier) {
                case CX_SC_Extern:
                        return "extern";
                case CX_SC_Static:
                        return "static";
                case CX_SC_Auto:
                        return "auto";
                case CX_SC_Register:
                        return "register";
                default:
                        return "unknown";
        }
}

/*
        Format - cur:getStorageClass()
        Parameter - cur -Cursor whose storage class specifier is to be obtained
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga230c7904f3878469d772f3e464b9c83d
 */
static int get_storage_class(lua_State *L)
{
        CXCursor cur = to_CXCursor(L, 1);
        enum CX_StorageClass sc_specifier = clang_Cursor_getStorageClass(cur);
        lua_pushstring(L, storage_class_str(sc_specifier));
        return 1;
}

/*
        Format - cur:getStorageClass()
        Parameter - cur - Cursor whose Result type is to be obtained(For function declaration)
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga39b4850746f39e17c6b8b4eef3154d85
 */
static int get_result_type(lua_State *L)
{
        CXCursor cur = to_CXCursor(L, 1);
        CXType *type = new_CXType(L);
        *type = clang_getCursorType(cur);
        CXString result_type = clang_getTypeSpelling(clang_getResultType(*type));
        lua_pushstring(L, clang_getCString(result_type));
        return 1;
}

/*      
        Format - cur:getNumArgs()
        Parameter - cur - Cursor corresponding to which the number non-variadic arguments is to be obtained
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga5254f761b57fd78de3ac9c6bfcaa7fed
*/
static int get_num_args(lua_State *L)
{
        CXCursor cur = to_CXCursor(L, 1);
        int num_args = clang_Cursor_getNumArguments(cur);
        lua_pushnumber(L, num_args);
        return 1;
}

/*      
        Format - cur:isFunctionInline()
        Parameter - cur - Cursor which is to be checked (whether the function declaration `cur` represents is inline)
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga5254f761b57fd78de3ac9c6bfcaa7fed
*/
static int is_function_inline(lua_State *L)
{
        CXCursor cur = to_CXCursor(L, 1);
        CINDEX_LINKAGE bool is_inline;
        is_inline = clang_Cursor_isFunctionInlined(cur);
        lua_pushboolean(L, is_inline);
        return 1;
}

/*      
        Format - cur:getArgument()
        Parameter - cur - Cursor of a fucntion/method whose associated argument cursor is to be obtained
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga673c5529d33eedd0b78aca5ac6fc1d7c
*/
static int get_argument(lua_State *L)
{
        CXCursor cur = to_CXCursor(L, 1);
        int arg_index = lua_tointeger(L, 2);
        CXCursor arg_cursor = clang_Cursor_getArgument(cur, arg_index);
        CXString arg_cursor_name = clang_getCursorSpelling(arg_cursor);
        lua_pushstring(L, clang_getCString(arg_cursor_name));
        clang_disposeString(arg_cursor_name);
        return 1;
}

static luaL_Reg cursor_functions[] = {
        {"getSpelling", get_cursor_spelling},
        {"getType", get_cursor_type},
        {"getKind", get_cursor_kind},
        {"getStorageClass", get_storage_class},
        {"getResultType", get_result_type},
        {"getNumArgs", get_num_args},
        {"isFunctionInline", is_function_inline},
        {"getArgument", get_argument},
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
