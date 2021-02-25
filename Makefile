CC := gcc
CC_ARM := arm-linux-gnueabihf-gcc
LDFLAGS +=
CFLAGS += -Wall
OBJS := build/host/debug/regex.o build/host/production/regex.o build/arm/debug/regex.o build/arm/production/regex.o
TESTS := build/host/debug/test build/host/production/test build/arm/debug/test build/arm/production/test
TESTS_LIBC := build/host/debug/test_libc build/host/production/test_libc build/arm/debug/test_libc build/arm/production/test_libc
SRCS := src/regex_light.c

all:
	@mkdir -p build/host/debug
	@mkdir -p build/host/production
	@mkdir -p build/arm/debug
	@mkdir -p build/arm/production
	$(MAKE) objs

objs: $(OBJS)

tests: $(TESTS)

tests_libc: $(TESTS_LIBC)

#
# object files
#
build/host/debug/regex.o: $(SRCS)
	$(CC) -c -o $@ $^ $(CFLAGS) -O0 -g3

build/host/production/regex.o: $(SRCS)
	$(CC) -c -o $@ $^ $(CFLAGS) -Os -DNDEBUG

build/arm/debug/regex.o: $(SRCS)
	$(CC_ARM) -c -o $@ $^ $(CFLAGS) -O0 -g3

build/arm/production/regex.o: $(SRCS)
	$(CC_ARM) -c -o $@ $^ $(CFLAGS) -Os -DNDEBUG

#
# test executables
#
build/host/debug/test: build/host/debug/regex.o test.c
	$(CC) -o $@ $^ $(CFLAGS) -O0 -g3 $(LDFLAGS)

build/host/production/test: build/host/production/regex.o test.c
	$(CC) -o $@ $^ $(CFLAGS) -Os -DNDEBUG $(LDFLAGS)

build/arm/debug/test: build/arm/debug/regex.o test.c
	$(CC_ARM) -o $@ $^ $(CFLAGS) -O0 -g3 -static $(LDFLAGS) -Wl,-s

build/arm/production/test: build/arm/production/regex.o test.c
	$(CC_ARM) -o $@ $^ $(CFLAGS) -Os -DNDEBUG -static $(LDFLAGS) -Wl,-s

#
# test executables with glibc
#
build/host/debug/test_libc: test.c
	$(CC) -o $@ $^ $(CFLAGS) -O0 -g3 $(LDFLAGS)

build/host/production/test_libc: test.c
	$(CC) -o $@ $^ $(CFLAGS) -Os -DNDEBUG $(LDFLAGS)

build/arm/debug/test_libc: test.c
	$(CC_ARM) -o $@ $^ $(CFLAGS) -O0 -g3 -static $(LDFLAGS) -Wl,-s

build/arm/production/test_libc: test.c
	$(CC_ARM) -o $@ $^ $(CFLAGS) -Os -DNDEBUG -static $(LDFLAGS) -Wl,-s

check: tests
	./build/host/debug/test

gdb:
	gdb ./build/host/debug/test

clean:
	rm -f $(OBJS) $(TESTS) $(TESTS_LIBC)

.PHONY: test
