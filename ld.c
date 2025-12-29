#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "ElfFile.h"
#include "Linker.h"

void usage(char *executable) {
  printf("Usage : %s -o [OUTPUT_PATH] -f [FILE1] [FILE2]\n", executable);
}

int main(int argc, char **argv) {
  char *output_path = NULL;
  char *file1_path = NULL;
  char *file2_path = NULL;
  int opt;

  while ((opt = getopt(argc, argv, "o:f:")) != -1) {
    switch (opt) {
      case 'o':
        output_path = optarg;
        break;
      case 'f':
        file1_path = optarg;
        if (optind < argc) {
          file2_path = argv[optind];
        }
        break;
      default:
        usage(argv[0]);
        return EXIT_FAILURE;
    }
  }

  if (!output_path || !file1_path || !file2_path) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  ElfFile *f1 = NULL;
  ElfParsingStatus status = ElfFileNew(file1_path, &f1);
  if (status != Success) {
    fprintf(stderr, "Error loading %s, (%s)\n", file1_path,
            ElfParsingStatusToString(status));
    return EXIT_FAILURE;
  }
  ElfFile *f2 = NULL;
  status = ElfFileNew(file2_path, &f2);
  if (status != Success) {
    fprintf(stderr, "Error loading %s, (%s)\n", file1_path,
            ElfParsingStatusToString(status));
    return EXIT_FAILURE;
  }

  ElfFile *result = NULL;
  MergeFiles(&result, f1, f2);

  if (result) {
    if (ElfFileWriteToDisk(output_path, result) != Success) {
      fprintf(stderr, "An error occured writing file to disk %s\n",
              output_path);
    } else {
      printf("Linking successful at output %s\n", output_path);
    }
    ElfFileDestroy(result);
  }

  ElfFileDestroy(f1);
  ElfFileDestroy(f2);

  return EXIT_SUCCESS;
}