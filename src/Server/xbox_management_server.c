/** Server to manage xbox clients
 * @file xbox_management_server.c
 * @author Simon Barth <Simon.Barth@gmx.de>
 */

#ifndef __FreeBSD__
#define _POSIX_C_SOURCE 200809L
#endif
#include "xbox_management_server.h"

pthread_t thread;

pthread_rwlock_t dir_mutex = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t threads_waiting_mutex = PTHREAD_RWLOCK_INITIALIZER;

static int threads_waiting = 0;

/**
 * \struct data
 * A structure that contains connection data
 */
struct data {
	/*@{*/
	int socketfd; /**< the file descriptor of the connected socket */
	struct sockaddr_in *clientAddress; /**< the IP-Address of the connected client */
	char *clientName; /**< the IP-Address of the connected client as string */
	char path[sizeof(PATH)]; /**< the path in wich the clients are registered */
	/*@}*/
};

int main(int argc, char **argv) {
	(void) argc;
	int sockfd, newSocket;
	struct sockaddr_in serverAddress;
	socklen_t clientLength = sizeof(serverAddress);
	

	openlog(argv[0], LOG_CONS, LOG_USER);
	
	if( (sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		/* loggen und so und fehlerbehandlung */
		syslog(LOG_ERR, "Failure at opening socket");
	}

	memset(&serverAddress, 0 , sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(SERVER_PORT);
	if ( inet_pton(AF_INET, SERVER_IP, &serverAddress.sin_addr) != 1 ) {
		syslog(LOG_ERR, "Couldn't set host address");
	}
	
	syslog(LOG_DEBUG, "Binding socket");
	if( bind(sockfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0 ) {
		/* logging and error handling */
		switch(errno) {
		case EADDRINUSE:
			syslog(LOG_ERR, "Either there is another program listening on the same port and address," 
						"or there is another instance of this program already started");
			break;
		case EADDRNOTAVAIL:
			syslog(LOG_ERR, "This host has no interface whis is asigned the IP: %s", SERVER_IP);
			break;
		case EISCONN:
			syslog(LOG_ERR, "This socket is already connected");
			break;
		default:
			syslog(LOG_ERR, "Unknown error while binding to %s:%d", SERVER_IP, SERVER_PORT);
		}
	}
	
	syslog(LOG_DEBUG, "Listening on %s:%d", SERVER_IP, SERVER_PORT);
	if( listen(sockfd, 5) < 0 ) {
		/* logging and error handling */
		syslog(LOG_ERR, "listening error");
	}

	while(true) {
		struct data *arg = NULL;
		if( (arg = (struct data *)malloc(sizeof(struct data))) == NULL ) {
			syslog(LOG_ERR, "error allocating memory for struct data");
			continue;
		} else {
			if( (arg->clientAddress = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in))) == NULL) {
				syslog(LOG_ERR, "error allocating memory for clientAddress");
				continue;
			}

			if( (arg->clientName = (char *) calloc(INET_ADDRESTRELEN, sizeof(char))) == NULL) {
				syslog(LOG_ERR, "error allocating memory for clientName");
				continue;
			}
		}

		/* warten auf eingehende verbindungen */
		if( (newSocket = accept(sockfd, (struct sockaddr *)arg->clientAddress, &clientLength)) < 0 ) {
			/* logging and error handling */
			switch(errno) {
			case ECONNABORTED:
				syslog(LOG_ERR, "Connection has been aborted");
				break;
			case EINTR:
				syslog(LOG_ERR, "accept() was interrupted by a signal");
				break;
			case ENOMEM:
				syslog(LOG_ERR, "not enough memory");
				break;
			case EPROTO:
				syslog(LOG_ERR, "A protocol error has occured");
				break;
			default:
				syslog(LOG_ERR, "Unknown error");
			}
		}

		if( (inet_ntop(AF_INET, &arg->clientAddress->sin_addr.s_addr, arg->clientName, INET_ADDRESTRELEN) == NULL ) ) {
			/* logging and error handling */
			switch(errno) {
			case EAFNOSUPPORT:
				syslog(LOG_ERR, "Addressfamily not supported by inet_ntop()");
				break;
			case ENOSPC:
				syslog(LOG_ERR, "Not enough memory allocated to store Address");
				break;
			default:
				syslog(LOG_ERR, "Unknown error at determining client information");
			}
		}

		arg->socketfd = newSocket;
		strcpy(arg->path, PATH);
		while(pthread_create(&thread, NULL, &startThread, arg) != 0) {
			if(errno == EAGAIN) {
				syslog(LOG_ERR, "Not enough ressources left to create thread. Retrying");
				continue;
			} else {
				break;
			}
		}

	}
	syslog(LOG_INFO, "Closing socket");
	closelog();
	exit(EXIT_SUCCESS);

}

/**
 * Frees memory behind *arg
 *
 * @param arg a pointer to a struct data structure
 */
void exit_handler_mem(void *arg) {
	struct data *mem = (struct data*) arg;
	free(mem->clientAddress);
	free(mem->clientName);
	free(mem);
}

/**
 * Startfunction of connection handling threads
 *
 * @param arg a pointer to a structure of type data
 */
void* startThread(void *arg) {
	pthread_detach(pthread_self());
	struct data *client = (struct data *) arg;
	pthread_cleanup_push( exit_handler_mem, (void *)arg );
	int nread = -1; 

	syslog(LOG_INFO, "Connected to %s", client->clientName);
	if ( clientKnown(client->clientName) ) {
		nread = processCommunication(client);
	}else{
		syslog(LOG_INFO, "Unknown Client");
	}


	if( nread == 0 ) {
		/* verbindung von client geschlossen */
		syslog(LOG_INFO, "Client %s has closed connection", client->clientName);
	}else{
		syslog(LOG_ERR, "read");
	}

	if( close(client->socketfd) < 0 ){
		/* logging and error handling */
		syslog(LOG_ERR, "Couldn't close Socket connected to %s", client->clientName);
		switch(errno) {
		case EINTR:
			syslog(LOG_ERR, "close() was interrupted by a signal");
			break;
		case EBADF:
			syslog(LOG_ERR, "client->socketfd is not a valid file descriptor");
			break;
		default:
			syslog(LOG_ERR, "Unknown error");
		}
	}
	pthread_cleanup_pop( 1 );
	pthread_exit((void *)pthread_self());
}


/**
 * Checks if a client is known to the server. This check is performed by the client's IP-Address
 *
 * @param clientName a constant character pointer, pointing to a c-string that contains the 
 * 	IP-Address of the client whicht shall be tested
 * @return a boolean signaling if the client is known or not
 */
bool clientKnown(const char *clientName) {
	if(	//list of known clients, replace with a linked list maybe
		(strcmp(clientName, XBOX_ESSZIMMER) == 0) ||
		(strcmp(clientName, XBOX_SIMON) == 0) ||
	       	(strcmp(clientName, MACBOOK_SIMON) == 0)
	  ) {
		return true;
	}
	else{
		return false;
	}
}

/**
 * Register a client on the server. This is done by generating a file with the IP-Address of the
 * client as its name.
 *
 * @param clientName a constant char pointer, pointing to a c-string that contains the IP-Address
 * 	of the client.
 * @param path a char pointer, pointing to a c-string that contains the path in wich the clients
 * 	are administered
 */
void registerBox(const char *clientName,const char *path) {
	if ( opendir(path) == NULL ) {
		mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
	}
	char* registry_path;
	bool waiting = false;
	if ( (registry_path = (char *) calloc(INET_ADDRESTRELEN, sizeof(PATH))) == NULL ) {
		syslog(LOG_ERR, "Couldn't allocate memory for registry_path");
	}
	strcpy(registry_path, path);

	mode_t permissions = S_IRUSR|S_IWUSR|S_IRGRP;
	strcat(registry_path, clientName);
	
	// note that threads wants to register, so server isn't shut
	// down while by another thread while this one is queued
	while( (pthread_rwlock_trywrlock(&dir_mutex) == EBUSY) ){
		if (!waiting ) {
			pthread_rwlock_wrlock(&threads_waiting_mutex);
			threads_waiting += 1;
			waiting = true;
			pthread_rwlock_unlock(&threads_waiting_mutex);
		}
	}
	if( waiting ) {
		pthread_rwlock_wrlock(&threads_waiting_mutex);
		threads_waiting -= 1;
		pthread_rwlock_unlock(&threads_waiting_mutex);
	}

	if ( open(registry_path, O_CREAT, permissions ) < 0) {
		/* logging and error handling */
		syslog(LOG_ERR, "Couldn't create file representing Xbox");
	}
	else {
		syslog(LOG_DEBUG, "file created");
	}
	
	pthread_rwlock_unlock(&dir_mutex);

	free(registry_path);
}

/** implementation of the management protocoll. 
 * @param socket_fd a file descriptor pointing to a connected socket to the client
 * @param clientName a constant character pointer, pointing to a c-string that contains the client IP-Address
 * 	in human readable form
 * @param path a char pointer to the path in which the clients are administered
 */
int processCommunication(struct data *arg) {
	char *line = NULL;
	if( (line = (char *)calloc(MAX_MSG, sizeof(char))) == NULL ){
		syslog(LOG_ERR, "error at allocating memory to buffer recieved message");
	}
	int bytesread;
	memset(line, '\0', MAX_MSG);
	while( (bytesread=recv(arg->socketfd, line, MAX_MSG, 0) ) > 0) {
		/* hier das protokoll implememtieren */

		syslog(LOG_INFO, "Client is: %s and sent %s", arg->clientName, line); 
		
		processMessage(line, arg);
		memset(line, '\0', MAX_MSG);
	}
	free(line);
	return bytesread;
}

/**
 * Unregister a client from the server. To accomplish this, the file in #path with the name in 
 * 	#clientName is deleted.
 *
 * @param clientName a const char pointer, pointing to a c-string which contains the IP-Address
 * 	of the client.
 * @param path a char pointer, pointing to a c-string which cointains the path in which the clients
 * 	are administered
 */
void unregisterBox(const char *clientName,const char *path) {
	char* registry_path;
	if ( (registry_path = (char *)malloc(sizeof(PATH) + INET_ADDRESTRELEN)) == NULL ) {
		syslog(LOG_ERR, "Couldn't allocate memory for registry_path");
	}
	strcpy(registry_path, path);

	strcat(registry_path, clientName);
	pthread_rwlock_wrlock(&dir_mutex);
	if ( remove(registry_path) < 0) {
		/* logging and error handling */
		syslog(LOG_ERR, "Couldn't remove file representing Xbox");
	}
	pthread_rwlock_unlock(&dir_mutex);
	free(registry_path);
}

/**
 * A small wrapper for countEntriesInDir() which returns the amount of registered clients.
 *
 * @param path a constant char pointer which points to a c-string containing the path of the directory
 * 	in which the clients are administered.
 * @return integer representing the number of registerd clients
 */
int boxesRegistered(const char *path) {
	int ret;
	pthread_rwlock_rdlock(&dir_mutex);
	ret = countEntriesInDir(path) - 2;
	pthread_rwlock_unlock(&dir_mutex);
	return ret;
}

/**
 * Shuts down the host which is running xbox_management_server.
 */
void serverShutdown() {
	if ( getuid() != (uid_t)0 ) {
		syslog(LOG_ERR, "Process is not privileged to shut down the computer");
		return;
	}
	if ( execl("/sbin/init", "init", "0", (char *)0) == -1 ){
		syslog(LOG_ERR, "Couldn't initiate shut down sequence");
	}
	else {
		syslog(LOG_INFO, "Server shut down by xbox_management_server");
	}
}
							
/**
 * Counts how many entries exist in a directory.
 *
 * @param dirname is a constant char pointer which points to a c-string that contains the path of
 * 	the directory which's content shall be counted.
 * @return an integer representing the number of entries inside the directory
 * @warning the special entries "." and ".." are also counted so that the minimum return value of
 * 	this function is 2.
 */
static int countEntriesInDir(const char *dirname) {
        int n=0;
        DIR* dir = opendir(dirname);
        if (dir == NULL) return 0;
        while((readdir(dir))!=NULL) n++;
        closedir(dir);
        return n;
}

/**
 * handles the message sent by the client
 *
 * @param line message sent by client
 * @param path path to which clients are registered
 * @param clientName IP-Address of client
 * @param connection file-descriptor of connected socket
 */
static void processMessage(const char *line, const struct data *arg) {
	char *answer = NULL;
	if( (answer = (char *)calloc(ANSWER_SIZE, sizeof(char))) == NULL) {
		syslog(LOG_ERR, "error allocating memory to buffer the answer");
	}
	memset(answer, '\0', ANSWER_SIZE);
	if( (strcmp("startup", line) ) == 0){
		syslog(LOG_DEBUG, "startup was sent");
		registerBox(arg->clientName, arg->path);
	}

	else if( (strcmp("shutdown", line) ) == 0){
		syslog(LOG_DEBUG, "shutdown was sent");
			
		unregisterBox(arg->clientName, arg->path);

		pthread_rwlock_rdlock(&threads_waiting_mutex);
		// check if there are any other clients registered or other
		// threads waiting to register new ones
		if ( (boxesRegistered(arg->path) == 0) && (threads_waiting == 0) ) {
			syslog(LOG_INFO, "Server is shut down by xbox_management_server");
			/* Nachricht an client senden dass server heruntergefahren wird */
			strcpy(answer, "off");
			send(arg->socketfd, answer, ANSWER_SIZE, 0); 
			serverShutdown();
		}
		else{
			/* Nachricht an client senden, dass server NICHT herunter gefahren wird */
			syslog(LOG_INFO, "Server recieved shut down message but there are still clients registered");
			strcpy(answer, "on");
			send(arg->socketfd, answer, ANSWER_SIZE, 0);
		}
		pthread_rwlock_unlock(&threads_waiting_mutex);	

	}

	else {
		syslog(LOG_ERR, "Unknown message");
	}
	free(answer);
}
