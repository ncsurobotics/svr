
#ifndef __SVR_SERVER_MESSAGE_HANDLERS_H
#define __SVR_SERVER_MESSAGE_HANDLERS_H

void SVRs_Stream_rOpen(SVRs_Client* client, SVR_Message* message);
void SVRs_Stream_rClose(SVRs_Client* client, SVR_Message* message);
void SVRs_Stream_rGetProp(SVRs_Client* client, SVR_Message* message);
void SVRs_Stream_rSetProp(SVRs_Client* client, SVR_Message* message);
void SVRs_Stream_rPause(SVRs_Client* client, SVR_Message* message);
void SVRs_Stream_rStart(SVRs_Client* client, SVR_Message* message);
void SVRs_Source_rOpen(SVRs_Client* client, SVR_Message* message);
void SVRs_Source_rClose(SVRs_Client* client, SVR_Message* message);
void SVRs_Source_rGetProp(SVRs_Client* client, SVR_Message* message);
void SVRs_Source_rSetProp(SVRs_Client* client, SVR_Message* message);
void SVRs_Source_rData(SVRs_Client* client, SVR_Message* message);
void SVRs_Event_rRegister(SVRs_Client* client, SVR_Message* message);
void SVRs_Event_rUnregister(SVRs_Client* client, SVR_Message* message);

#endif // #ifndef __SVR_SERVER_MESSAGE_HANDLERS_H
