
# Build options for Linux
CFLAGS += -D__SVR_Linux__ -pthread

# Install to /usr/local if no alternate is given
PREFIX ?= /usr/local

# Linux and FreeBSD both have POSIX real time extensions in a separate library
EXTRA_LDFLAGS = -lrt
