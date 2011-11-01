/**
 * \file
 * \brief Message logging
 */

#include "svr.h"

#include <ctype.h>

/** Minimum level at which to log messages */
static short min_log_level = NORMAL;

/** String names for log levels */
static char* level_names[] = {"DEBUG",
                              "INFO",
                              "NORMAL",
                              "WARNING",
                              "ERROR",
                              "CRITICAL"};

/**
 * \brief Set the logging threshold
 *
 * Set the threshold for logging to the given level
 *
 * \param level The minimum level to log at
 */
void SVR_Logging_setThreshold(short level) {
    min_log_level = level;
}

/**
 * \brief Get the string representation of a log level
 *
 * Get the string representation of a log level
 *
 * \param log_level The log level as given above
 * \return A string representation of the log level
 */
char* SVR_Logging_getLevelName(short log_level) {
    return level_names[log_level];
}

/**
 * \brief Get the numerical representation of a log level
 *
 * Get the numerical representation of a log level from the textual
 * representation. This check is case insensitive.
 *
 * \param log_level The log level
 * \return The numerical equivalent or -1 if unknown
 */
short SVR_Logging_getLevelFromName(const char* log_level) {
    short level;
    char* level_copy = strdup(log_level);

    /* Convert to all uppercase */
    for(int i = 0; level_copy[i] != '\0'; i++) {
        level_copy[i] = toupper(level_copy[i]);
    }

    /* Scan levels */
    for(level = SVR_DEBUG; level <= SVR_CRITICAL; level++) {
        if(strcmp(SVR_Logging_getLevelName(level), level_copy) == 0) {
            free(level_copy);
            return level;
        }
    }

    free(level_copy);
    return -1;
}

/**
 * \brief Log a message
 *
 * Log a message. If initialization is complete, the message will be logged
 * through a hub server and may also be printed to standard output depending on
 * the value of the "LogReplicateStdout" variable.
 *
 * \param log_level One of the log levels specified above
 * \param msg The message to log
 */
void SVR_log(short level, char* message) {
    /* Only log messages with a log level at least as high as min_debug_level */
    if(level >= min_log_level) {
        fprintf(stderr, "[%s][SVR] %s\n", level_names[level], message);
    }
}
