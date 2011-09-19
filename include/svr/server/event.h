
#ifndef __SVR_SERVER_EVENT_H
#define __SVR_SERVER_EVENT_H

#include <svr/forward.h>
#include <svr/server/forward.h>

void SVRs_Event_register(SVRs_Client* client, SVR_Message* message);
void SVRs_Event_unregister(SVRs_Client* client, SVR_Message* message);

#endif // #ifndef __SVR_SERVER_EVENT_H
