# See LICENSE file for license and copyright information
# jumanji make config

VERSION = 0.0.0

# paths
PREFIX ?= /usr
MANPREFIX ?= ${PREFIX}/share/man

# libs
GTK_INC = $(shell pkg-config --cflags gtk+-2.0 webkit-1.0 unique-1.0)
GTK_LIB = $(shell pkg-config --libs gtk+-2.0 gthread-2.0 webkit-1.0 unique-1.0)

INCS = -I. -I/usr/include ${GTK_INC}
LIBS = -lc ${GTK_LIB} -lpthread

# flags
CFLAGS += -std=c99 -pedantic -Wall -Wextra $(INCS)

# debug
DFLAGS = -O0 -g

# compiler
CC ?= gcc

# strip
SFLAGS = -s
