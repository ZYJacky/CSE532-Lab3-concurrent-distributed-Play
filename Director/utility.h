/*
 *   utility.h   CSE532S Lab2 Version
 *
 *   Define return values, cmd line arguments format, and other macros for the program Play
 *
 *   Created: 9/16/21
 *   Last edited: 11/8/21
 */

#ifndef UTILITY_H
#define UTILITY_H

#define MIN_ARGS_NEEDED 5  // need at least 4 argument including program name

enum RET_VALUES
{
    SUCCESS,
    INVALID_ARG,
    CONNECT_FAIL,
    REGISTER_FAIL,
    NO_FILE,
    NO_PLAY_NAME,
    NO_VALID_ROLE,
    UNKNOWN_ERROR,
    WIRED_ON_STAGE,
    WIRED_FRAG_COUNT
};

enum ARGS_FORMAT
{
    PROGRAM_NAME,
    PORT_NUM,
    IP_ADDR,
    MIN_THREAD,
    FILE_START
};

#endif  // #define UTILITY_H