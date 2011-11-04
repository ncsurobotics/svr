
include ../mk/config.base.mk
include ../$(CONFIG)

EXTRA_CFLAGS = -I../include/ $(CV_CFLAGS)
LDFLAGS += -L../src/ -l$(LIB_NAME:lib%.so=%) -lpthread $(CV_LDFLAGS)

all: test

test: test.c
	$(CC) $(EXTRA_CFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@

clean:
	-rm -f test 2> /dev/null

.PHONY: all clean
