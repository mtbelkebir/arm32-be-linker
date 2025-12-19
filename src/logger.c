/**
 * @file logger.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-12-12
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "../include/logger.h"
#include <stdio.h>
#include <stdlib.h>

void print_error(const unsigned char message[]) {
  printf("\033[91m[ ❌ ERROR ] %s\033[0m\n", message);
  exit(1);
}
void print_warning(const unsigned char message[]) {
  printf("\033[93m[ ⚠️ WARNING ] %s\033[0m\n", message);
}
void print_notification(const unsigned char message[]) {
  printf("\033[96m[ 💬 NOTIFICATIONS ] %s\033[0m\n", message);
}
void print_information(const unsigned char message[]) {
  printf("\033[92m[ ✅ ] %s\033[0m\n", message);
}

void handler_extract_status(EXTRACT_STATUS extract_status) {
  switch (extract_status) {
  case SUCCESS_EXTRACT:
    break;
  case ERROR_ELF_FILE_END_OF_FILE_UNEXPECTED:
    print_error((unsigned char *)"EOF unexpected !");
    break;
  case ERROR_ELF_FILE_READING:
    perror("Error while reading ELF file");
    break;
  case ERROR_ELF_FILE_UNKNOW:
    print_error((unsigned char *)"Unknow error while reading ELF file");
    break;
  default:
    print_warning((unsigned char *)"Extract status not recognized");
    break;
  }
}