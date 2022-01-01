/*
 *  utility.h  
 *
 *  Define return values and other macros used in the program Producer
 *  
 *  Created: 12/11/21
 *  Last edited: 12/11/21
 */

#ifndef UTILITY_H
#define UTILITY_H

#define MIN_ARG_NEEDED 2

// return values
enum RET_VALUES
{
    SUCCESS,
    INVALID_ARG,
    PORT_OPEN_FAIL,
    REGISTER_FAIL,
    NO_CONNECTION,
    NO_PLAY,
    SEND_FAIL
};

// input format
enum ARG_FORMAT
{
    PROGRAM_NAME,
    PORT_NUM
};

#endif  // #define UTILITY_H