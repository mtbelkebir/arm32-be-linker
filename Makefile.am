ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -Wall -Werror -g -DDEBUG -DWARNING
# Uncomment if performance when running with -DDEBUG is an issue
# Warning, if uncommented, issuing calls to debug functions during options
# parsing might result in debug flag incorrectly set to 0 for some files
#AM_CFLAGS+=-D CACHE_DEBUG_FLAG
# Unity needs to know where its source is
UNITY_DIR = $(top_srcdir)/tests/Unity/src
AM_CPPFLAGS = -I$(srcdir)/include -I$(srcdir) -I$(UNITY_DIR)
SRC_FOLDER=src
SOURCES=     $(SRC_FOLDER)/ascii.c \
             $(SRC_FOLDER)/display_elf_header.c \
             $(SRC_FOLDER)/logger.c \
             $(SRC_FOLDER)/extract_elf_header.c \
             $(SRC_FOLDER)/elf_ops.c \
             $(SRC_FOLDER)/extract_elf_section.c \
             $(SRC_FOLDER)/extract_elf_sym.c \
             $(SRC_FOLDER)/display_elf_sym.c

SUBDIRS=
SUBDIRS+=. Examples_fusion

bin_PROGRAMS=
bin_PROGRAMS+=main


COMMON=debug.h debug.c util.h util.c

EXTRA_DIST=filtre.pl .gitignore
main_SOURCES=$(COMMON) main.c $(SOURCES)

EXTRA_DIST+=

