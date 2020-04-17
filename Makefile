CC := gcc
CC_ARM := arm-linux-gnueabihf-gcc
LDFLAGS +=

all: src/regex_light.h src/regex_light.c
	$(MAKE) host_debug host_product LDFLAGS=$(LDFLAGS)
	$(MAKE) arm_debug arm_product LDFLAGS="-static $(LDFLAGS)"
	$(MAKE) assert

arm_debug:
	$(CC_ARM) src/regex_light.c test.c -o test_a_d_static -Wall -O0 -g3 $(LDFLAGS)

arm_product:
	$(CC_ARM) src/regex_light.c test.c -o test_a_p_static -Wall -Os -DNDEBUG $(LDFLAGS) -Wl,-s

host_debug:
	$(CC) src/regex_light.c test.c -o test_h_d -Wall -O0 -g3 $(LDFLAGS)

host_product:
	$(CC) src/regex_light.c test.c -o test_h_p -Wall -Os -DNDEBUG $(LDFLAGS) -Wl,-s

assert:
	./test_h_d

gdb:
	gdb ./test_h_d

clean:
	rm test
