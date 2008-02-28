#include "except_macros.h"

/* very lazy */
0

/* these are common exceptions, like dns servers */

/*
||op0(op2(202,106,0,20))
||op0(op2(202,106,196,115))
*/

/* some weird things they came up with */

||op0(op2(1,1,1,1))
/* apparently this is invalid */
/* ||op0(op2(2,2,2,2)) */

/* is this valid? */
||op1(op2(211,167,0,0), op2(255,255,0,0))

/* i got these from the server's login ack packet */
||op1(op2(211,147,9,0), op2(255,255,255,0))
||op1(op2(211,167,9,0), op2(255,255,255,0))

