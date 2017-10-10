
include ../mk/config.base.mk
include ../$(CONFIG)

EXTRA_CFLAGS = -I../include/ -Iinclude $(CV_CFLAGS)
LDFLAGS += -L../lib/ -l$(LIB_NAME) -lseawolf -lpthread $(CV_LDFLAGS)

INCLUDES= ../include/svr/*.h ../include/svr.h include/svrd/*.h include/svrd.h

SRC= client.c event.c main.c messagehandlers.c messagerouting.c server.c \
	source.c stream.c sources/test.c sources/cam.c sources/file.c sources/v4l.c
OBJ= $(SRC:.c=.o)

all: $(SERVER_NAME)

$(SERVER_NAME): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(SERVER_NAME)

.c.o:
	$(CC) $(EXTRA_CFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ): $(INCLUDES)

clean:
	-rm -f $(OBJ) $(SERVER_NAME) 2> /dev/null

install: $(SERVER_NAME)
	install -m 0755 $(SERVER_NAME) $(PREFIX)/bin

uninstall:
	-rm $(PREFIX)/bin/$(SERVER_NAME)

.PHONY: all clean install uninstall
