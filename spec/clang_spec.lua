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
        it("succeeds in matching the expected output", function()
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
        
        it("succeeds in getting the spelling of", function()
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
                end, "spec/clang_spec.lua:125: myerror")
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