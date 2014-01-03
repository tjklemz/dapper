######################################################################
#
# Makefile
#
# Copyright 2014 Thomas Klemz
#
# Licensed under GPLv3 or later at your choice.
# You are free to use, distribute, and modify this
# as long as you grant the same freedoms to others.
# See the file COPYING for the full license.
######################################################################

BINARY = dapper

CC = cc -std=c99 -Wall -Os

ifdef DEBUG
	CC += -g
endif

ifeq ($(OS),Windows_NT)
	PLAT = win32
	BINARY = $(APPNAME).exe
else
	UNAME = $(shell uname)
	PLAT = nix
	
	ifeq ($(UNAME),Darwin)
		PLAT = mac
	endif
endif

ifeq ($(PLAT),win32)
	CC += -m32
	ARCH = x86
else
	ARCH = x$(shell getconf LONG_BIT)
endif	

SRCDIR = src
LIBDIR = lib/$(PLAT)/$(ARCH)

SRC = \
  main.c \
  $(NULL)

COMMON_LIBS = -lm -lglfw3 -lGLEW

ifeq ($(PLAT),win32)
	OS_LIBS = -luser32 -lgdi32 -lkernel32
	GL_LIBS = -lopengl32 -lglu32
endif

ifeq ($(PLAT),mac)
	OS_LIBS	= -framework Cocoa -framework IOKit -framework QuartzCore
	GL_LIBS = -framework OpenGL
else ifeq ($(PLAT),nix)
	OS_LIBS = -lX11
	GL_LIBS = -lGL -lGLU
endif

SOURCE = $(addprefix $(SRCDIR)/, $(SRC) $(MAIN_SRC))
LDFLAGS = $(COMMON_LIBS) $(OS_LIBS) $(GL_LIBS)
CFLAGS = -L$(LIBDIR) -I./include


######################################################################
# Compiling the app
######################################################################

.PHONY: all
all: $(BINARY)

$(BINARY): $(SOURCE)
	$(CC) $(SOURCE) -o $(BINARY) $(CFLAGS) $(LDFLAGS)
