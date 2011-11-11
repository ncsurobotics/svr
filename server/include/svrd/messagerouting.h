
#ifndef __SVR_SERVER_MESSAGEROUTING_H
#define __SVR_SERVER_MESSAGEROUTING_H

#include <svr/forward.h>

void SVRD_MessageRouter_init(void);
void SVRD_processMessage(SVRD_Client* client, SVR_Message* message);

#endif // #ifndef __SVR_SERVER_MESSAGEROUTING_H
