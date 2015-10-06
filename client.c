/*
 * A simple http client using  TCP.
 * Some  of  this  code  is  copied  from  internet  and is
 * not  from our authorship, the way  to solve this  problem
 * is usually the same, so probably the majority of examples
 * looks similar.
 *
 * Usage is as follows >  ./cliente [IP] [Puerto] [Archivo] [K] [N]
 *
 * Instituto Tecnológico de Costa Rica
 * Principios de Sistemas Operativos
 * Prof: Eddy Ramírez Jiménez
 * Melvin Elizondo Perez
 * Daniel Solís Méndez
 * Second Semester 2015
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <math.h>

#define BUFSIZ 32768

// Functions definitions
void *getfile(void * args);
void stats();
void servconfig();
void error(char *msg);

// Main variables for the client.
char *ip, *reqfile;
int k, n, port, ind = 0;
double *timestats;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct sockaddr_in serv_addr;

int main(int argc, char *argv[])
{
    // Fetching arguments from console.
    ip = argv[1];
    port = atoi(argv[2]);
    reqfile = argv[3];
    n = atoi(argv[4]);
    k = atoi(argv[5]);
    timestats = calloc(n*k, sizeof(double)); // Array for the times of execution.

    // Memory for threads.
    pthread_t *tid;
    tid = calloc(n, sizeof(pthread_t));
    int i;
    servconfig();

    printf("Executing...\n");
    for (i = 0; i < n; i++)
        if (pthread_create(&tid[i], NULL, &getfile, (void *) i))
            error("Error creating thread\n");

    for (i=0; i<n; i++) pthread_join(tid[i], NULL);

    // Print stats of execution.
    stats();

    return 0;
}

/*
 * Function for each of the threads.
 * Each Thread request a file k times to the server.
 * */
void *getfile(void * args){

    int sockfd, tn =  (int *)args, i;
    tn++;

    // Send message to server.
    char message[128];

    printf("Thread #%d started\n",tn);
    for (i=0; i<k; i++) {
        // Create socket.
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            error("ERROR opening socket");

        // Connect to server.
        if ( connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
            error("ERROR connecting");

        snprintf(message, sizeof(message), "GET /%s HTTP/1.1\r\nHost: localhost\r\n\r\n", reqfile);

        // Take time.
        clock_t start = clock(), diff;

        // Request file.
        if (write(sockfd, message, strlen(message)) < 0)
            puts("Send failed");

        // Receive file from server.
        int bytes;
        char buffer[BUFSIZ];
        while((bytes = read(sockfd, buffer, BUFSIZ) ) > 0);

        // Get execution time.
        diff = clock() - start;
        int msec1 = diff * 1000 / CLOCKS_PER_SEC;
        printf("Time is %d seconds %d milliseconds\n",msec1/1000, msec1%1000);

        pthread_mutex_lock(&mutex);
        timestats[ind++] = diff;
        pthread_mutex_unlock(&mutex);
    }
    printf("Thread #%d ended\n",tn);
};

/*
 * Set the configuration for the target server.
 * */
void servconfig(){
    struct hostent *server;
    if ( (server = gethostbyname(ip)) == NULL)
        error("ERROR, host does not exist.\n");

    // Setting values for sockaddr_in struct.
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(port);
}

/*
 * Print stats about times of execution
 * of every request to the server.
 * Data printed includes average and variance.
 */
void stats() {
    double avg = 0, variance = 0;
    int i;
    for(i = 0; i<(n*k); i++) avg += timestats[i];
    avg /= (n*k);
    for(i = 0; i<(n*k); i++) variance += pow(timestats[i]-avg, 2);
    variance /= (n*k);

    int msec1 = avg * 1000 / CLOCKS_PER_SEC;
    int msec2 = variance * 1000 / CLOCKS_PER_SEC;
    msec2 /= 1000;

    printf("Average is %d seconds %d milliseconds\n",msec1/1000, msec1%1000);
    printf("Variance is %d seconds %d milliseconds\n",msec2/1000, msec2%1000);
}

/*
 * Function to handle displaying
 * errors.
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}
