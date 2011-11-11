
include mk/config.base.mk
include $(CONFIG)

# Ensure that PREFIX is saved as an absolute path
export PREFIX := $(abspath $(PREFIX))

all: $(LIB_NAME) $(SERVER_NAME)

$(LIB_NAME):
	cd lib && $(MAKE) $@

$(SERVER_NAME):
	cd server && $(MAKE) $@

python:
	cd python && $(PYTHON) setup.py build

python-install:
	cd python && $(PYTHON) setup.py install

tests:
	cd test && $(MAKE)

clean:
	cd lib && $(MAKE) $@
	cd server && $(MAKE) $@
	-rm -rf doc/html/ 2> /dev/null
	-rm -rf doc/server/html/ 2> /dev/null

install uninstall:
	cd lib && $(MAKE) $@
ifneq ($(strip $(SERVER_NAME)),)
	cd server && $(MAKE) $@
endif

doc:
	doxygen doc/Doxyfile

doc-hub:
	doxygen doc/server/Doxyfile

.PHONY: all clean install uninstall doc python python-install tests
