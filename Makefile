SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, objs/%.o, $(SRCS))

build: ./game.exe
	@echo "Build complete."

clean:
	del objs\*.o

run: build
		@game.exe

game.exe: $(OBJS)
	clang ./objs/*.o -o game.exe

objs/%.o: %.c
	clang -Wall -c $^ -o $@
