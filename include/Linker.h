#ifndef ELF_MERGE_H
#define ELF_MERGE_H
#include "ElfFile.h"
typedef enum LinkerStatus {
  LinkerSuccess,
  LinkerDuplicateSymbol,
  LinkerMemoryError,
  LinkerUndefinedSymbol,
  LinkerUnknownError,
} LinkerStatus;

LinkerStatus MergeFiles(ElfFile** out, ElfFile* f1, ElfFile* f2);


const char* LinkerStatusToString(LinkerStatus status);
#endif  // ELF_MERGE_H
