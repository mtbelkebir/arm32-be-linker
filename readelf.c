#include <elf.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ElfFile.h"
#include "debug.h"

void usage(char *name) {
  fprintf(
      stderr,
      "Usage:\n"
      "%s [ --help ] [ --copy source dest ] [ --header file ] [ "
      "--section-table file ]\n"
      "   [ --hex-dump section file ] [ --symbols file ] [ --relocations file "
      "] [ --debug file ]\n\n"
      "Options:\n"
      "  -c, --copy src dst      Copies an ELF file (Load then Save to disk)\n"
      "  -H, --header file       Displays ELF header\n"
      "  -S, --section-table     Displays section table\n"
      "  -x, --hex-dump sec file Displays hex dump of a section\n"
      "  -s, --symbols file      Displays symbol table\n"
      "  -r, --relocations file  Displays relocations\n",
      name);
}

int main(int argc, char *argv[]) {
  int opt;
  ElfFile *f = NULL;
  ElfParsingStatus status;

  struct option longopts[] = {{"debug", required_argument, NULL, 'd'},
                              {"header", required_argument, NULL, 'H'},
                              {"section-table", required_argument, NULL, 'S'},
                              {"hex-dump", required_argument, NULL, 'x'},
                              {"symbols", required_argument, NULL, 's'},
                              {"relocations", required_argument, NULL, 'r'},
                              {"copy", required_argument, NULL, 'c'},
                              {"help", no_argument, NULL, 'h'},
                              {NULL, 0, NULL, 0}};

  char *filename_src = NULL;
  char *filename_dst = NULL;
  char *section_name = NULL;

  while ((opt = getopt_long(argc, argv, "d:H:S:x:s:h:r:c:", longopts, NULL)) !=
         -1) {
    switch (opt) {
      case 'c':  // Option de copie
        filename_src = optarg;
        if (optind < argc) {
          filename_dst = argv[optind++];
          // 1. Chargement du fichier source
          status = ElfFileNew(filename_src, &f);
          if (status != Success) {
            fprintf(stderr, "Error loading source %s: %s\n", filename_src,
                    ElfParsingStatusToString(status));
          } else {
            // 2. Écriture immédiate vers la destination
            status = ElfFileWriteToDisk(filename_dst, f);
            if (status != Success) {
              fprintf(stderr, "Error writing to %s: %s\n", filename_dst,
                      ElfParsingStatusToString(status));
            } else {
              printf("Successfully copied %s to %s\n", filename_src,
                     filename_dst);
            }
            ElfFileDestroy(f);
          }
        } else {
          fprintf(stderr,
                  "Option -c requires a source filename AND a destination "
                  "filename\n");
        }
        break;

      case 'r':
        status = ElfFileNew(optarg, &f);
        if (status != Success) {
          fprintf(stderr, "Error parsing file %s: %s\n", optarg,
                  ElfParsingStatusToString(status));
        } else {
          ElfFileDisplayRelocations(f);
          ElfFileDestroy(f);
        }
        break;

      case 'H':
        status = ElfFileNew(optarg, &f);
        if (status != Success) {
          fprintf(stderr, "Error parsing file %s: %s\n", optarg,
                  ElfParsingStatusToString(status));
        } else {
          ElfFileDisplayHeader(f);
          ElfFileDestroy(f);
        }
        break;

      case 'S':
        status = ElfFileNew(optarg, &f);
        if (status != Success) {
          fprintf(stderr, "Error parsing file %s: %s\n", optarg,
                  ElfParsingStatusToString(status));
        } else {
          ElfFileDisplaySections(f);
          ElfFileDestroy(f);
        }
        break;

      case 'x':
        section_name = optarg;
        if (optind < argc) {
          filename_src = argv[optind++];
          status = ElfFileNew(filename_src, &f);
          if (status != Success) {
            fprintf(stderr, "Error parsing file %s: %s\n", filename_src,
                    ElfParsingStatusToString(status));
          } else {
            printf("Hex dump of section '%s':\n", section_name);
            ElfFileDisplaySectionContentsByName(section_name, f);
            ElfFileDestroy(f);
          }
        } else {
          fprintf(stderr, "Option -x requires a section name AND a filename\n");
        }
        break;

      case 's':
        status = ElfFileNew(optarg, &f);
        if (status != Success) {
          fprintf(stderr, "Error parsing file %s: %s\n", optarg,
                  ElfParsingStatusToString(status));
        } else {
          ElfFileDisplaySymbols(f);
          ElfFileDestroy(f);
        }
        break;

      case 'd':
        add_debug_to(optarg);
        break;

      case 'h':
        usage(argv[0]);
        return 0;

      default:
        usage(argv[0]);
        exit(1);
    }
  }
  return 0;
}