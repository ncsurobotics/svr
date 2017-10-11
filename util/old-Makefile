
include ../mk/config.base.mk
include ../$(CONFIG)

EXTRA_CFLAGS = -I../include/ $(CV_CFLAGS)
LDFLAGS += -L../lib/ -l$(LIB_NAME) -lseawolf
LDFLAGS += $(CV_LDFLAGS)

SRCS=$(wildcard *.c)
TOOLS=$(SRCS:%.c=%)

all: $(TOOLS)

%: %.c
	$(CC) $(EXTRA_CFLAGS) $(CFLAGS) $< $(LDFLAGS) -o $@

install: all
	install -m 755 $(TOOLS) $(PREFIX)/bin

uninstall:
	-rm $(TOOLS:%=$(PREFIX)/bin/%)

clean:
	-rm -f $(TOOLS) 2> /dev/null

.PHONY: all clean install uninstall
