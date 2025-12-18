ACLOCAL_AMFLAGS = -I m4
SUBDIRS = . Examples_fusion

AM_CFLAGS = -Wall -Werror -g -DDEBUG -DWARNING
# Unity needs to know where its source is
UNITY_DIR = $(top_srcdir)/tests/Unity/src
AM_CPPFLAGS = -I$(srcdir)/include -I$(srcdir) -I$(UNITY_DIR)

# --- Internal Static Library ---
noinst_LIBRARIES = libelflinker.a
libelflinker_a_SOURCES = \
    debug.c util.c src/ascii.c src/display_elf_header.c \
    src/logger.c src/extract_elf_header.c src/elf_ops.c src/extract_elf_section.c

# --- Main Application ---
bin_PROGRAMS = main
main_SOURCES = main.c
main_LDADD = libelflinker.a

# --- Unit Tests (Pure Unity) ---
check_PROGRAMS = test_extract_elf_headers
TESTS = $(check_PROGRAMS)

# We only need the unity.c file and our test file
test_extract_elf_headers_SOURCES = tests/test_extract_elf_headers.c $(UNITY_DIR)/unity.c
test_extract_elf_headers_LDADD = libelflinker.a

EXTRA_DIST = filtre.pl .gitignore