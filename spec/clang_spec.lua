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
                assert.are.equals("spec/visit.c", cursor_spelling)
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

function get_last_child(cursor)
        local last_child
        cursor:visitChildren(function(cursor, parent)
                last_child = cursor
                return "continue"
        end)
        return last_child
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
                cursor = get_last_child(cursor)
                local cursor_type = cursor:getType()
                local cursor_type_spelling = cursor_type:getSpelling()
                assert.are.equals("void (float, float *)", cursor_type_spelling)
        end)
end)

describe("result type", function()
        it("succeeds in matching the expected result type", function()
                local parser = luaclang.newParser("spec/function.c")
                local cursor = parser:getCursor()
                cursor = get_last_child(cursor)
                local cursor_type = cursor:getType()
                local result_type = cursor_type:getResultType()
                local result_type_str = result_type:getSpelling()
                assert.are.equals("void", result_type_str)
        end)

        it("uses an incompatible cursor with getResultType()", function()
                local parser = luaclang.newParser("spec/struct.c")
                local cursor = parser:getCursor()
                cursor = get_last_child(cursor)
                local cursor_type = cursor:getType()
                assert.has.errors(function()
                        local result_type = cursor_type:getResultType()
                end, "calling 'getResultType' on bad self (expect cursor with function kind)")
        end)
end)

describe("arg type", function()
        it("succeeds in matching the expected arg type", function()
                local parser = luaclang.newParser("spec/function.c")
                local cursor = parser:getCursor()
                cursor = get_last_child(cursor)
                local cursor_type = cursor:getType()
                assert.has.errors(function()
                        arg_type = cursor_type:getArgType(1.2)
                end, "bad argument #1 to 'getArgType' (number has no integer representation)")
        end)

        it("uses a non-integer as index value", function()
                local parser = luaclang.newParser("spec/function.c")
                local cursor = parser:getCursor()
                cursor = get_last_child(cursor)
                local cursor_type = cursor:getType()  
        end)
        it("uses an incompatible cursor with getArgType()", function()
                local parser = luaclang.newParser("spec/struct.c")
                local cursor = parser:getCursor()
                cursor = get_last_child(cursor)
                local cursor_type = cursor:getType()
                assert.has.errors(function()
                        local arg_type = cursor_type:getArgType(1)
                end, "calling 'getArgType' on bad self (expect cursor with function kind)")
        end)
end)

describe("get num args", function()
        it("succeeds in getting the number of arguments", function()
                local parser = luaclang.newParser("spec/function.c")
                local cursor = parser:getCursor()
                cursor = get_last_child(cursor)
                local num_args = cursor:getNumArgs()
                assert.are.equals(2, num_args)
        end)

        it("uses an incompatible cursor with getNumArgs()", function()
                local parser = luaclang.newParser("spec/struct.c")
                local cursor = parser:getCursor()
                cursor = get_last_child(cursor)
                assert.has.errors(function()
                        local num_args = cursor:getNumArgs()
                end, "calling 'getNumArgs' on bad self (expect cursor with function kind)")
        end)
end)

describe("inline function check", function()
        it("succeeds in identifying an inline function", function()
                local parser = luaclang.newParser("spec/function_inline.c")
                local cursor = parser:getCursor()
                cursor = get_last_child(cursor)
                assert.are.equals(true, cursor:isFunctionInlined())
        end)

        it("succeeds in identifying a non-inline function", function()
                local parser = luaclang.newParser("spec/function.c")
                local cursor = parser:getCursor()
                cursor = get_last_child(cursor)
                assert.are.equals(false, cursor:isFunctionInlined())
        end)

        it("uses an incompatible cursor with isInlineFunction", function()
                local parser = luaclang.newParser("spec/struct.c")
                local cursor = parser:getCursor()
                cursor = get_last_child(cursor)
                assert.has.errors(function()
                        local is_inline = cursor:isFunctionInlined()
                end, "calling 'isFunctionInlined' on bad self (expect cursor with function kind)")
        end)
end)

describe("enum constant value", function()
        it("succeeds in getting the correct integral value of the EnumConstantDecl", function()
                local parser = luaclang.newParser("spec/enum.c")
                local cursor = parser:getCursor()  
                cursor = get_last_child(cursor)
                local last_enumconst
                cursor:visitChildren(function(cursor, parent)
                     last_enumconst = parent
                     return "recurse"
                end)
                local enum_value = last_enumconst:getEnumValue()
                assert.are.equal(7, enum_value)
        end)

        it("uses an incompatible cursor with getEnumValue()", function()
                local parser = luaclang.newParser("spec/struct.c")
                local cursor = parser:getCursor()
                cursor = get_last_child(cursor)
                assert.has.errors(function()
                        local enum_value = cursor:getEnumValue()
                end, "calling 'getEnumValue' on bad self (expect cursor with enum constant kind)")
        end)
end)

describe("get storage class", function()
        it("succeeds in getting the correct storage class speciifer", function()
                local parser = luaclang.newParser("spec/function.c")
                local cursor = parser:getCursor()  
                cursor = get_last_child(cursor)
                storage_class = cursor:getStorageClass()
                assert.are.equal("extern", storage_class)
        end)
end)
function get_last_member(cursor)
        cursor:visitChildren(function(cursor, parent)
                last_member = parent   
                return "recurse"                     
        end)
        return last_member
end

describe("is bit field", function()
        it("succeeds in identifying a bit field", function()
                local parser = luaclang.newParser("spec/struct.c")
                local cursor = parser:getCursor()  
                cursor = get_last_child(cursor)
                local last_member = get_last_member(cursor)
                assert.are.equal(true, last_member:isBitField())
        end)

        it("uses an incompatible cursor with isBitField()", function()
                local parser = luaclang.newParser("spec/struct.c")
                local cursor = parser:getCursor()
                cursor = get_last_child(cursor)
                assert.has.errors(function()
                        local is_bit_field = cursor:isBitField()
                end, "calling 'isBitField' on bad self (expect cursor with struct/union field kind)")
        end)
end)

describe("get bit field width", function()
        it("succeeds in getting bit field width", function()
                local parser = luaclang.newParser("spec/struct.c")
                local cursor = parser:getCursor()  
                cursor = get_last_child(cursor)
                local last_member = get_last_member(cursor)
                assert.are.equal(9, last_member:getBitFieldWidth())
        end)

        it("uses an incompatible cursor with getBitFieldWidth()", function()
                local parser = luaclang.newParser("spec/enum.c")
                local cursor = parser:getCursor()
                cursor = get_last_child(cursor)
                assert.has.errors(function()
                        local bit_field_width = cursor:getBitFieldWidth()
                end, "calling 'getBitFieldWidth' on bad self (expect cursor with struct/union field kind that is a bit field)")
        end)
end)

describe("get typedef underlying type", function()
        it("succeeds in getting bit field width", function()
                local parser = luaclang.newParser("spec/typedef.c")
                local cursor = parser:getCursor()  
                cursor = get_last_child(cursor)
                local underlying_type
                assert.has_no.errors(function()
                        underlying_type = cursor:getTypedefUnderlyingType()
                end)
                underlying_type_str = underlying_type:getSpelling()
                assert.are.equal("GROUP *", underlying_type_str)
        end)

        it("uses an incompatible cursor with getTypedefUnderlying()", function()
                local parser = luaclang.newParser("spec/enum.c")
                local cursor = parser:getCursor()
                cursor = get_last_child(cursor)
                assert.has.errors(function()
                        local underlying_type = cursor:getTypedefUnderlyingType()
                end, "calling 'getTypedefUnderlyingType' on bad self (expect cursor with typedef kind)")
        end)
end)
