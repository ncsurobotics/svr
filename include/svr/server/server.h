
#ifndef __SVR_SERVER_SERVER_H
#define __SVR_SERVER_SERVER_H

void SVRs_Server_preClose(void);
void SVRs_Server_close(void);
void SVRs_Server_mainLoop(void);

#define MAX_CLIENTS 128

#endif // #ifndef __SVR_SERVER_SERVER_H
