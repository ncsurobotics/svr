
#ifndef __SVR_SERVER_EVENT_H
#define __SVR_SERVER_EVENT_H

#include <svr/forward.h>
#include <svrd/forward.h>

void SVRD_Event_register(SVRD_Client* client, SVR_Message* message);
void SVRD_Event_unregister(SVRD_Client* client, SVR_Message* message);

#endif // #ifndef __SVR_SERVER_EVENT_H
