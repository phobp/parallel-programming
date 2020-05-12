/**
 * dht.c
 *
 * CS 470 Project 4
 *
 * Implementation for distributed hash table (DHT).
 *
 * Name: Brendan Pho, Emma Magner 
 */

#include <mpi.h>
#include <pthread.h>
#include "dht.h"

// Message tags for the server
#define PUT 1
#define GET 2
#define CONFIRM 3
#define SIZE 5
#define DESTROY 6

// Message tags for the client
#define RETURN_VALUE 10
#define TOTAL_SIZE 6

// Proces ID, process rank, number of processes
static int pid;
int rank;
int nprocs;

// Condition for the while loop regarding locks and unlocks
int approved;

// Pthread variables/mutexes
pthread_mutex_t approve_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t approve_cond = PTHREAD_COND_INITIALIZER;
pthread_t threadid;


// Represents a message being sent between client and server
struct dht_msg
{
    char key[MAX_KEYLEN];
    long value;
};


// Creates a zeroed out "blank" message
struct dht_msg blank()
{
    struct dht_msg msg;
    memset(&msg, 0, sizeof(struct dht_msg));
    return msg;
}


// Receives a message with the given tag from any source
void recv_tag(struct dht_msg *msg, int tag, MPI_Status *recv_status)
{
    MPI_Recv(msg, sizeof(struct dht_msg), MPI_BYTE, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, recv_status);
}

/*
Processes all messages with server tags and by using the source and tag, the server will choose to accept the message in MPI_Recv
*/
void *server(void *ptr)
{
    // This loop will continue until it receives a message DESTROY
    while(1) {
        MPI_Status status;
        MPI_Status recv_status;
        struct dht_msg msg = blank();
        
        // Looks at the messages for the status, source, and tag
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        
        switch (status.MPI_TAG)
        {
            case PUT:
                recv_tag(&msg, PUT, &recv_status);
                MPI_Request req;
                local_put(msg.key, msg.value);
            
            struct dht_msg ret_msg = blank();
            ret_msg.value = CONFIRM; 
            
            // Stops a bug that blocked when sending to own process
            if (status.MPI_SOURCE != rank)
            {   
            MPI_Send(&ret_msg, sizeof(struct dht_msg), MPI_BYTE, status.MPI_SOURCE, CONFIRM, MPI_COMM_WORLD);
            }
            else 
            {
                MPI_Isend(&ret_msg, sizeof(struct dht_msg), MPI_BYTE, status.MPI_SOURCE, CONFIRM, MPI_COMM_WORLD, &req);
            }
            break;
            case GET:

                recv_tag(&msg, GET, &recv_status);
                MPI_Request request;

                // Prepares a new message with the value received
                long val = local_get(msg.key);
                struct dht_msg get_value = blank();
                get_value.value = val;

                // Stops a bug that blocked when sending to own process
                if (status.MPI_SOURCE != rank)
                {
                    MPI_Send(&get_value, sizeof(struct dht_msg), MPI_BYTE, status.MPI_SOURCE, RETURN_VALUE, MPI_COMM_WORLD);
                }
                else
                {
                    MPI_Isend(&get_value, sizeof(struct dht_msg), MPI_BYTE, status.MPI_SOURCE, RETURN_VALUE, MPI_COMM_WORLD, &request);
                }
            break;
            case CONFIRM:

                // Receives a message of a approval from PUT and signals the client
                recv_tag(&msg, CONFIRM, &recv_status);
                approved = 1;
                pthread_mutex_unlock(&approve_lock);
                pthread_cond_signal(&approve_cond);
            
                break;
            case SIZE:

                // A request for the size of the hash table is received and all process will give their size through MPI_REDUCE
                // The final result is sent to the client
                recv_tag(&msg, SIZE, &recv_status);
                long localsize = 0;
                localsize = local_size();
                long totalsize = 0;
                MPI_Reduce(&localsize, &totalsize, 1, MPI_LONG, MPI_SUM, recv_status.MPI_SOURCE, MPI_COMM_WORLD);
                if (rank == recv_status.MPI_SOURCE)
                {
                    MPI_Send(&totalsize, 1, MPI_LONG, recv_status.MPI_SOURCE, TOTAL_SIZE, MPI_COMM_WORLD);
                }
            case DESTROY:

                // Message DESTROY is received so loop and server terminates
                recv_tag(&msg, DESTROY, &recv_status);
                pthread_exit(NULL);
                break;
            default:

                break;
        
        } // end switch case
    }
}

