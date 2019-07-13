INCDIRS = $(shell llvm-config --includedir) -I/usr/include/lua5.3
LDFLAGS = $(shell llvm-config --ldflags)

# Generate luaclang.so
all: luaclang

luaclang: src/luaclang.c
	clang -I $(INCDIRS) $(LDFLAGS) src/luaclang.c -lclang -shared -fpic -o luaclang.so -Wall

# Run all tests 
test_all: src/luaclang.c
	clang -I $(INCDIRS) $(LDFLAGS) src/luaclang.c -lclang -shared -fpic -o tests/luaclang.so -Wall
	cd tests; bash test_all.sh

# Remove luaclang.so
clean:
	rm -f luaclang.so