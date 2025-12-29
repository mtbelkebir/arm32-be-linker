#ifndef ELF_MERGE_H
#define ELF_MERGE_H
#include "ElfFile.h"

ElfFile* ElfMergeSections(ElfFile* f1, ElfFile* f2);
#endif  // ELF_MERGE_H
