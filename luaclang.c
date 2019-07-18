#include <clang-c/Index.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

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
static int clang_newparser(lua_State *L)
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
static int parser_dispose(lua_State *L)
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
        Returns translation unit cursor
*/
static int parser_getcursor(lua_State *L) 
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
	Returns the name(spelling) of the entity represented by the cursor
*/
static int cursor_getspelling(lua_State *L) 
{
        CXCursor *cur;
        to_object(L, cur, CURSOR_METATABLE, 1);
        CXString name = clang_getCursorSpelling(*cur);
        lua_pushstring(L, clang_getCString(name));
        clang_disposeString(name);
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
                        return "Unaddressed";
        }
}

/*      
        Format - cur:getKind()
        Parameter - cur - Cursor whose kind is to be obtained    
        More info - https://clang.llvm.org/doxygen/group__CINDEX__CURSOR__MANIP.html#ga018aaf60362cb751e517d9f8620d490c
*/
static int cursor_getkind(lua_State *L)
{
        CXCursor *cur;
        to_object(L, cur, CURSOR_METATABLE, 1);
        lua_pushstring(L, cursor_kind_str(clang_getCursorKind(*cur)));
        return 1;
}

enum CXChildVisitResult visitor_function(CXCursor cursor, CXCursor parent, CXClientData client_data)
{
        lua_State *L = (lua_State*) client_data;
        lua_pushvalue(L, 1);    
        CXCursor *cur;
        new_object(L, cur, CURSOR_METATABLE);           
        *cur = cursor;
        CXCursor *par;
        new_object(L, par, CURSOR_METATABLE);
        *par = parent;
        if (lua_pcall(L, 2, 1, 0) != 0) {
                return CXChildVisit_Break;
        }
        const char *result = lua_tostring(L, -1);
        if (strcmp(result, "continue") == 0) {
                lua_pop(L, 1);
                return CXChildVisit_Continue;
        }
        else if (strcmp(result, "recurse") == 0) {
                lua_pop(L, 1);
                return CXChildVisit_Recurse;
        }
        else if (strcmp(result, "break") == 0) {
                lua_pop(L, 1);
                return CXChildVisit_Break;
        }
        else {
                lua_pushstring(L, "undefined return to visitor");
                return CXChildVisit_Break;
        }
}

/*      
        Format - cur:visitChildren(visitor_function)
        Parameter - cur - Cursor whose children are to be visited  
        A string should be returned by the visitor_function to indicate how visit_children should proceed : 
                1. "break" - Terminates the cursor traversal 
                2. "continue" - Continues the cursor traversal with the next sibling of the cursor just visited, without visiting its children
                3. "recurse" - Recursively traverse the children of this cursor, using the same visitor and client data
        More info - https://clang.llvm.org/doxygen/group__CINDEX__CURSOR__TRAVERSAL.html#ga5d0a813d937e1a7dcc35f206ad1f7a91
        Returns nothing
*/
static int cursor_visitchildren(lua_State *L)                               
{
        CXCursor *cur;
        to_object(L, cur, CURSOR_METATABLE, 1);
        luaL_checktype(L, 2, LUA_TFUNCTION);
        lua_remove(L, 1);       
        clang_visitChildren(*cur, visitor_function, L);
        if (lua_isstring(L, lua_gettop(L))) {
                luaL_error(L, lua_tostring(L, lua_gettop(L)));
                return 1;
        }
        return 0;
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
        {"newParser", clang_newparser},
        {NULL, NULL}
};

static luaL_Reg parser_functions[] = {
        {"dispose", parser_dispose},
        {"getCursor", parser_getcursor},
        {"__gc", parser_dispose},
        {NULL, NULL}
};

static luaL_Reg cursor_functions[] = {
        {"getSpelling", cursor_getspelling},
        {"getKind", cursor_getkind},
        {"visitChildren", cursor_visitchildren},
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
