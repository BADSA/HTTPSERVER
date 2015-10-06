/*
 * A  simple  server  in  the  internet  domain  using  TCP
 * Some  of  this  code  is  copied  from  internet  and is
 * not  from our authority, the way  to solve this  problem
 * is usually the same, so probably the majority of examples
 * looks similar.
 *
 * The port number, source folder and mode is passed as an argument
 *
 * Instituto Tecnologico de Costa Rica
 * Melvin Elizondo Perez
 * Daniel Solis Mendez
 * II Semester 2015
 *
 *
 * Web Server 5 modes usage.
*/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>

#define BYTES 1024
#define MAXSOCK 64
/*
 * Function definitions.
 */

void response(void*);
void fifo(void*);
void forked(void*);
void threaded(void*);
void pre_forked(void*);
void pre_threaded(void*);
void *stop_server(void*);
void error(char *msg);
void *handle_req(void * args);

/*
 * Struct definition
 *
 * This is needed because the threads only receive void* parameter
 * This struct is parsed as void and parsed back into struck in the
 * different functions to get the values.
 */

typedef struct s_params{
    int k,sockfd,clilen;
    char resource[100];
    struct sockaddr_in cli_addr;
} params;

typedef struct s_response_params {
    int sock;
    char resource[100];
} rparams;

// Global var for mutex.
int socks[MAXSOCK], attendClient, newClient;
pthread_mutex_t	cliMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cliCond = PTHREAD_COND_INITIALIZER;

char resource[100];

/*
 * Main function, it handles all the socket info,
 * Also determine which mode the server is going
 * to run.
 *
 * Some other statements are declared here, they
 * are needed to the socket to work correctly.
 */

int main(int argc, char *argv[]) {
    //Variable declaration
    int sockfd, portno,modenum,k;
    struct sockaddr_in serv_addr, cli_addr;
    char mode[15],port[10],k_process[5];
    params fun_params;

    //Just to print which mode was selected
    const char *MODE[5] = {"FIFO","Forked",
                           "Threaded","Pre-Forked","Pre-Threaded"};

    //Arguments error handling
    if (argc < 4){
        fprintf(stderr,"ERROR, too few arguments, 4 or 5 expected\n");
        exit(1);
    }

    //Socket definition
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    //Sock error handling
    if (sockfd < 0)
        error("ERROR opening socket");

    //Cleaning serv_addr struck, coping arguments provided
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    strcpy(port,argv[1]);
    strcpy(resource, argv[2]);
    modenum = atoi(argv[3]);
    strcpy(mode,MODE[modenum-1]);

    void* status;
    pthread_t read_console;
    pthread_create(&read_console,NULL,stop_server,(void*) 1);
    //pthread_join(read_console,&status);


    //Just a smooth touch
    printf("BADSA Server iniciado\n");
    printf("Numero de puerto: %s%s%s \n","\033[92m", port, "\033[0m");
    printf("Carpeta solicitada: %s%s%s \n","\033[92m", resource, "\033[0m");
    printf("Modo de servidor: %s%s%s (%d)\n","\033[92m", mode, "\033[0m",modenum);
    if (argc == 5){
        strcpy(k_process,argv[4]);
        k = atoi(argv[4]);
        printf("Cantidad de procesos: %s%s%s \n","\033[92m", k_process, "\033[0m");
    }

    //Filling serv_addr attributes
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    //Binding and error handling
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    //Syscall to listen socket requests
    listen(sockfd,5);

    //Filling params to the functions
    fun_params.k = k;
    fun_params.cli_addr = cli_addr;
    fun_params.clilen = sizeof(cli_addr);
    fun_params.sockfd = sockfd;
    memcpy(fun_params.resource, resource, strlen(resource)+1);

    int t = 0;
    switch(modenum){
        case 1:
            fifo(&fun_params);
            break;
        case 2:
            forked(&fun_params);
            break;
        case 3:;
            threaded(&fun_params);
            break;
        case 4:
            if (argc < 5)
                error("Value of k not provided\n");
            pre_forked(&fun_params);
            break;
        case 5:
            if (argc < 5)
                error("Value of k not provided\n");
            pre_threaded(&fun_params);
            break;
        default:
            break;
    }
    return 0; /* we never get here */
}

/*
 * FIFO
 * There is a separate instance of this
 * function for each connection. It handles
 * one connection at time, FIFO order.
 *
 * This function duplicated if threaded or
 * pre-threaded mode is selected.
 */

void fifo(void* fun_params) {
    params fparams = *((params*)(fun_params));
    int sock;

    while (1) {
        sock = accept(fparams.sockfd,
                      (struct sockaddr *) &fparams.cli_addr, &fparams.clilen);
        if (sock < 0)
            error("ERROR on accept");
        rparams response_params;
        response_params.sock = sock;
        memcpy(response_params.resource,fparams.resource,strlen(fparams.resource)+1);
        response(&response_params);
    }
}

/*
 * Forked
 * This function create a process to handle
 * each request made to the socked.
 */

void forked(void* fun_params) {
    params fparams = *((params*)(fun_params));
    int newsockfd;
    int pid;
    while (1) {
        newsockfd = accept(fparams.sockfd,
                    (struct sockaddr *) &fparams.cli_addr,
                    &fparams.clilen);

        if (newsockfd < 0)
            error("ERROR on accept");

        pid = fork();

        if (pid < 0)
            error("ERROR on fork");

        if (pid == 0){
            close(fparams.sockfd);
            rparams response_params;
            response_params.sock = newsockfd;
            memcpy(response_params.resource,fparams.resource,strlen(fparams.resource)+1);
            response(&response_params);
            exit(0);
        }else
            close(newsockfd);
    }
}

