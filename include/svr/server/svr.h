
#ifndef __SVR_SERVER_SVR_H
#define __SVR_SERVER_SVR_H

#include "svr.h"

#include "svr/server/forward.h"
#include "svr/server/client.h"
#include "svr/server/reencoder.h"
#include "svr/server/server.h"
#include "svr/server/source.h"
#include "svr/server/stream.h"
#include "svr/server/event.h"
#include "svr/server/messagerouting.h"

void SVRs_exit(void);
void SVRs_exitError(void);

#endif // #ifndef __SVR_SERVER_SVR_H
