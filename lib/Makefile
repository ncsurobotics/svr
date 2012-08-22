
include ../mk/config.base.mk
include ../$(CONFIG)

EXTRA_CFLAGS = -fPIC -I../include/ $(CV_CFLAGS)

LDFLAGS += -lpthread -lseawolf -ljpeg
LDFLAGS += $(CV_LDFLAGS)
LDFLAGS += $(EXTRA_LDFLAGS)

INCLUDES= ../include/svr/*.h ../include/svr.h

SRC = blockalloc.c mempool.c message.c pack.c net.c logging.c refcount.c	\
	frameproperties.c encoding.c lockable.c main.c encodings/raw.c		\
	responseset.c messagerouting.c messagehandlers.c stream.c source.c	\
	comm.c optionstring.c encodings/jpeg.c
OBJ = $(SRC:.c=.o)

all: $(LIB_FILE)

$(LIB_FILE): $(OBJ)
	$(CC) $(LDFLAGS) $(LIB_LDFLAGS) $(OBJ) -o $(LIB_FILE)
	ln -sf $(LIB_FILE) $(LIB_FILE_BASE)

.c.o:
	$(CC) $(EXTRA_CFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ): $(INCLUDES)

clean:
	-rm -f $(OBJ) $(LIB_FILE) $(LIB_FILE_BASE) 2> /dev/null

install: $(LIB_FILE)
	install -m 0755 -d $(PREFIX)/include $(PREFIX)/include/svr $(PREFIX)/bin $(PREFIX)/lib
	install -m 0644 ../include/svr.h $(PREFIX)/include/
	install -m 0644 ../include/svr/*.h $(PREFIX)/include/svr/
	install -m 0644 $(LIB_FILE) $(PREFIX)/lib/$(LIB_FILE)
	ln -sf $(PREFIX)/lib/$(LIB_FILE) $(PREFIX)/lib/$(LIB_FILE_BASE)

uninstall:
	-rm $(PREFIX)/lib/$(LIB_FILE)
	-rm $(PREFIX)/lib/$(LIB_FILE_BASE)
	-rm $(PREFIX)/include/svr.h
	-rm -r $(PREFIX)/include/svr

.PHONY: all clean install uninstall
