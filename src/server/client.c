
#include <seawolf.h>

#include "svr.h"
#include "svr/server/client.h"

static void* SVR_removeMarkedClosedClients(void* __unused);
static void* SVR_Client_worker(void* _client);

/* List of active clients */
static List* clients = NULL;
static Queue* closed_clients = NULL;

/* Global clients lock */
static pthread_mutex_t global_clients_lock = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t remove_client_lock = PTHREAD_MUTEX_INITIALIZER;

/** Task handle of thread that destroys clients */
static pthread_t close_clients_thread;

void SVR_Client_init(void) {
    clients = List_new();
    closed_clients = Queue_new();
    pthread_create(&close_clients_thread, NULL, &SVR_removeMarkedClosedClients, NULL);
}

void SVR_Client_close(void) {
    SVR_Client* client;

    if(clients) {
        /* Kick all still attached clients */
        SVR_acquireGlobalClientsLock();
        for(int i = 0; (client = List_get(clients, i)) != NULL; i++) {
            SVR_Client_kick(client, "SVR closing");
        }
        SVR_releaseGlobalClientsLock();

        /* Ensure removeMarkedClosedClients stops blocking to exit */
        Queue_append(closed_clients, NULL);
        
        /* Wait for all client threads to complete */
        SVR_joinAllClientThreads();
        
        /* Free the clients lists */
        List_destroy(clients);
        Queue_destroy(closed_clients);
    }
}

SVR_Client* SVR_Client_new(int socket) {
    SVR_Client* client = malloc(sizeof(SVR_Client));

    client->socket = socket;
    client->streams = Dictionary_new();
    client->sources = Dictionary_new();

    return client;
}

void SVR_addClient(int socket) {
    SVR_Client* client = SVR_Client_new(socket);

    SVR_acquireGlobalClientsLock();
    List_append(clients, client);
    SVR_releaseGlobalClientsLock();

    pthread_create(&client->thread, NULL, &SVR_Client_worker, client);
}

/**
 * \brief Remove clients previously marked as closed
 *
 * \return Always returns NULL
 */
static void* SVR_removeMarkedClosedClients(void* __unused) {
    SVR_Client* client;

    /* NULL pushed to the queue after all clients have been disconnected
       during shutdown */
    while((client = Queue_pop(closed_clients, true)) != NULL) {
        /* Immediately close the socket. The client can not longer generate requests */
        shutdown(client->socket, SHUT_RDWR);
        close(client->socket);
        
        /* Remove client from clients list */
        SVR_acquireGlobalClientsLock();
        List_remove(clients, List_indexOf(clients, client));
        SVR_releaseGlobalClientsLock();

        /* Wait for the client thread to terminate */
        pthread_join(client->thread, NULL);
        
        /* With the client thread dead and the client removed from the client
           list the client is inaccessible at this point. Only threads which
           acquired a reference before the client was closed may still have
           access to it */
        
        /* Wait for any thread which holds an in_use read lock on the client to
           complete. No more threads could possibly try to acquire a read lock
           so we're just waiting for existing locks to expire */
        //pthread_rwlock_wrlock(&client->in_use);

        /* The client is completely removed and unused. Safe to free backing memory */
        free(client);
    }

    return NULL;
}

/**
 * \brief Kick a client 
 */
void SVR_Client_kick(SVR_Client* client, const char* reason) {
    SVR_Message* message = SVR_Message_new(2);

    message->components[0] = SVR_Arena_strdup(message->alloc, "KICK");
    message->components[1] = SVR_Arena_strdup(message->alloc, reason);

    SVR_Net_sendMessage(client->socket, message);
    SVR_Message_release(message);

    SVR_Client_markForClosing(client);
}


/**
 * \brief Mark a client as closed
 *
 * Mark a client as closed. It's resources will be released on the next call to
 * SVR_Client_removeMarkedClosedClients()
 *
 * \param client Mark the given client as closed
 */
void SVR_Client_markForClosing(SVR_Client* client) {
    pthread_mutex_lock(&remove_client_lock);
    if(client->state != SVR_CLOSED) {
        client->state = SVR_CLOSED;
        Queue_append(closed_clients, client);
    }
    pthread_mutex_unlock(&remove_client_lock);
}

/**
 * \brief Wait for all client threads to die
 *
 * Wait for all client threads to shutdown and be destroyed
 */
void SVR_joinAllClientThreads(void) {
    pthread_join(close_clients_thread, NULL);
}

/**
 * \brief Get clients
 *
 * Get a list of all clients. Access to the list should be proteced by calls to
 * SVR_Client_acquireGlobalClientsLock and SVR_Client_releaseGlobalClientsLock
 *
 * \return The list of clients
 */
List* SVR_getAllClients(void) {
    return clients;
}

/**
 * \brief Acquire the clients list lock
 *
 * Acquire a global lock on the clients list. This lock should only be held for
 * a short amount of time if necessary.
 */
void SVR_acquireGlobalClientsLock(void) {
    pthread_mutex_lock(&global_clients_lock);
}

/**
 * \brief Release the clients list lock
 *
 * Release the global lock on the clients list. This lock should only be held
 * for a short amount of time if necessary.
 */
void SVR_releaseGlobalClientsLock(void) {
    pthread_mutex_unlock(&global_clients_lock);
}

/**
 * \brief Client connection thread
 *
 * Handle requests from a single client until the client closes
 *
 * \param _client Pointer to a SVR_Client structure
 * \return Always returns NULL
 */
static void* SVR_Client_worker(void* _client) {
    SVR_Client* client = (SVR_Client*) _client;
    SVR_Message* message;
 
    while(client->state != SVR_CLOSED) {
        /* Read message from the client  */
        message = SVR_Net_receiveMessage(client->socket);

        if(message == NULL) {
            SVR_Client_markForClosing(client);
            break;
        }

        /* Process message */
        //SVR_Process_process(client, message);

        /* Destroy client message */
        SVR_Message_release(message);
    }

    return NULL;
}