void threaded(void* fun_params){
    params fparams = *((params*)(fun_params));
    int newsockfd;
    int loops =0;
    while (1) {
        printf("loops %d\n",loops++);
        newsockfd = accept(fparams.sockfd,
                           (struct sockaddr *) &fparams.cli_addr,
                           &fparams.clilen);

        if (newsockfd < 0)
            error("ERROR on accept");

        rparams response_params;
        response_params.sock = newsockfd;
        memcpy(response_params.resource,fparams.resource,strlen(fparams.resource)+1);

        pthread_t thread;

        if (pthread_create(&thread,NULL,&response,&response_params) < 0){
            close(newsockfd);
            error("Error creating thread");
        }
    }
}

void pre_forked(void *fun_params){
    params fparams = *((params*)(fun_params));
    int newsockfd;
    int i;
    pid_t pid;
    for(i=0; i<fparams.k; i++){
        pid = fork();
        if (pid == 0) {
            while(1){

                newsockfd = accept(fparams.sockfd,
                                   (struct sockaddr *) &fparams.cli_addr,
                                   &fparams.clilen);
                printf("Nueva Solicitud");

                if(newsockfd>=0) {
                    rparams response_params;
                    response_params.sock = newsockfd;
                    memcpy(response_params.resource, fparams.resource, strlen(fparams.resource) + 1);
                    response(&response_params);
                    close(newsockfd);
                }
            }
        } else if(pid<0){
            close(fparams.sockfd);
            error("Error on fork");
        }

    }
    close(fparams.sockfd);
    for (;;)
        pause();
}

void pre_threaded(void *fun_params){

    params fparams = *((params*)(fun_params));
    pthread_t *tid = calloc(fparams.k, sizeof(pthread_t));
    int sock, i;

	for (i = 0; i < fparams.k; i++){
		if (pthread_create(&tid[i], NULL, &handle_req, i))
			error("ERROR: pthread_create");
    }


	attendClient = newClient = 0;
	while(1){
		sock = accept(fparams.sockfd, NULL,NULL);
		if(sock==-1)
			error("ERROR: could not accept new connection on socket");
		pthread_mutex_lock(&cliMutex);
		socks[newClient] = sock;
		if (++newClient == MAXSOCK) newClient = 0;
		if (newClient == attendClient) error("ERROR: newClient = attendClient");
		pthread_cond_signal(&cliCond);
		pthread_mutex_unlock(&cliMutex);
	}
}

void *handle_req(void * args){
	int sock;
    int tn = ((int*)(args));

	pthread_detach(pthread_self());
	printf("Thread #%d created and waiting for clients\n", ++tn);

	while(1){
    	pthread_mutex_lock(&cliMutex);
		while (attendClient == newClient)
			pthread_cond_wait(&cliCond, &cliMutex);
		sock = socks[attendClient];
		if (++attendClient == MAXSOCK) attendClient = 0;
		pthread_mutex_unlock(&cliMutex);

        rparams fparams;
        fparams.sock = sock;
        printf("sock es %d\n",sock);
		response(&fparams);
	}
	return 0;
}

void response(void *fun_params){
    rparams fparams = *((rparams*)(fun_params));
    char mesg[99999], *reqline[3] ,data_to_send[BYTES], path[99999];
    int fd,bytes_read;
    printf("sock2 es %d\n",fparams.sock);
    recv(fparams.sock, mesg, 99999, 0);

    reqline[0] = strtok (mesg, " \t\n");
    reqline[1] = strtok(NULL, " \t");

    strcpy(path, resource);
    printf("recurso: %s\n",reqline[1]);
    strcpy(&path[strlen(resource)], reqline[1] == NULL ? "/\0" : reqline[1]);
    printf("file: %s\n", path);
    if (reqline[1]!=NULL)
    if (strncmp(reqline[1], "/\0", 2)==0 ){
        send(fparams.sock, "HTTP/1.0 200 OK\n\n", 17, 0);
        char msg[] = {"<h1>Bienvenido a BADSA SERVER</h1></br>Se encuentra en la carpeta principal"};
        write (fparams.sock, msg, strlen(msg));
    }else if ( (fd=open(path, O_RDONLY)) != -1 ){   //FILE FOUND
        send(fparams.sock, "HTTP/1.0 200 OK!\n\n", 18, 0);

        while ((bytes_read=read(fd, data_to_send, BYTES))>0)
            write (fparams.sock, data_to_send, bytes_read);

    }else{
        printf("Not found\n");
        send(fparams.sock, "HTTP/1.0 404 Not Found\n", 23, 0);
        write(fparams.sock, "HTTP/1.0 404 Not Found\n", 23);
    }


    shutdown(fparams.sock, SHUT_RDWR);
    close(fparams.sock);

}

/*
 * Just in case you want to shut the server off
 * Just type "fin"
 */

void* stop_server(void* args) {
    char end[4];
    printf("\nWrite \"fin\" if you want to shut the server off: \n");
    scanf("%s", &end);
    while ( strcmp(end, "fin") != 0  ){
        printf("\nWrite \"fin\" if you want to shut the server off: \n");
        scanf("%s", &end);
    }
    printf("Server was shutted down\n");
    exit(0);
}

//Error printing message
void error(char *msg){
    perror(msg);
    exit(1);
}
