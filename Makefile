CC := gcc

all: src/regex_light.h src/regex_light.c
	$(MAKE) build
	$(MAKE) assert

build: src/regex_light.h src/regex_light.c
	$(CC) src/regex_light.c test.c -o test -Wall -O0 -g3

assert:
	./test

clean:
	rm test
