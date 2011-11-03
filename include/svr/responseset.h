/*
 * responseset.h
 *
 *  Created on: Nov 2, 2011
 *      Author: christopher
 */

#ifndef __SVR_RESPONSESET_H
#define __SVR_RESPONSESET_H

#include <svr/forward.h>

struct SVR_ResponseSet_s {
    bool* response_pending;
    void** response;
    int max_request_id;
    int next_request_id;
    int set_size;

    pthread_cond_t new_response;
    SVR_LOCKABLE;
};

SVR_ResponseSet* SVR_ReponseSet_new(int max_request_id);
void SVR_ResponseSet_destroy(SVR_ResponseSet* response_set);
int SVR_ResponseSet_getRequestId(SVR_ResponseSet* response_set);
void* SVR_ResponseSet_getResponse(SVR_ResponseSet* response_set, int response_id);
int SVR_ResponseSet_setResponse(SVR_ResponseSet* response_set, int response_id, void* response);

#endif // #ifndef __SVR_RESPONSESET_H
