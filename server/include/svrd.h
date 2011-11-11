
#ifndef __SVR_SERVER_SVR_H
#define __SVR_SERVER_SVR_H

#include "svr.h"

#include "svrd/forward.h"
#include "svrd/client.h"
#include "svrd/server.h"
#include "svrd/source.h"
#include "svrd/stream.h"
#include "svrd/event.h"
#include "svrd/messagerouting.h"
#include "svrd/messagehandlers.h"

void SVRD_exit(void);
void SVRD_exitError(void);

#endif // #ifndef __SVR_SERVER_SVR_H
