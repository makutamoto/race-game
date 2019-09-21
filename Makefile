SRCS := $(wildcard *.c)
OBJS_CLANG := $(patsubst %.c, objs/%.o, $(SRCS))
OBJS_BCC32 := $(patsubst %.c, objs/%.obj, $(SRCS))

clang: ./game-clang.exe
	@echo "Build complete."

bcc32: ./game-bcc32.exe
	@echo "Build complete."

clean:
	del objs\*.o objs\*.obj

game-clang.exe: $(OBJS_CLANG)
	clang ./objs/*.o -o game-clang.exe

objs/%.o: %.c
	clang -Wall -c $^ -o $@

game-bcc32.exe: $(OBJS_BCC32)
	bcc32 -egame-bcc32.exe ./objs/*.obj
	del game-bcc32.tds

objs/%.obj: %.c
	bcc32 -wAll -o"$@" -c $^
