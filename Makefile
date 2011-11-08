
include mk/config.base.mk
include $(CONFIG)

# Ensure that PREFIX is saved as an absolute path
export PREFIX := $(abspath $(PREFIX))

all: $(LIB_NAME) $(SERVER_NAME) tests

$(LIB_NAME):
	cd src && $(MAKE) $@

$(SERVER_NAME):
	cd src/server/ && $(MAKE) $@
	
tests:
	cd test && $(MAKE)

clean:
	cd src && $(MAKE) $@
	cd src/server/ && $(MAKE) $@
	-rm -rf doc/html/ 2> /dev/null
	-rm -rf doc/server/html/ 2> /dev/null

install uninstall:
	cd src && $(MAKE) $@
ifneq ($(strip $(SERVER_NAME)),)
	cd src/server/ && $(MAKE) $@
endif

doc:
	doxygen doc/Doxyfile

doc-hub:
	doxygen doc/server/Doxyfile

.PHONY: all clean install uninstall doc
