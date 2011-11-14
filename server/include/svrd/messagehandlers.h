
#ifndef __SVR_SERVER_MESSAGE_HANDLERS_H
#define __SVR_SERVER_MESSAGE_HANDLERS_H

void SVRD_Stream_rOpen(SVRD_Client* client, SVR_Message* message);
void SVRD_Stream_rClose(SVRD_Client* client, SVR_Message* message);
void SVRD_Stream_rGetInfo(SVRD_Client* client, SVR_Message* message);
void SVRD_Stream_rPause(SVRD_Client* client, SVR_Message* message);
void SVRD_Stream_rUnpause(SVRD_Client* client, SVR_Message* message);
void SVRD_Stream_rResize(SVRD_Client* client, SVR_Message* message);
void SVRD_Stream_rSetChannels(SVRD_Client* client, SVR_Message* message);
void SVRD_Stream_rAttachSource(SVRD_Client* client, SVR_Message* message);
void SVRD_Stream_rSetEncoding(SVRD_Client* client, SVR_Message* message);
void SVRD_Stream_rSetPriority(SVRD_Client* client, SVR_Message* message);
void SVRD_Stream_rSetDropRate(SVRD_Client* client, SVR_Message* message);

void SVRD_Source_rOpen(SVRD_Client* client, SVR_Message* message);
void SVRD_Source_rSetEncoding(SVRD_Client* client, SVR_Message* message);
void SVRD_Source_rSetFrameProperties(SVRD_Client* client, SVR_Message* message);
void SVRD_Source_rClose(SVRD_Client* client, SVR_Message* message);
void SVRD_Source_rData(SVRD_Client* client, SVR_Message* message);
void SVRD_Source_rGetSourcesList(SVRD_Client* client, SVR_Message* message);

void SVRD_Event_rRegister(SVRD_Client* client, SVR_Message* message);
void SVRD_Event_rUnregister(SVRD_Client* client, SVR_Message* message);

#endif // #ifndef __SVR_SERVER_MESSAGE_HANDLERS_H
