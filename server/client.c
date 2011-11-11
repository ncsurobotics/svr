
#include <seawolf.h>

#include "svr.h"
#include "svrd.h"

static void* SVRD_Client_worker(void* _client);
static void SVRD_Client_cleanup(void* _client);

/* List of active clients */
static List* clients = NULL;

/* Global clients lock */
static pthread_mutex_t global_clients_lock = PTHREAD_MUTEX_INITIALIZER;

static int client_thread_count = 0;
static pthread_mutex_t client_thread_count_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t client_thread_count_zero = PTHREAD_COND_INITIALIZER;

void SVRD_Client_init(void) {
    clients = List_new();
}

void SVRD_Client_close(void) {
    SVRD_Client* client;

    if(clients) {
        /* Kick all still attached clients */
        SVRD_acquireGlobalClientsLock();
        for(int i = 0; (client = List_get(clients, i)) != NULL; i++) {
            SVRD_Client_kick(client, "SVR closing");
        }
        SVRD_releaseGlobalClientsLock();

        /* Wait for all client threads to complete */
        SVRD_joinAllClientThreads();
        
        /* Free the clients lists */
        List_destroy(clients);
    }
}

SVRD_Client* SVRD_Client_new(int socket) {
    SVRD_Client* client = malloc(sizeof(SVRD_Client));

    client->socket = socket;
    client->streams = Dictionary_new();
    client->sources = Dictionary_new();
    client->state = SVR_CONNECTED;
    client->payload_buffer = NULL;
    client->payload_buffer_size = 0;

    SVR_REFCOUNTED_INIT(client, SVRD_Client_cleanup);
    SVR_LOCKABLE_INIT(client);

    pthread_mutex_lock(&client_thread_count_lock);
    client_thread_count++;
    pthread_mutex_unlock(&client_thread_count_lock);

    return client;
}

void SVRD_Client_provideSource(SVRD_Client* client, SVRD_Source* source) {
    Dictionary_set(client->sources, source->name, source);
}

void SVRD_Client_unprovideSource(SVRD_Client* client, SVRD_Source* source) {
    Dictionary_remove(client->sources, source->name);
}

void SVRD_Client_openStream(SVRD_Client* client, const char* stream_name) {
    SVRD_Stream* stream;

    SVR_LOCK(client);
    if(client->state == SVR_CONNECTED) {
        stream = SVRD_Stream_new(stream_name);
        Dictionary_set(client->streams, stream_name, stream);
        SVRD_Stream_setClient(stream, client);
    }
    SVR_UNLOCK(client);
}

void SVRD_Client_closeStream(SVRD_Client* client, const char* stream_name) {
    SVRD_Stream* stream;

    SVR_LOCK(client);
    stream = Dictionary_get(client->streams, stream_name);
    if(stream == NULL) {
        return;
    }
    Dictionary_remove(client->streams, stream_name);
    SVR_UNLOCK(client);

    SVRD_Stream_pause(stream);
    SVRD_Stream_destroy(stream);
}

SVRD_Stream* SVRD_Client_getStream(SVRD_Client* client, const char* stream_name) {
    return Dictionary_get(client->streams, stream_name);
}

SVRD_Source* SVRD_Client_getSource(SVRD_Client* client, const char* source_name) {
    return Dictionary_get(client->sources, source_name);
}

