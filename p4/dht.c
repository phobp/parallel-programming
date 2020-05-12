/**
 * dht.c
 *
 * CS 470 Project 4
 *
 * Implementation for distributed hash table (DHT).
 *
 * Name: Emma Magner, Brendan Pho
 *
 */

#include <mpi.h>
#include <pthread.h>
#include "dht.h"

// tags for message types
#define PUT 1 
#define GET 2
#define CONFIRM 3
#define SIZE 4
#define DESTROY 5 

// tags for messages types for threads 
#define RETURN_VALUE 10
#define TOTAL_SIZE 5

//Private module variable: current process ID
static int pid;


//Private module variable: number of processes
static int nprocs;

int rank;
int lsize;

int approved;

pthread_mutex_t approve_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t approve_cond = PTHREAD_COND_INITIALIZER; 
pthread_t tid;

// struct for messages that will be passed through MPI messaging
struct dht_msg {
    char key[MAX_KEYLEN];
    long value;
};

struct dht_msg blank() {
    struct dht_msg msg;
    memset(&msg, 0, sizeof(struct dht_msg));
    return msg;
}

void recv_tag(struct dht_msg *msg, int tag, MPI_Status *recv_status) 
{
    MPI_Recv(msg, sizeof(struct dht_msg), MPI_BYTE, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, recv_status);
}

/*
 * given a key name, return the distributed hash table owner
 * (uses djb2 algorithm: http://www.cse.yorku.ca/~oz/hash.html)
 */
int hash(const char *name)
{
    unsigned hash = 5381;
    while (*name != '\0') {
        hash = ((hash << 5) + hash) + (unsigned)(*name++);
    }
    return hash % nprocs;
}

/*
 * Server thread for each process that receives a message and handles accordingly
 * based on message type and may possible send back a return value.
 */
void *server(void *arg) {


    // wait for and process requests
    while(true) {
        struct dht_msg msg;
        MPI_Status status;
        MPI_Status recv_status;
        
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	// handle based on the message tag of received message
        switch (status.MPI_TAG) { 
           case PUT:
                 recv_tag(&msg, PUT, &recv_status);
                 MPI_Request req;
                 local_put(msg.key, msg.value);

                 struct dht_msg ret_msg = blank();
                 ret_msg.value = CONFIRM;

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
                 ; 
                 long val = local_get(msg.key);
                 recv_tag(&msg, GET, &recv_status);
                 MPI_Request request;
                 struct dht_msg get_value = blank();
                 get_value.value = val; 

                
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

                 recv_tag(&msg, CONFIRM, &recv_status);
                 approved = 1;
                 pthread_mutex_unlock(&approve_lock);
                 pthread_cond_signal(&approve_cond);
                 break;
            case SIZE:
                 ;
                 long local_sz = 0; 
                 long total_sz = 0;
                 recv_tag(&msg, SIZE, &recv_status);
                 local_sz = local_size();
                 MPI_Reduce(&local_sz, &total_sz, 1, MPI_LONG, MPI_SUM, recv_status.MPI_SOURCE, MPI_COMM_WORLD);

                 if (rank == recv_status.MPI_SOURCE)
                 {
                     MPI_Send(&total_sz, 1, MPI_LONG, recv_status.MPI_SOURCE, total_sz, MPI_COMM_WORLD);
                 }

                 break;
            case DESTROY:

                 recv_tag(&msg, DESTROY, &recv_status);
                 pthread_exit(NULL);
                 break;
            default:
                 return NULL;
        }
    }
}

int dht_init()
{

    local_init();

    int provided;
    approved = 0;

    MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided);
    if (provided != MPI_THREAD_MULTIPLE) {
        printf("ERROR: Cannot initialize MPI in THREAD_MULTIPLE mode.\n");
        exit(EXIT_FAILURE);
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    pid = rank;

    // create the server thread for the process
    if (pthread_create(&tid, NULL, server, NULL) != 0) {
        printf("ERROR: Cannot create server thread.\n");
        exit(EXIT_FAILURE);
    }

    return pid;
}

void put_send(int dest, const char *key, long value)
{
    struct dht_msg msg = blank();
    if (key != NULL) 
    {
        strcpy(msg.key, key);
    }
    msg.value = value;
    MPI_Ssend(&msg, sizeof(struct dht_msg), MPI_BYTE, dest, PUT, MPI_COMM_WORLD);
}

long get_send(int source, const char *key)
{
    MPI_Status status;
    struct dht_msg msg = blank();
    struct dht_msg ret_msg = blank();
    if (key != NULL) 
    {
        strcpy(msg.key, key);   
    }

    MPI_Sendrecv(&msg, sizeof(struct dht_msg), MPI_BYTE, source, GET,
                 &ret_msg, sizeof(struct dht_msg), MPI_BYTE, source, RETURN_VALUE, MPI_COMM_WORLD, &status);
    return ret_msg.value;
}

void dht_put(const char *key, long value)
{
    int dest = hash(key);
    put_send(dest, key, value);
    pthread_mutex_lock(&approve_lock);
    while(!approved) 
    {
        pthread_cond_wait(&approve_cond, &approve_lock);
    }

    approved = 0;
    pthread_mutex_unlock(&approve_lock);
}

long dht_get(const char *key)
{
    int source = hash(key);
    long value = get_send(source, key);
    return value;
}

size_t dht_size()
{
    int size = 0;
    MPI_Request reqs[nprocs];
    struct dht_msg msg = blank();

    for (int i = 0; i < nprocs; i++) 
    {
        MPI_Isend(&msg, sizeof(struct dht_msg), MPI_BYTE, i, SIZE, MPI_COMM_WORLD, &reqs[i]);
    }

    MPI_Waitall(nprocs, reqs, MPI_STATUSES_IGNORE);
    MPI_Status status;
    MPI_Recv(&size, 1, MPI_LONG, rank, TOTAL_SIZE, MPI_COMM_WORLD, &status);
    return size;
}

void dht_sync()
{
    MPI_Barrier(MPI_COMM_WORLD);
}

void dht_destroy(FILE *output)
{
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Request req;
    local_destroy(output);
    struct dht_msg msg = blank();
    MPI_Isend(&msg, sizeof(struct dht_msg), MPI_BYTE, rank, DESTROY, MPI_COMM_WORLD, &req);
    pthread_join(tid, NULL);
    MPI_Finalize();
}

