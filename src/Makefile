
include ../mk/config.base.mk
include ../$(CONFIG)

EXTRA_CFLAGS = -fPIC -I../include/ -I/usr/include/opencv

SONAME = $(LIB_NAME).$(MAJOR)
LDFLAGS += -lpthread -lseawolf --shared -Wl,-soname,$(SONAME)
LDFLAGS += $(EXTRA_LDFLAGS)

INCLUDES= ../include/svr/*.h ../include/svr.h

SRC = allocators.c streambuffer.c blockalloc.c mempool.c message.c
OBJ = $(SRC:.c=.o)

all: $(LIB_NAME)

$(LIB_NAME): $(OBJ) 
	$(CC) $(LDFLAGS) $(OBJ) -o $(LIB_NAME)

.c.o:
	$(CC) $(EXTRA_CFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ): $(INCLUDES)

clean:
	-rm -f $(OBJ) $(LIB_NAME) 2> /dev/null

install: $(LIB_NAME)
	install -m 0755 -d $(PREFIX)/include $(PREFIX)/include/svr $(PREFIX)/bin $(PREFIX)/lib
	install -m 0644 $(LIB_NAME) $(PREFIX)/lib/$(LIB_NAME).$(MAJOR).$(MINOR).$(REV)
	install -m 0644 ../include/svr.h $(PREFIX)/include/
	install -m 0644 ../include/svr/* $(PREFIX)/include/svr/
	ln -sf $(PREFIX)/lib/$(LIB_NAME).$(MAJOR).$(MINOR).$(REV) $(PREFIX)/lib/$(LIB_NAME).$(MAJOR)
	ln -sf $(PREFIX)/lib/$(LIB_NAME).$(MAJOR).$(MINOR).$(REV) $(PREFIX)/lib/$(LIB_NAME)

uninstall:
	-rm $(PREFIX)/lib/$(LIB_NAME)*
	-rm $(PREFIX)/include/svr.h
	-rm -r $(PREFIX)/include/svr

.PHONY: all clean install uninstall
