# changez la variable NAME en y mettant votre nom !
NAME = Traini


# ADD -DNDEBUG to remove assertions
FLAGS = -std=c99 -Wall -Wextra -pedantic -Werror -O4 -DNDEBUG
# FLAGS = -std=c99 -Wall -Wextra -pedantic -Wno-unused-parameter -Wno-unused-variable -O4
# FLAGS = -std=c99 -Wall -Wextra -pedantic -Werror -O0 -pg # -no-pie
LFLAGS =

GCC = gcc
# GCC = clang

FILES = main.c utils.c print.c test-$(NAME).c naive.c solve-$(NAME).c
O_FILES = $(FILES:.c=.o)

all: sat

%.o: %.c sat.h
	$(GCC) $(FLAGS) -c $<

sat: $(O_FILES)
	$(GCC) $(FLAGS) $(LFLAGS) $(O_FILES) -o sat

clean:
	rm -f *.o a.out gmon.out callgrind.out*

veryclean: clean
	rm -f sat
	rm -rf TP2-info501-NOM/ TP2-info501.tgz

.PHONY: all clean veryclean
