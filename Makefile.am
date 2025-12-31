ACLOCAL_AMFLAGS = -I m4
SUBDIRS = . Examples_fusion

AM_CFLAGS = -Wall -Werror -g -DDEBUG -DWARNING
# Unity needs to know where its source is
UNITY_DIR = $(top_srcdir)/tests/Unity/src
AM_CPPFLAGS = -I$(srcdir)/include -I$(srcdir) -I$(UNITY_DIR)

# --- Internal Static Library ---
noinst_LIBRARIES = libelflinker.a
libelflinker_a_SOURCES = \
    debug.c util.c src/ElfFile.c src/Linker.c

# --- Main Application ---
bin_PROGRAMS = readelf ld
readelf_SOURCES = readelf.c
readelf_LDADD = libelflinker.a

ld_SOURCES_ = ld.c
ld_LDADD = libelflinker.a
# --- Unit Tests (Pure Unity) ---
check_PROGRAMS = test_ElfFile
TESTS = $(check_PROGRAMS)

# We only need the unity.c file and our test file
test_ElfFile_SOURCES = tests/test_ElfFile.c $(UNITY_DIR)/unity.c
test_ElfFile_LDADD = libelflinker.a

EXTRA_DIST = filtre.pl .gitignore