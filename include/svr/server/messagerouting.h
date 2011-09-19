
#ifndef __SVR_SERVER_MESSAGEROUTING_H
#define __SVR_SERVER_MESSAGEROUTING_H

#include <svr/forward.h>

void SVRs_MessageRouter_init(void);
void SVRs_processMessage(SVRs_Client* client, SVR_Message* message);

#endif // #ifndef __SVR_SERVER_MESSAGEROUTING_H
