INCDIRS = $(shell llvm-config --includedir) -I/usr/include/lua5.3
LDFLAGS = $(shell llvm-config --ldflags)

# Generate luaclang.so
all: luaclang

luaclang: luaclang.c
	clang -I $(INCDIRS) $(LDFLAGS) luaclang.c -lclang -shared -fpic -o luaclang.so -Wall
	cp luaclang.so spec/

# Remove luaclang.so
clean:
	rm -f luaclang.so