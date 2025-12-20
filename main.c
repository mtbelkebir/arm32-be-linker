/*
ELF Loader - chargeur/implanteur d'exécutables au format ELF à but pédagogique
Copyright (C) 2012 Guillaume Huard
Ce programme est libre, vous pouvez le redistribuer et/ou le modifier selon les
termes de la Licence Publique Générale GNU publiée par la Free Software
Foundation (version 2 ou bien toute autre version ultérieure choisie par vous).

Ce programme est distribué car potentiellement utile, mais SANS AUCUNE
GARANTIE, ni explicite ni implicite, y compris les garanties de
commercialisation ou d'adaptation dans un but spécifique. Reportez-vous à la
Licence Publique Générale GNU pour plus de détails.

Vous devez avoir reçu une copie de la Licence Publique Générale GNU en même
temps que ce programme ; si ce n'est pas le cas, écrivez à la Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307,
États-Unis.

Contact: Guillaume.Huard@imag.fr
         ENSIMAG - Laboratoire LIG
         51 avenue Jean Kuntzmann
         38330 Montbonnot Saint-Martin
*/
#include "debug.h"
#include "elf_ops.h"
#include "logger.h"
#include <elf.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "elf_ops.h"

void usage(char *name)
{
  fprintf(stderr,
          "Usage:\n"
          "%s [ --help ] [ --option1 value ] [ --option2 value ] [ --debug "
          "file ] file\n\n"
          "Prints values given as option. The --debug flag enables the output "
          "produced by "
          "calls to the debug function in the named source file.\n",
          name);
}

void sample_function(char *option1, char *option2)
{
  debug("Beginning of the sample function\n");
  debug("Given values are [ %s ] and [ %s ], time to print them:\n", option1,
        option2);
  printf("Option 1: %s\n", option1);
  printf("Option 2: %s\n", option2);
  debug("End of the sample function\n");
}

void header_elf(FILE *elf_file) {
  // Elf32_Ehdr elf = extract_elf_informations(elf_file);

  print_elf_header(&elf);
}

int main(int argc, char *argv[])
{
  int opt;

  struct option longopts[] = {{"debug", required_argument, NULL, 'd'},
                              {"option1", required_argument, NULL, '1'},
                              {"option2", required_argument, NULL, '2'},
                              {"header", required_argument, NULL, 'e'},
                              {"help", no_argument, NULL, 'h'},
                              {NULL, 0, NULL, 0}};

  char *filename_obj = NULL;

  while ((opt = getopt_long(argc, argv, "1:2:e:d:h", longopts, NULL)) != -1) {
    switch (opt) {
    case '1':
      break;
    case '2':
      Elf32_File *f = read_elf(optarg);
      // display_elf_sections(f);
      // display_elf_section_contents(".text", f);

      // Essai pour la table des symboles:
      display_sym_tab(f);
      free_elf_file(f);
      break;
    case 'h':
      usage(argv[0]);
      exit(0);
    case 'e':
      // Get the filename:
      filename_obj = optarg;

      Elf32_File file;
      file.file = fopen(filename_obj, "rb");
      file.e_ehdr = *initialize_ehdr();

      if (extract_elf_informations(&file.e_ehdr, file.file) !=
          SUCCESS_EXTRACT) {
        printf("Erreur extraction EHDR");
      }

      file.e_shrdrs =
          initialize_shdr(file.e_ehdr.e_shentsize, file.e_ehdr.e_shnum);

      if (extract_section_headers(file.e_shrdrs, file.file, file.e_ehdr.e_shoff,
                                  file.e_ehdr.e_shnum,
                                  file.e_ehdr.e_ident) != SUCCESS_EXTRACT) {
        printf("Erreur extraction SHDRS");
      }

      file.sym = initialize_sym(file.e_shrdrs, file.e_ehdr.e_shnum, file.file);

      if (extract_sym(file.sym, file.e_ehdr.e_ident, file.e_shrdrs,
                      file.e_ehdr.e_shnum, file.file) != SUCCESS_EXTRACT) {
        printf("Erreur extraction SYM");
      }

      display_elf_headers(&file.e_ehdr);
      display_elf_sections(&file);
      display_sym_tab(&file);

      // free_ehdr(&file.e_ehdr);
      free_shdr(file.e_shrdrs);
      free_sym(file.sym);

      return 0;
      break;
    case 'd':
      add_debug_to(optarg);
      break;
    default:
      fprintf(stderr, "Unrecognized option %c\n", opt);
      usage(argv[0]);
      exit(1);
    }
  }

  // sample_function(option1, option2);
  return 0;
}
