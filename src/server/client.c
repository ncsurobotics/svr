
#include <seawolf.h>

#include "svr.h"
#include "svr/server/svr.h"

static void* SVRs_Client_worker(void* _client);
static void SVRs_Client_cleanup(void* _client);

/* List of active clients */
static List* clients = NULL;

/* Global clients lock */
static pthread_mutex_t global_clients_lock = PTHREAD_MUTEX_INITIALIZER;

static int client_thread_count = 0;
static pthread_mutex_t client_thread_count_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t client_thread_count_zero = PTHREAD_COND_INITIALIZER;

void SVRs_Client_init(void) {
    clients = List_new();
}

void SVRs_Client_close(void) {
    SVRs_Client* client;

    if(clients) {
        /* Kick all still attached clients */
        SVRs_acquireGlobalClientsLock();
        for(int i = 0; (client = List_get(clients, i)) != NULL; i++) {
            SVRs_Client_kick(client, "SVR closing");
        }
        SVRs_releaseGlobalClientsLock();

        /* Wait for all client threads to complete */
        SVRs_joinAllClientThreads();
        
        /* Free the clients lists */
        List_destroy(clients);
    }
}

SVRs_Client* SVRs_Client_new(int socket) {
    SVRs_Client* client = malloc(sizeof(SVRs_Client));

    client->socket = socket;
    client->streams = Dictionary_new();
    client->sources = Dictionary_new();
    client->state = SVR_CONNECTED;

    SVR_REFCOUNTED_INIT(client, SVRs_Client_cleanup);
    SVR_LOCKABLE_INIT(client);

    pthread_mutex_lock(&client_thread_count_lock);
    client_thread_count++;
    pthread_mutex_unlock(&client_thread_count_lock);

    return client;
}

void SVRs_Client_provideSource(SVRs_Client* client, SVRs_Source* source) {
    Dictionary_set(client->sources, source->name, source);
}

void SVRs_Client_openStream(SVRs_Client* client, const char* stream_name) {
    SVRs_Stream* stream;

    SVR_LOCK(client);
    if(client->state == SVR_CONNECTED) {
        stream = SVRs_Stream_new(stream_name);
        Dictionary_set(client->streams, stream_name, stream);
        SVRs_Stream_setClient(stream, client);
    }
    SVR_UNLOCK(client);
}

void SVRs_Client_closeStream(SVRs_Client* client, const char* stream_name) {
    SVRs_Stream* stream;

    SVR_LOCK(client);
    stream = Dictionary_get(client->streams, stream_name);
    if(stream == NULL) {
        return;
    }
    Dictionary_remove(client->streams, stream_name);
    SVR_UNLOCK(client);

    SVRs_Stream_pause(stream);
    SVRs_Stream_destroy(stream);
}

SVRs_Stream* SVRs_Client_getStream(SVRs_Client* client, const char* stream_name) {
    return Dictionary_get(client->streams, stream_name);
}

static void SVRs_Client_cleanup(void* _client) {
    SVRs_Client* client = (SVRs_Client*) _client;

    SVR_log(SVR_DEBUG, "Cleaning up client");

    pthread_join(client->thread, NULL);

    Dictionary_destroy(client->sources);
    Dictionary_destroy(client->streams);
    free(client);

    pthread_mutex_lock(&client_thread_count_lock);
    client_thread_count--;
    if(client_thread_count == 0) {
        pthread_cond_broadcast(&client_thread_count_zero);
    }
    pthread_mutex_unlock(&client_thread_count_lock);
}

void SVRs_addClient(int socket) {
    SVRs_Client* client = SVRs_Client_new(socket);

    SVRs_acquireGlobalClientsLock();
    List_append(clients, client);
    SVRs_releaseGlobalClientsLock();

    pthread_create(&client->thread, NULL, &SVRs_Client_worker, client);
}

/**
 * \brief Mark a client as closed
 *
 * Mark a client as closed. It's resources will be released on the next call to
 * SVRs_Client_removeMarkedClosedClients()
 *
 * \param client Mark the given client as closed
 */
