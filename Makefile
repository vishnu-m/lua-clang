INCDIRS = $(shell llvm-config --includedir) -I/usr/include/lua5.3
LDFLAGS = $(shell llvm-config --ldflags)

all: luaclang

luaclang: 
	clang -I $(INCDIRS) $(LDFLAGS) src/luaclang.c -lclang -shared -fpic -o luaclang.so -Wall

clean:
	rm -f lib.so