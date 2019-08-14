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
        Format - parser:dispose()
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

/*
        Format - cur:getType()
        Parameter - cur - Cursor whose type is to be obtained    
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#gaae5702661bb1f2f93038051737de20f4
        Returns the type of the cursor
*/
static int cursor_gettype(lua_State *L)
{
        CXCursor *cur;
        to_object(L, cur, CURSOR_METATABLE, 1);
        CXType *cur_type;
        new_object(L, cur_type, TYPE_METATABLE);
        *cur_type = clang_getCursorType(*cur);
        return 1;
}

/*      
        Format - cur:getNumArgs()
        Parameter - cur - Cursor which represents a function
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga5254f761b57fd78de3ac9c6bfcaa7fed
        Returns the number of non-variadic arguments in the function is to be obtained
*/
static int cursor_getnumargs(lua_State *L)
{
        CXCursor *cur;
        to_object(L, cur, CURSOR_METATABLE, 1);
        luaL_argcheck(L, clang_getCursorKind(*cur) == CXCursor_FunctionDecl, 1, "expect cursor with function kind");
        int num_args = clang_Cursor_getNumArguments(*cur);
        lua_pushnumber(L, num_args);
        return 1;
}

/*
        Format - cur:getArgCursor(idx)
        Parameters - cur - Cursor representing the function declaration 
                   - idx - parameter index (starting from 1)
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga673c5529d33eedd0b78aca5ac6fc1d7c
        Returns the cursor of the parameter at index 'idx' in the function 
*/
static int cursor_getarg(lua_State *L)
{
        CXCursor *cur;
        to_object(L, cur, CURSOR_METATABLE, 1);
        luaL_argcheck(L, clang_getCursorKind(*cur) == CXCursor_FunctionDecl, 1, "expect cursor with function kind");
        unsigned int index = luaL_checkinteger(L, 2);
        luaL_argcheck(L, index <= clang_Cursor_getNumArguments(*cur), 1, "argument index out of bounds");
        CXCursor *arg_cur;
        new_object(L, arg_cur, CURSOR_METATABLE);
        *arg_cur = clang_Cursor_getArgument(*cur, index-1);
        return 1;
}

/*      
        Format - cur:isFunctionInline()
        Parameter - cur - Cursor which is to be checked (whether the FunctionDecl 'cur' represents is inline)
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga5254f761b57fd78de3ac9c6bfcaa7fed
        Returns 'true' if the function is inlined , 'false' otherwise
*/
static int cursor_isinlined(lua_State *L)
{
        CXCursor *cur;
        to_object(L, cur, CURSOR_METATABLE, 1);
        luaL_argcheck(L, clang_getCursorKind(*cur) == CXCursor_FunctionDecl, 1, "expect cursor with function kind");
        CINDEX_LINKAGE bool is_inline;
        is_inline = clang_Cursor_isFunctionInlined(*cur);
        lua_pushboolean(L, is_inline);
        return 1;
}

/*      
        Format - cur:getEnumValue()
        Parameter - cur - Cursor which represents an EnumConstantDecl whose integer value is to be obtained
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga6b8585818420e7512feb4c9d209b4f4d
        Returns the integer value corresponding to the EnumConstantDecl
*/
static int cursor_getenumvalue(lua_State *L)
{
        CXCursor *cur;
        to_object(L, cur, CURSOR_METATABLE, 1);
        luaL_argcheck(L, clang_getCursorKind(*cur) == CXCursor_EnumConstantDecl, 1, "expect cursor with enum constant kind");
        int enum_value = clang_getEnumConstantDeclValue(*cur);
        lua_pushinteger(L, enum_value);
        return 1;
}

