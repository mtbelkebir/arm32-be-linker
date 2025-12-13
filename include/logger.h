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

void print_error(const unsigned char message[]);
void print_warning(const unsigned char message[]);
void print_notification(const unsigned char message[]);
void print_information(const unsigned char message[]);
#endif //__LOGGER__