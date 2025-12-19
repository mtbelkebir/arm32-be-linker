/**
 * @file logger.h
 * @author DUC Corentin
 * @brief File to handle some common errors
 * @version 0.1
 * @date 2025-12-12
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef __LOGGER__
#define __LOGGER__

#define LOG_FILE_PATH logs.data

typedef enum {
  SUCCESS_EXTRACT,
  ERROR_ELF_FILE_END_OF_FILE_UNEXPECTED,
  ERROR_ELF_FILE_READING,
  ERROR_ELF_FILE_UNKNOW,
} EXTRACT_STATUS;

void print_error(const unsigned char message[]);
void print_warning(const unsigned char message[]);
void print_notification(const unsigned char message[]);
void print_information(const unsigned char message[]);

//! MOVE:
void handler_extract_status(EXTRACT_STATUS extract_status);
#endif //__LOGGER__