void SVRs_Client_markForClosing(SVRs_Client* client) {
    List* streams;
    char* stream_name;

    SVR_LOCK(client);
    if(client->state != SVR_CLOSED) {
        client->state = SVR_CLOSED;
        SVR_UNLOCK(client);

        /* Immediately close the socket. The client can not longer generate requests */
        shutdown(client->socket, SHUT_RDWR);
        close(client->socket);
        
        /* Remove client from clients list */
        SVRs_acquireGlobalClientsLock();
        List_remove(clients, List_indexOf(clients, client));
        SVRs_releaseGlobalClientsLock();

        /* Destroy all streams */
        streams = Dictionary_getKeys(client->streams);
        for(int i = 0; (stream_name = List_get(streams, i)) != NULL; i++) {
            SVRs_Client_closeStream(client, stream_name);
        }
        List_destroy(streams);
    } else {
        SVR_UNLOCK(client);
    }
}

void SVRs_Client_reply(SVRs_Client* client, SVR_Message* request, SVR_Message* response) {
    response->request_id = request->request_id;
    SVRs_Client_sendMessage(client, response);
}

void SVRs_Client_replyError(SVRs_Client* client, SVR_Message* request, int error_code) {
    SVR_Message* message = SVR_Message_new(2);

    message->components[0] = SVR_Arena_strdup(message->alloc, "SVR.response");
    message->components[1] = SVR_Arena_sprintf(message->alloc, "%d", error_code);
    SVRs_Client_reply(client, request, message);
    SVR_Message_release(message);
}

void SVRs_Client_replySuccess(SVRs_Client* client, SVR_Message* request) {
    SVRs_Client_replyError(client, request, SVR_SUCCESS);
}

/**
 * \brief Kick a client 
 */
void SVRs_Client_kick(SVRs_Client* client, const char* reason) {
    SVR_Message* message = SVR_Message_new(2);

    message->components[0] = SVR_Arena_strdup(message->alloc, "SVR.kick");
    message->components[1] = SVR_Arena_strdup(message->alloc, reason);

    SVRs_Client_sendMessage(client, message);
    SVR_Message_release(message);

    SVRs_Client_markForClosing(client);
}

/**
 * \brief Wait for all client threads to die
 *
 * Wait for all client threads to shutdown and be destroyed
 */
void SVRs_joinAllClientThreads(void) {
    pthread_mutex_lock(&client_thread_count_lock);
    while(client_thread_count > 0) {
        pthread_cond_wait(&client_thread_count_zero, &client_thread_count_lock);
    }
    pthread_mutex_unlock(&client_thread_count_lock);
}

/**
 * \brief Get clients
 *
 * Get a list of all clients. Access to the list should be proteced by calls to
 * SVRs_Client_acquireGlobalClientsLock and SVRs_Client_releaseGlobalClientsLock
 *
 * \return The list of clients
 */
List* SVRs_getAllClients(void) {
    return clients;
}

/**
 * \brief Acquire the clients list lock
 *
 * Acquire a global lock on the clients list. This lock should only be held for
 * a short amount of time if necessary.
 */
void SVRs_acquireGlobalClientsLock(void) {
    pthread_mutex_lock(&global_clients_lock);
}

/**
 * \brief Release the clients list lock
 *
 * Release the global lock on the clients list. This lock should only be held
 * for a short amount of time if necessary.
 */
void SVRs_releaseGlobalClientsLock(void) {
    pthread_mutex_unlock(&global_clients_lock);
}

int SVRs_Client_sendMessage(SVRs_Client* client, SVR_Message* message) {
    int n;

    SVR_LOCK(client);
    if(client->state != SVR_CLOSED) {
        n = SVR_Net_sendMessage(client->socket, message);
    }
    SVR_UNLOCK(client);

    return n;
}

/**
 * \brief Client connection thread
 *
 * Handle requests from a single client until the client closes
 *
 * \param _client Pointer to a SVRs_Client structure
 * \return Always returns NULL
 */
static void* SVRs_Client_worker(void* _client) {
    SVRs_Client* client = (SVRs_Client*) _client;
    SVR_Message* message;
 
    while(client->state != SVR_CLOSED) {
        /* Read message from the client  */
        message = SVR_Net_receiveMessage(client->socket);

        if(message == NULL) {
            SVR_log(SVR_WARNING, "Lost client connection");
            SVRs_Client_markForClosing(client);
            break;
        }

        /* Process message */
        SVRs_processMessage(client, message);

        /* Destroy client message */
        SVR_Message_release(message);
    }

    SVR_UNREF(client);

    return NULL;
}
