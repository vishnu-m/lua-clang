local luaclang = require "luaclang"

describe("parser object creation", function() 
        it("succeeds in creating object for an available file", function()
                local parser = luaclang.newParser("spec/visit.c")
                assert.are.same('userdata', type(parser))
        end)

        it("fails to create an object for an unavailable file", function()
                assert.has.errors(function()
                        luaclang.newParser("non_existent.c")
                end, "file doesn't exist")
        end)
end)

describe("cursor creation", function() 
        it("succeeds in creating translation unit cursor", function() 
                local parser = luaclang.newParser("spec/visit.c")
                local cursor = parser:getCursor()
                assert.are.same('userdata', type(cursor))
        end)       
end)

describe("cursor spelling", function() 
        it("succeeds in matching the expected cursor spelling", function()
                local parser = luaclang.newParser("spec/visit.c")
                local cursor = parser:getCursor()
                local cursor_spelling = cursor:getSpelling()
                assert.are.equals(cursor_spelling, "spec/visit.c")
        end)
end)

describe("parser object disposal", function()
        it("fails when a disposed parser object is used for cursor creation", function()
                local parser = luaclang.newParser("spec/visit.c")
                parser:dispose()
                assert.has.errors(function()
                        local cursor = parser:getCursor()
                end, "calling 'getCursor' on bad self (parser object was disposed)")
        end)
        
        it("succeeds in getting the spelling of cursor whose parser object was disposed", function()
                local parser = luaclang.newParser("spec/visit.c")
                local cursor = parser:getCursor()
                parser:dispose()
                assert.has_no.errors(function()
                        local cursor_spelling = cursor:getSpelling()
                end)
        end)
end)

describe("visit children", function()
        it("returns continue", function()
                local parser = luaclang.newParser("spec/visit.c")
                local cur = parser:getCursor()
                local expected = {
                                        {"outer", "spec/visit.c"},
                                        {"type", "spec/visit.c"}
                                 }
                local children = {}
                cur:visitChildren(function (cursor, parent)
                        local cur_spelling, par_spelling = cursor:getSpelling(), parent:getSpelling()
                        table.insert(children, {cur_spelling, par_spelling})
                        return "continue"
                end)
                assert.are.same(expected, children)
        end)

        it("returns break", function()
                local parser = luaclang.newParser("spec/visit.c")
                local cur = parser:getCursor()
                local expected = {
                                        {"outer", "spec/visit.c"}
                                 }
                local children = {}
                cur:visitChildren(function (cursor, parent)
                        local cur_spelling, par_spelling = cursor:getSpelling(), parent:getSpelling()
                        table.insert(children, {cur_spelling, par_spelling})
                        return "break"
                end)
                assert.are.same(expected, children)
        end)

        it("returns recurse", function()
                local parser = luaclang.newParser("spec/visit.c")
                local cur = parser:getCursor()
                local expected = {
                                        {"outer", "spec/visit.c"},
                                        {"first", "outer"},
                                        {"inner", "outer"},
                                        {"second", "inner"},
                                        {"inner_var", "outer"},
                                        {"inner", "inner_var"},
                                        {"second", "inner"},
                                        {"type", "spec/visit.c"},
                                        {"Integer", "type"},
                                        {"Float", "type"},
                                        {"String", "type"}
                                 }
                local children = {}
                cur:visitChildren(function (cursor, parent)
                        local cur_spelling, par_spelling = cursor:getSpelling(), parent:getSpelling()
                        table.insert(children, {cur_spelling, par_spelling})
                        return "recurse"
                end)
                assert.are.same(expected, children)
        end)

        it("throws an error with undefined return to visitor", function()
                local parser = luaclang.newParser("spec/visit.c")
                local cur = parser:getCursor()
                assert.has.errors(function()
                        cur:visitChildren(function (cursor, parent) 
                                return "unknown"
                        end)
                end, "undefined return to visitor")
        end)

        it("throws an error inside the callback function", function()
                local parser = luaclang.newParser("spec/visit.c")
                local cur = parser:getCursor()
                assert.has.errors(function()
                        cur:visitChildren(function (cursor, parent)
                          error("myerror")
                        end)
                end, "spec/clang_spec.lua:124: myerror")
        end)
 
        it("throws an error if argument supplied is not a function", function()
                local parser = luaclang.newParser("spec/visit.c")
                local cur = parser:getCursor()
                assert.has.errors(function()
                        cur:visitChildren("not a function")
                end, "bad argument #1 to 'visitChildren' (function expected, got string)")
        end)

        it("nested visit children", function()
                local parser = luaclang.newParser("spec/visit.c")
                local cur = parser:getCursor()
                local expected = {
                                        {"outer", "spec/visit.c"},
                                        {"first", "outer"},
                                        {"inner", "outer"},
                                        {"inner_var", "outer"},
                                        {"type", "spec/visit.c"},
                                        {"Integer", "type"},
                                        {"Float", "type"},
                                        {"String", "type"}
                                 }
                local children = {}
                cur:visitChildren(function (cursor, parent)
                        local cur_spelling, par_spelling = cursor:getSpelling(), parent:getSpelling()
                        table.insert(children, {cur_spelling, par_spelling})
                        cursor:visitChildren(function (cursor, parent)
                                local cur_spelling, par_spelling = cursor:getSpelling(), parent:getSpelling()
                                table.insert(children, {cur_spelling, par_spelling})
                                return "continue"
                        end)
                        return "continue"
                end )
                assert.are.same(expected, children)     
        end)
end)

