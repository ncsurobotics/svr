
#ifndef __SVR_NET_H
#define __SVR_NET_H

int SVR_Net_sendPackedMessage(int socket, SVR_PackedMessage* packed_message);
int SVR_Net_sendMessage(int socket, SVR_Message* message);
SVR_Message* SVR_Net_receiveMessage(int socket);
int SVR_Net_receivePayload(int socket, SVR_Message* message);

#endif // #ifndef __SVR_NET_H
