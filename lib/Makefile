
include ../mk/config.base.mk
include ../$(CONFIG)

EXTRA_CFLAGS = -fPIC -I../include/ $(CV_CFLAGS)

SONAME = $(LIB_NAME).$(MAJOR)
LDFLAGS += -lpthread -lseawolf -ljpeg --shared -Wl,-soname,$(SONAME)
LDFLAGS += $(CV_LDFLAGS)
LDFLAGS += $(EXTRA_LDFLAGS)

INCLUDES= ../include/svr/*.h ../include/svr.h

SRC = blockalloc.c mempool.c message.c pack.c net.c logging.c refcount.c	\
	frameproperties.c encoding.c lockable.c main.c encodings/raw.c		\
	responseset.c messagerouting.c messagehandlers.c stream.c source.c	\
	comm.c optionstring.c encodings/jpeg.c
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
	install -m 0644 ../include/svr/*.h $(PREFIX)/include/svr/
	ln -sf $(PREFIX)/lib/$(LIB_NAME).$(MAJOR).$(MINOR).$(REV) $(PREFIX)/lib/$(LIB_NAME).$(MAJOR)
	ln -sf $(PREFIX)/lib/$(LIB_NAME).$(MAJOR).$(MINOR).$(REV) $(PREFIX)/lib/$(LIB_NAME)

uninstall:
	-rm $(PREFIX)/lib/$(LIB_NAME)*
	-rm $(PREFIX)/include/svr.h
	-rm -r $(PREFIX)/include/svr

.PHONY: all clean install uninstall
