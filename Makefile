CC := gcc
CC_ARM := arm-linux-gnueabihf-gcc
LDFLAGS +=
CFLAGS += -Wall -DREGEX_USE_ALLOC_LIBC
TESTS := build/host/debug/test build/host/production/test
TESTS_ARM := build/arm/debug/test build/arm/production/test
SRCS = src/regex.c

all: $(SRCS)
	@mkdir -p build/host/debug
	@mkdir -p build/host/production
	@mkdir -p build/arm/debug
	@mkdir -p build/arm/production
	cd src ; $(MAKE)
	$(MAKE) $(TESTS)
	$(MAKE) $(TESTS_ARM)

#
# test executables
#
build/host/debug/test: $(SRCS) test.c
	$(CC) -o $@ $^ $(CFLAGS) -O0 -g3 $(LDFLAGS)

build/host/production/test: $(SRCS) test.c
	$(CC) -o $@ $^ $(CFLAGS) -Os -DNDEBUG $(LDFLAGS)

build/arm/debug/test: $(SRCS) test.c
	$(CC_ARM) -o $@ $^ $(CFLAGS) -O0 -g3 -static $(LDFLAGS) -Wl,-s

build/arm/production/test: $(SRCS) test.c
	$(CC_ARM) -o $@ $^ $(CFLAGS) -Os -DNDEBUG -static $(LDFLAGS) -Wl,-s

check: $(TESTS)
	./build/host/debug/test

check_arm: $(TESTS_ARM)
	./build/arm/debug/test

gdb:
	gdb ./build/host/debug/test

clean:
	cd src ; $(MAKE) clean
	rm -f $(TESTS) $(TESTS_ARM)

.PHONY: test