/* Retrieve the storage class specifier string */
static const char *storage_class_str(enum CX_StorageClass sc_specifier) 
{
        switch (sc_specifier) {
                case CX_SC_Invalid:
                        return "invalid";
                case CX_SC_None:
                        return "none";
                case CX_SC_Extern:
                        return "extern";
                case CX_SC_Static:
                        return "static";
                case CX_SC_PrivateExtern:
                        return "private extern";
                case CX_SC_OpenCLWorkGroupLocal:
                        return "opencl workgroup local";
                case CX_SC_Auto:
                        return "auto";
                case CX_SC_Register:
                        return "register";
                default:
                        return NULL;
        }
}

/*
        Format - cur:getStorageClass()
        Parameter - cur -Cursor whose storage class specifier is to be obtained
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga230c7904f3878469d772f3e464b9c83d
        Returns the storage class specifier string
*/
static int cursor_getstorageclass(lua_State *L)
{
        CXCursor *cur;
        to_object(L, cur, CURSOR_METATABLE, 1);
        enum CX_StorageClass sc_specifier = clang_Cursor_getStorageClass(*cur);
        const char *sc_specifier_str = storage_class_str(sc_specifier);
        if(sc_specifier_str == NULL)
                luaL_error(L, "lua-clang: %s: unknown case %d", __func__, sc_specifier);
        else
                lua_pushstring(L, storage_class_str(sc_specifier));
        return 1;
}

/*
        Format - cur:isBitField()
        Parameter - cur -Cursor to be checked
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga750705f6b418b25ca00495b7392c740d
        Returns 'true if the cursor represents a bitfield; 'false' otherwise
*/
static int cursor_isbitfield(lua_State *L)
{
        CXCursor *cur;
        to_object(L, cur, CURSOR_METATABLE, 1);
        luaL_argcheck(L, clang_getCursorKind(*cur) == CXCursor_FieldDecl, 1, "expect cursor with struct/union field kind");
        bool is_bitfield = clang_Cursor_isBitField(*cur);
        lua_pushboolean(L, is_bitfield);
        return 1;
}

/*
        Format - cur:getBitFieldWidth()
        Parameter - cur - Cursor which represents the bitfield
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga80bbb872dde5b2f26964081338108f91
        Returns the bit width of a bit field declaration as an integer
*/
static int cursor_getbitfield_width(lua_State *L)
{
        CXCursor *cur;
        to_object(L, cur, CURSOR_METATABLE, 1);
        luaL_argcheck(L, clang_Cursor_isBitField(*cur), 1, "expect cursor with struct/union field kind that is a bit field");
        int bitfield_width = clang_getFieldDeclBitWidth(*cur);
        lua_pushinteger(L, bitfield_width);
        return 1;
}

/*
        Format - cur:getTypedefUnderlyingType()
        Parameter - cur - Cursor which represents the TypedefDecl
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga8de899fc18dc859b6fe3b97309f4fd52
        Returns the Underlying type of a TypedefDecl
*/
static int cursor_gettypdef_underlying(lua_State *L)
{
        CXCursor *cur;
        to_object(L, cur, CURSOR_METATABLE, 1);
        luaL_argcheck(L, clang_getCursorKind(*cur) == CXCursor_TypedefDecl, 1, "expect cursor with typedef kind");
        CXType *underlying_type;
        new_object(L, underlying_type, TYPE_METATABLE);
        *underlying_type = clang_getTypedefDeclUnderlyingType(*cur);
        return 1;
}

