/**
 * \file
 */

#ifndef __SVR_LOGGING_INCLUDE_H
#define __SVR_LOGGING_INCLUDE_H

/**
 * Debug log-level
 */
#define SVR_DEBUG     0x00

/**
 * Info log-level
 */
#define SVR_INFO      0x01

/**
 * Normal log-level
 */
#define SVR_NORMAL    0x02

/**
 * Warning log-level
 */
#define SVR_WARNING   0x03

/**
 * Error log-level
 */
#define SVR_ERROR     0x04

/**
 * Critical log-level
 */
#define SVR_CRITICAL  0x05

void SVR_Logging_setThreshold(short level);
char* SVR_Logging_getLevelName(short log_level);
short SVR_Logging_getLevelFromName(const char* log_level);
void SVR_log(short level, char* message);

#endif // #ifndef __SVR_LOGGING_INCLUDE_H
