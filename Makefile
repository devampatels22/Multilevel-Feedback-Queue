LDLIBS = -lpthread
CFLAGS = -Wall -Wextra -Wpedantic -Werror -g
CC = clang

.PHONY: clean

all: a3q1

clean:
	rm -f a3q1
