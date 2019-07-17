
# lua-clang
Lua binding for Libclang functions that are required to parse C declarations.

## Installation

Create a loadable shared object file( `luaclang.so` ) with :

    make

Load this module in Lua by :

    lib = require "luaclang"

Run tests :

 *In the root directory run -* 
   

     busted
