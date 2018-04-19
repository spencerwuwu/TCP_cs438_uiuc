#pragma once

#include "tcp.h"
#include <string.h>


/* Transition Diagram of 
 * Receiver TCP State
 */
enum Receiver_State {
    CLOSED_R,
    SYN_SENT_R,
    ESTABLISHED_R,
    CLOSE_WAIT_R,
    LAST_ACK_R,
};
