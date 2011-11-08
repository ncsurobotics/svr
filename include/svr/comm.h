
#ifndef __SVR_COMM_H
#define __SVR_COMM_H

int SVR_Comm_init(const char* server_address);
void* SVR_Comm_sendMessage(SVR_Message* message, bool is_request);
int SVR_Comm_parseResponse(SVR_Message* response);

#endif // #ifndef __SVR_COMM_H