/**
 * Given hash function for this project
 */
int hash(const char *name) 
{
  unsigned hash = 5381;
  while (*name != '\0')
  {
    hash = ((hash << 5) + hash) + (unsigned)(*name++);
  }
  return hash % nprocs;
}

int dht_init()
{
    int provided;
    
    MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided);
    if (provided != MPI_THREAD_MULTIPLE) 
    {
      printf("ERROR: Cannot initialize MPI in THREAD_MULTIPLE mode.\n");
      exit(EXIT_FAILURE);
    }
    
    // Set the rank and number of processes
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    // Approval flag where 0 is not approved and 1 is approved
    approved = 0;

    local_init();
    
    // Process ID is the process's rank
    pid = rank;

    // Create server thread
    pthread_create(&threadid, NULL, server, NULL);
    return pid;
}

void put_send(int dest, const char *key, long value)
{
    struct dht_msg msg = blank();

    // Put the new key and value into the blank message
    strcpy(msg.key, key);
    msg.value = value;
    
    // Send message to server 
    MPI_Send(&msg, sizeof(struct dht_msg), MPI_BYTE, dest, PUT, MPI_COMM_WORLD);
}

void dht_put(const char *key, long value)
{
    // Destination for the key and value
    int dest = hash(key);

    // Send the key and value
    put_send(dest, key, value);

    // Lock until the approved flag is 1
    pthread_mutex_lock(&approve_lock);
    while (!approved)
    {
        pthread_cond_wait(&approve_cond, &approve_lock);
    }

    // Set approved to unconfirmed.
    approved = 0;

    pthread_mutex_unlock(&approve_lock);
}

long get_send(int source, const char *key)
{
    MPI_Status status;

    // Blank messages to send and receive
    struct dht_msg msg = blank();
    struct dht_msg ret_msg = blank();
    
    // Put key into message
    strcpy(msg.key, key);

    // Send a request for value
    MPI_Sendrecv(&msg, sizeof(struct dht_msg), MPI_BYTE, source, GET,
                 &ret_msg, sizeof(struct dht_msg), MPI_BYTE, source, RETURN_VALUE, MPI_COMM_WORLD, &status);
    return ret_msg.value;
}

long dht_get(const char *key)
{
    // Source of the key
    int source = hash(key);
    // Send the key
    long value = get_send(source, key);

    // Value at the key
    return value;
}

size_t dht_size()
{
    int size = 0;
    // Requests for wait
    MPI_Request reqs[nprocs];
    struct dht_msg msg = blank();
    // All threads get requested for their size
    for (int i = 0; i < nprocs; i++) 
    {
        MPI_Isend(&msg, sizeof(struct dht_msg), MPI_BYTE, i, SIZE, MPI_COMM_WORLD, &reqs[i]);
    }
    // All sends need to complete before receive
    MPI_Waitall(nprocs, reqs, MPI_STATUSES_IGNORE);
    
    // Receive total
    MPI_Status status;
    MPI_Recv(&size, 1, MPI_LONG, rank, TOTAL_SIZE, MPI_COMM_WORLD, &status);
    return size;
}

void dht_sync()
{
    // All clients wait until all clients sync
    MPI_Barrier(MPI_COMM_WORLD);
}

void dht_destroy(FILE *output)
{
    // Wait for all threads
    dht_sync();
    MPI_Request req;
    local_destroy(output);
    struct dht_msg msg = blank();
    // Sends a message that the server thread is terminating
    MPI_Isend(&msg, sizeof(struct dht_msg), MPI_BYTE, rank, DESTROY, MPI_COMM_WORLD, &req);
    pthread_join(threadid, NULL);
    // Clean up
    MPI_Finalize();
}
