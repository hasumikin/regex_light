CC := gcc

all: src/regex_light.h src/regex_light.c
	$(CC) src/regex_light.c test.c -o test -O0 -g3
	$(MAKE) assert

assert:
	./test

clean:
	rm test
