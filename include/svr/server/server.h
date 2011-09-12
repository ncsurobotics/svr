
#ifndef __SVR_SERVER_SERVER_H
#define __SVR_SERVER_SERVER_H

void SVR_Server_preClose(void);
void SVR_Server_close(void);
void SVR_Server_mainLoop(void);

#define MAX_CLIENTS 128

#endif // #ifndef __SVR_SERVER_SERVER_H
