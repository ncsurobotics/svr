
include ../../mk/config.base.mk
include ../../$(CONFIG)

EXTRA_CFLAGS = -I../../include/
LDFLAGS += -L../ -l$(LIB_NAME:lib%.so=%) -lpthread

INCLUDES= ../../include/svr/*.h ../../include/svr/server/*.h ../../include/svr.h

SRC= client.c event.c main.c fullreencoder.c messagehandlers.c messagerouting.c server.c source.c stream.c
OBJ= $(SRC:.c=.o)

all: $(SERVER_NAME)

$(SERVER_NAME): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $(SERVER_NAME)

.c.o:
	$(CC) $(EXTRA_CFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ): $(INCLUDES)

clean:
	-rm -f $(OBJ) $(HUB_NAME) 2> /dev/null

install: $(SERVER_NAME)
	install -m 0755 $(SERVER_NAME) $(PREFIX)/bin

uninstall:
	-rm $(PREFIX)/bin/$(SERVER_NAME)

.PHONY: all clean install uninstall