describe("cursor kind", function()
        it("succeeds in getting the kind of cursor", function()
                local parser = luaclang.newParser("spec/visit.c")
                local cur = parser:getCursor()
                local expected = {
                        "StructDecl",
                        "EnumDecl"
                }
                local children = {}
                cur:visitChildren(function (cursor, parent)
                        table.insert(children, cursor:getKind())
                        return "continue"
                end)
                assert.are.same(expected, children)     
        end)
end)

function find_decl(cursor)
        local decl_cursor
        cursor:visitChildren(function(cursor, parent)
                decl_cursor = cursor
                return "continue"
        end)
        return decl_cursor
end

describe("type object creation", function() 
        it("succeeds in creating type object", function() 
                local parser = luaclang.newParser("spec/function.c")
                local cursor = parser:getCursor()   
                local cursor_type = cursor:getType()             
                assert.are.same('userdata', type(cursor_type))
        end)       
end)

describe("type spelling", function() 
        it("succeeds in matching the expected type spelling", function()
                local parser = luaclang.newParser("spec/function.c")
                local cursor = parser:getCursor()
                cursor = find_decl(cursor)
                local cursor_type = cursor:getType()
                local cursor_type_spelling = cursor_type:getTypeSpelling()
                assert.are.equals(cursor_type_spelling, "void (float, float *)")
        end)
end)

describe("result type", function()
        it("succeeds in matching the expected result type", function()
                local parser = luaclang.newParser("spec/function.c")
                local cursor = parser:getCursor()
                cursor = find_decl(cursor)
                local cursor_type = cursor:getType()
                assert.has_no.errors(function()
                        result_type = cursor_type:getResultType()
                end)
                local result_type_str = result_type:getTypeSpelling()
                assert.are.equals(result_type_str, "void")
        end)
end)

describe("arg type", function()
        it("succeeds in matching the expected arg type", function()
                local parser = luaclang.newParser("spec/function.c")
                local cursor = parser:getCursor()
                cursor = find_decl(cursor)
                local cursor_type = cursor:getType()
                assert.has_no.errors(function()
                        arg_type = cursor_type:getArgType(1)
                end)
                local arg_type_str = arg_type:getTypeSpelling()
                assert.are.equals(arg_type_str, "float *")
        end)
end)

describe("get num args", function()
        it("succeeds in getting the number of arguments", function()
                local parser = luaclang.newParser("spec/function.c")
                local cursor = parser:getCursor()
                cursor = find_decl(cursor)
                local num_args = cursor:getNumArgs()
                assert.are.equals(num_args, 2)
        end)
end)

describe("inline function check", function()
        it("succeeds in identifying an inline function", function()
                local parser = luaclang.newParser("spec/function_inline.c")
                local cursor = parser:getCursor()
                cursor = find_decl(cursor)
                assert.are.equals(cursor:isFunctionInlined(), true)
        end)

        it("succeeds in identifying a non-inline function", function()
                local parser = luaclang.newParser("spec/function.c")
                local cursor = parser:getCursor()
                cursor = find_decl(cursor)
                assert.are.equals(cursor:isFunctionInlined(), false)
        end)
end)

describe("enum constant value", function()
        it("suceeds in getting the correct integral value of the EnumConstantDecl", function()
                local parser = luaclang.newParser("spec/enum.c")
                local cursor = parser:getCursor()  
                cursor = find_decl(cursor)
                local last_enumconst
                cursor:visitChildren(function(cursor, parent)
                     last_enumconst = parent
                     return "recurse"
                end)
                enum_value = last_enumconst:getEnumValue()
                assert.are.equal(enum_value, 7)
        end)
end)

describe("get storage class", function()
        it("succeeds in getting the correct storage class speciifer", function()
                local parser = luaclang.newParser("spec/function.c")
                local cursor = parser:getCursor()  
                cursor = find_decl(cursor)
                storage_class = cursor:getStorageClass()
                assert.are.equal(storage_class, "extern")
        end)
end)

describe("is bit field", function()
        it("succeeds in identifying a bit field", function()
                local parser = luaclang.newParser("spec/struct.c")
                local cursor = parser:getCursor()  
                cursor = find_decl(cursor)
                local last_member
                cursor:visitChildren(function(cursor, parent)
                        last_member = parent   
                        return "recurse"                     
                end)
                assert.are.equal(last_member:isBitField(), true)
        end)
end)

describe("get bit field width", function()
        it("succeeds in getting bit field width", function()
                local parser = luaclang.newParser("spec/struct.c")
                local cursor = parser:getCursor()  
                cursor = find_decl(cursor)
                local last_member
                cursor:visitChildren(function(cursor, parent)
                        last_member = parent   
                        return "recurse"                     
                end)
                assert.are.equal(last_member:getBitFieldWidth(), 9)
        end)
end)

describe("get typedef underlying type", function()
        it("succeeds in getting bit field width", function()
                local parser = luaclang.newParser("spec/typedef.c")
                local cursor = parser:getCursor()  
                cursor = find_decl(cursor)
                local underlying_type
                assert.has_no.errors(function()
                        underlying_type = cursor:getTypedefUnderlyingType()
                end)
                underlying_type_str = underlying_type:getTypeSpelling()
                assert.are.equal(underlying_type_str, "GROUP *")
        end)
end)
