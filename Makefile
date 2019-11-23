SRCS := $(wildcard *.c)
OBJS_CLANG := $(patsubst %.c, objs/%.o, $(SRCS))
OBJS_BCC32 := $(patsubst %.c, objs/%.obj, $(SRCS))

clang: ./game-clang.exe
	@echo "Build complete."

bcc32: ./game-bcc32.exe
	@echo "Build complete."

clean:
	del objs\*.o objs\*.obj
	cd cnsglib && make clean

game-clang.exe: cnsglib-clang $(OBJS_CLANG)
	clang ./objs/*.o ./cnsglib/objs/*.o -o game-clang.exe

cnsglib-clang:
	cd cnsglib && make

objs/%.o: %.c ./include/*.h
	clang -Wall -c $< -o $@ -Wno-invalid-source-encoding

game-bcc32.exe: cnsglib-bcc32 $(OBJS_BCC32)
	bcc32 -egame-bcc32.exe ./objs/*.obj ./cnsglib/objs/*.obj
	del game-bcc32.tds

cnsglib-bcc32:
	cd cnsglib && make bcc32

objs/%.obj: %.c ./include/*.h
	bcc32 -wAll -o"$@" -c $<
