
#include <svr.h>

#define RESPONSE_SET_GROW 8

SVR_ResponseSet* SVR_ReponseSet_new(int max_request_id) {
    SVR_ResponseSet* response_set = malloc(sizeof(SVR_ResponseSet));

    response_set->response_pending = calloc(RESPONSE_SET_GROW, sizeof(bool));
    response_set->response = calloc(RESPONSE_SET_GROW, sizeof(void*));
    response_set->max_request_id = max_request_id;
    response_set->next_request_id = 0;
    response_set->set_size = RESPONSE_SET_GROW;
    pthread_cond_init(&response_set->new_response, NULL);
    SVR_LOCKABLE_INIT(response_set);

    return response_set;
}

void SVR_ResponseSet_destroy(SVR_ResponseSet* response_set) {
    free(response_set->response_pending);
    free(response_set->response);
    free(response_set);
}

int SVR_ResponseSet_getRequestId(SVR_ResponseSet* response_set) {
    unsigned int response_id;

    SVR_LOCK(response_set);
    response_id = response_set->next_request_id;
    while(response_set->response_pending[response_id] == true) {
        response_id = (response_id + 1) % response_set->set_size;

        if(response_id == response_set->next_request_id) {
            if(response_set->set_size + RESPONSE_SET_GROW >= response_set->max_request_id) {
                return -1;
            }

            response_set->response_pending = realloc(response_set->response_pending, (response_set->set_size + RESPONSE_SET_GROW) * sizeof(bool));
            response_set->response = realloc(response_set->response, (response_set->set_size + RESPONSE_SET_GROW) * sizeof(void*));
            memset(response_set->response_pending + response_set->set_size, 0, RESPONSE_SET_GROW * sizeof(bool));

            response_id = response_set->set_size;
            response_set->set_size += RESPONSE_SET_GROW;
        }
    }

    response_set->next_request_id = (response_id + 1) % response_set->set_size;
    response_set->response_pending[response_id] = true;
    response_set->response[response_id] = NULL;

    SVR_UNLOCK(response_set);

    return response_id;
}

void* SVR_ResponseSet_getResponse(SVR_ResponseSet* response_set, int response_id) {
    void* response;

    SVR_LOCK(response_set);
    while(response_set->response[response_id] == NULL) {
        SVR_LOCK_WAIT(response_set, &response_set->new_response);
    }

    response = response_set->response[response_id];
    response_set->response_pending[response_id] = false;

    SVR_UNLOCK(response_set);

    return response;
}

int SVR_ResponseSet_setResponse(SVR_ResponseSet* response_set, int response_id, void* response) {
    int success;

    SVR_LOCK(response_set);
    if(response_set->response_pending[response_id]) {
        response_set->response[response_id] = response;
        pthread_cond_broadcast(&response_set->new_response);
    } else {
        success = -1;
    }
    SVR_UNLOCK(response_set);

    return success;
}