static void SVRD_Client_cleanup(void* _client) {
    SVRD_Client* client = (SVRD_Client*) _client;

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

void SVRD_addClient(int socket) {
    SVRD_Client* client = SVRD_Client_new(socket);

    SVRD_acquireGlobalClientsLock();
    List_append(clients, client);
    SVRD_releaseGlobalClientsLock();

    pthread_create(&client->thread, NULL, &SVRD_Client_worker, client);
}

/**
 * \brief Mark a client as closed
 *
 * Mark a client as closed. It's resources will be released on the next call to
 * SVRD_Client_removeMarkedClosedClients()
 *
 * \param client Mark the given client as closed
 */
void SVRD_Client_markForClosing(SVRD_Client* client) {
    SVRD_Source* source;
    List* streams;
    List* sources;
    char* stream_name;
    char* source_name;

    SVR_LOCK(client);
    if(client->state != SVR_CLOSED) {
        client->state = SVR_CLOSED;
        SVR_UNLOCK(client);

        /* Immediately close the socket. The client can not longer generate requests */
        shutdown(client->socket, SHUT_RDWR);
        close(client->socket);
        
        /* Remove client from clients list */
        SVRD_acquireGlobalClientsLock();
        List_remove(clients, List_indexOf(clients, client));
        SVRD_releaseGlobalClientsLock();

        /* Destroy all streams */
        streams = Dictionary_getKeys(client->streams);
        for(int i = 0; (stream_name = List_get(streams, i)) != NULL; i++) {
            SVRD_Client_closeStream(client, stream_name);
        }
        List_destroy(streams);

        /* Destroy all sources */
        sources = Dictionary_getKeys(client->sources);
        for(int i = 0; (source_name = List_get(sources, i)) != NULL; i++) {
            source = Dictionary_get(client->sources, source_name);
            SVRD_Source_destroy(source);
        }
        List_destroy(sources);
    } else {
        SVR_UNLOCK(client);
    }
}

void SVRD_Client_reply(SVRD_Client* client, SVR_Message* request, SVR_Message* response) {
    response->request_id = request->request_id;
    SVRD_Client_sendMessage(client, response);
}

void SVRD_Client_replyCode(SVRD_Client* client, SVR_Message* request, int error_code) {
    SVR_Message* message = SVR_Message_new(2);

    message->components[0] = SVR_Arena_strdup(message->alloc, "SVR.response");
    message->components[1] = SVR_Arena_sprintf(message->alloc, "%d", error_code);
    SVRD_Client_reply(client, request, message);
    SVR_Message_release(message);
}

/**
 * \brief Kick a client 
 */
void SVRD_Client_kick(SVRD_Client* client, const char* reason) {
    SVR_Message* message = SVR_Message_new(2);

    message->components[0] = SVR_Arena_strdup(message->alloc, "SVR.kick");
    message->components[1] = SVR_Arena_strdup(message->alloc, reason);

    SVRD_Client_sendMessage(client, message);
    SVR_Message_release(message);

    SVRD_Client_markForClosing(client);
}

/**
 * \brief Wait for all client threads to die
 *
 * Wait for all client threads to shutdown and be destroyed
 */
void SVRD_joinAllClientThreads(void) {
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
 * SVRD_Client_acquireGlobalClientsLock and SVRD_Client_releaseGlobalClientsLock
 *
 * \return The list of clients
 */
List* SVRD_getAllClients(void) {
    return clients;
}

/**
 * \brief Acquire the clients list lock
 *
 * Acquire a global lock on the clients list. This lock should only be held for
 * a short amount of time if necessary.
 */
void SVRD_acquireGlobalClientsLock(void) {
    pthread_mutex_lock(&global_clients_lock);
}

/**
 * \brief Release the clients list lock
 *
 * Release the global lock on the clients list. This lock should only be held
 * for a short amount of time if necessary.
 */
void SVRD_releaseGlobalClientsLock(void) {
    pthread_mutex_unlock(&global_clients_lock);
}

int SVRD_Client_sendMessage(SVRD_Client* client, SVR_Message* message) {
    int n = -1;

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
 * \param _client Pointer to a SVRD_Client structure
 * \return Always returns NULL
 */
static void* SVRD_Client_worker(void* _client) {
    SVRD_Client* client = (SVRD_Client*) _client;
    SVR_Message* message;
 
    while(client->state != SVR_CLOSED) {
        /* Read message from the client  */
        message = SVR_Net_receiveMessage(client->socket);

        if(message == NULL) {
            SVR_log(SVR_WARNING, "Lost client connection");
            SVRD_Client_markForClosing(client);
            break;
        }

        /* Receive payload */
        if(message->payload_size > 0) {
            if(message->payload_size > client->payload_buffer_size) {
                client->payload_buffer = realloc(client->payload_buffer, message->payload_size);
                message->payload = client->payload_buffer;

                SVR_Net_receivePayload(client->socket, message);
            }
        }

        /* Process message */
        SVRD_processMessage(client, message);

        /* Destroy client message */
        SVR_Message_release(message);
    }

    SVR_UNREF(client);

    return NULL;
}
