FLAGS=-Wextra -Wall -ggdb -I./raylib-5.0_linux_amd64/include -L./raylib-5.0_linux_amd64/lib
LIBS=-l:libraylib.a -lm
ciano: main.c
	gcc -o ciano $(FLAGS) main.c $(LIBS)