/*
        Format - cur1:equals(cur2)
        Parameters - cur1 - First cursor
                     cur2 - Second cursor
        More info - https://clang.llvm.org/doxygen/group__CINDEX__CURSOR__MANIP.html#ga98df58f09878710b983b6f3f60f0cba3
        Returns a boolean value after comparing the two cursors for equality
*/
static int cursor_equals(lua_State *L)
{
       CXCursor *cur1;
       to_object(L, cur1, CURSOR_METATABLE, 1); 
       CXCursor *cur2;
       to_object(L, cur2, CURSOR_METATABLE, 2);
       bool is_equal = clang_equalCursors(*cur1, *cur2);
       lua_pushboolean(L, is_equal);
       return 1;
}
enum CXChildVisitResult visitor_function(CXCursor cursor, CXCursor parent, CXClientData client_data)
{
        lua_State *L = (lua_State*) client_data;
        int nargs = lua_gettop(L);
        lua_pushvalue(L, 1);    
        CXCursor *cur;
        new_object(L, cur, CURSOR_METATABLE);           
        *cur = cursor;
        CXCursor *par;
        new_object(L, par, CURSOR_METATABLE);
        *par = parent;
        for (int i = 2; i <= nargs; i++) {
                lua_pushvalue(L, i);
        }
        if (lua_pcall(L, nargs+1, 1, 0) != 0) {
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
        if (!lua_checkstack(L, lua_gettop(L)+2))
                luaL_error(L, "excessive number of params");       
        clang_visitChildren(*cur, visitor_function, L);
        if (lua_isstring(L, lua_gettop(L))) {
                luaL_error(L, lua_tostring(L, lua_gettop(L)));
                return 1;
        }
        return 0;
}


/* -- Type functions -- */

/*
        Format - cur_type:getTypeSpelling()
        Parameter - cur_type - Cursor type whose spelling is to be printed  
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#gac9d37f61bede521d4f42a6553bcbc09f
        Returns the spelling of the cursor type
*/
static int type_getspelling(lua_State *L)
{
        CXType *type;
        to_object(L, type, TYPE_METATABLE, 1);
        CXString type_str = clang_getTypeSpelling(*type);
        lua_pushstring(L, clang_getCString(type_str));
        clang_disposeString(type_str);
        return 1; 
}

/*
        Format - cur_type:getResultType()
        Parameter - cur_type - Cursor type which is that of a function, whose return type is to be printed 
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#gac9d37f61bede521d4f42a6553bcbc09f
        Returns the return type associated with a function type
*/
static int type_getresult(lua_State *L)
{
        CXType *type;
        to_object(L, type, TYPE_METATABLE, 1);
        luaL_argcheck(L, type->kind == CXType_FunctionProto, 1, "expect type object with function kind");
        CXType *result_type;
        new_object(L, result_type, TYPE_METATABLE);
        *result_type = clang_getResultType(*type);
        return 1;
}

/*
        Format - cur_type:getArgType(idx)
        Parameters - cur_type - Cursor type of the function 
                   - idx - parameter index (starting from 1)
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga67f60ba4831b1bfd90ab0c1c12adab27
        Returns the type of the parameter at index 'idx' in the function 
*/
static int type_getarg(lua_State *L)
{
        CXType *type;
        to_object(L, type, TYPE_METATABLE, 1);
        luaL_argcheck(L, type->kind == CXType_FunctionProto, 1, "expect type object with function kind");
        unsigned int index = luaL_checkinteger(L, 2);
        CXType *arg_type;
        new_object(L, arg_type, TYPE_METATABLE);
        *arg_type = clang_getArgType(*type, index-1);
        return 1;
}

/*
        Format - cur_type:getArrayElementType()
        Parameter - cur_type - Cursor type of the function whose kind is an array
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga718591f4b07d9d4861557a3ed8b29713
        Returns the type of the array
*/
static int type_getarrtype(lua_State *L)
{
        CXType *type;
        to_object(L, type, TYPE_METATABLE, 1);
        luaL_argcheck(L, type->kind == CXType_ConstantArray || type->kind == CXType_VariableArray || type->kind == CXType_IncompleteArray || type->kind == CXType_DependentSizedArray, 1, "expect type object with array kind");
        CXType *arr_type;
        new_object(L, arr_type, TYPE_METATABLE);
        *arr_type = clang_getArrayElementType(*type);
        return 1;
}

/*
        Format - cur_type:getArraySize()
        Parameter - cur_type - Cursor type of the function  whose kind is a constant array
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga91521260817054f153b5f1295056192d
        Returns the size of the constant array
*/
static int type_getarrsize(lua_State *L)
{
        CXType *type;
        to_object(L, type, TYPE_METATABLE, 1);
        luaL_argcheck(L, type->kind == CXType_ConstantArray, 1, "expect type object with array kind");
        long long size = clang_getArraySize(*type);
        lua_pushnumber(L, size);
        return 1;
}

/*
        Format - cur_type:getPointeeType()
        Parameter - cur_type - Cursor type of the function whose kind is a pointer
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#gaafa3eb34932d8da1358d50ed949ff3ee
        Returns the type of the pointee
*/
static int type_getpointee_type(lua_State *L)
{
        CXType *type;
        to_object(L, type, TYPE_METATABLE, 1);
        luaL_argcheck(L, type->kind == CXType_Pointer, 1, "expect type object with pointer kind");
        CXType *pointee_type;
        new_object(L, pointee_type, TYPE_METATABLE);
        *pointee_type = clang_getPointeeType(*type);
        return 1;
}


/*
        Format - cur_type:getTypeKind()
        Parameter - cur_type - Cursor type whose type kind is to be found
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#gaad39de597b13a18882c21860f92b095a
        Returns the type kind of the type object
*/
static int type_gettypekind(lua_State *L)
{
        CXType *type;
        to_object(L, type, TYPE_METATABLE, 1);
        lua_pushstring(L, clang_getCString(clang_getTypeKindSpelling(type->kind)));
        return 1;
}

/*
        Format - cur_type:getNumArgTypes()
        Parameter - cur_type - Cursor type which has FunctionProto kind 
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga705e1a4ed7c7595606fc30ed5d2a6b5a
        Returns the number of arguments
 */
static int type_getnumargtypes(lua_State *L)
{
        CXType *type;
        to_object(L, type, TYPE_METATABLE, 1);
        luaL_argcheck(L, type->kind == CXType_FunctionProto, 1, "expect type object with function kind");
        int num_args = clang_getNumArgTypes(*type);
        lua_pushnumber(L, num_args);
        return 1;
}

/*
        Format - cur_type:getTypeDeclaration()
        Parameter - cur_type - Type object whose cursor is to be obtained
        More info - https://clang.llvm.org/doxygen/group__CINDEX__TYPES.html#ga0aad74ea93a2f5dea58fd6fc0db8aad4
        Returns the corresponding cursor for the declaration of given type
 */
static int type_gettypedecl(lua_State *L)
{
        CXType *type;
        to_object(L, type, TYPE_METATABLE, 1);
        CXCursor *cur;
        new_object(L, cur, CURSOR_METATABLE);
        *cur = clang_getTypeDeclaration(*type);
        return 1;
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
        {"getType", cursor_gettype}, 
        {"getNumArgs", cursor_getnumargs}, 
        {"getArgCursor", cursor_getarg},
        {"isFunctionInlined", cursor_isinlined},
        {"getEnumValue", cursor_getenumvalue}, 
        {"getStorageClass", cursor_getstorageclass}, 
        {"isBitField", cursor_isbitfield},
        {"getBitFieldWidth", cursor_getbitfield_width},
        {"getTypedefUnderlyingType", cursor_gettypdef_underlying},
        {"equals", cursor_equals},
        {NULL, NULL}
};

static luaL_Reg type_functions[] = {
        {"getSpelling", type_getspelling}, 
        {"getResultType", type_getresult}, 
        {"getArgType", type_getarg},
        {"getArrayElementType", type_getarrtype}, 
        {"getArraySize", type_getarrsize},
        {"getPointeeType", type_getpointee_type},
        {"getTypeKind", type_gettypekind},
        {"getNumArgTypes", type_getnumargtypes},   
        {"getTypeDeclaration", type_gettypedecl}, 
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
        new_metatable(L, PARSER_METATABLE, parser_functions);
        new_metatable(L, CURSOR_METATABLE, cursor_functions);
        new_metatable(L, TYPE_METATABLE, type_functions);

        lua_newtable(L);
        luaL_setfuncs(L, clang_functions, 0);
        return 1;
}