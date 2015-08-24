#ifndef MINISTRY_RUN_H
#define MINISTRY_RUN_H

// run flags

#define RUN_DAEMON					0x00000001
#define RUN_STATS					0x00000002
#define RUN_DEBUG					0x00000004

#define RUN_LOOP					0x00000010
#define RUN_SHUTDOWN				0x00000020
#define RUN_LOOP_CTL_MASK			0x00000030

#define RUN_SHUT_STATS				0x00010000
#define RUN_SHUT_MASK				0x000f0000

#endif
