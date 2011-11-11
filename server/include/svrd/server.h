
#ifndef __SVR_SERVER_SERVER_H
#define __SVR_SERVER_SERVER_H

void SVRD_Server_preClose(void);
void SVRD_Server_close(void);
void SVRD_Server_mainLoop(const char* bind_address);

#define MAX_CLIENTS 128

#endif // #ifndef __SVR_SERVER_SERVER_H
