/** Server to manage xbox clients
 * @file xbox_management_server.c
 * @author Simon Barth <Simon.Barth@gmx.de>
 */

#ifndef __FreeBSD__
#define _POSIX_C_SOURCE 200809L
#endif
#include "xbox_management_server.h"

/* TODO: get IP's from DNS 
 */

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
		syslog(LOG_ERR, "Failure binding to socket");
	}
	
	syslog(LOG_DEBUG, "Listening on %s:%d", SERVER_IP, SERVER_PORT);
	if( listen(sockfd, 5) < 0 ) {
		/* logging and error handling */
		syslog(LOG_ERR, "listening error");
	}

	while(true) {

		struct data *arg = (struct data*)malloc(sizeof(struct data));
		arg->clientAddress = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));		

		/* warten auf eingehende verbindungen */
		if( (newSocket = accept(sockfd, (struct sockaddr *)arg->clientAddress, &clientLength)) < 0 ) {
				/* logging and error handling */
			syslog(LOG_ERR, "error while accepting");
		}		
		arg->socketfd = newSocket;
		strcpy(arg->path, PATH);
		pthread_create(&thread, NULL, &startThread, arg);

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
	char *clientName = (char *)malloc((sizeof(char) * INET_ADDRESTRELEN));
	int nread = -1; 

	if( (inet_ntop(AF_INET, &client->clientAddress->sin_addr.s_addr, clientName, INET_ADDRESTRELEN) == NULL ) ) {
		/* logging and error handling */
		syslog(LOG_ERR, "error at determining client information");
	}
	
	syslog(LOG_INFO, "Connected to %s", clientName);
	if ( clientKnown(clientName) ) {
		nread = processCommunication(client->socketfd, clientName, client->path);
	}else{
		syslog(LOG_INFO, "Unknown Client");
	}


	if( nread == 0 ) {
		/* verbindung von client geschlossen */
		syslog(LOG_INFO, "Client %s has closed connection", clientName);
	}else{
		syslog(LOG_ERR, "read");
	}

	if( close(client->socketfd) < 0 ){
		/* logging and error handling */
		syslog(LOG_ERR, "Couldn't close connected Socket to %s", clientName);
	}
	free(clientName);
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
	// DONE: make threadsafe
	// DONE: create directory if it doesn't exist
	// TODO: check if file allready exists
	char* registry_path;
	bool waiting = false;
	if ( (registry_path = (char *)malloc(sizeof(PATH) + INET_ADDRESTRELEN)) == NULL ) {
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
int processCommunication(const int socket_fd, const char *clientName,const char *path) {
	// TODO: split to subfunctions
	// DONE: make thread safe
	char *line = (char *)malloc(sizeof(char) * MAX_MSG);
	int bytesread;
	memset(line, '\0', MAX_MSG);
	while( (bytesread=read(socket_fd, line, MAX_MSG) ) > 0) {
		/* hier das protokoll implememtieren */

		syslog(LOG_INFO, "Client is: %s and sent %s", clientName, line); 
		
		processMessage(line, path, clientName, socket_fd);			
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
	//DONE: make thread safe
	//TODO: check if path and file exist
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
	// DONE: make thread safe
	// TODO: catch case if directory doesn't exist
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
static void processMessage(const char *line, const char *path, const char *clientName, int connection) {

	char *answer = (char *)malloc(sizeof(char) * ANSWER_SIZE);
	
	if( (strcmp("startup", line) ) == 0){
		syslog(LOG_DEBUG, "startup was sent");
		registerBox(clientName, path);
	}

	else if( (strcmp("shutdown", line) ) == 0){
		syslog(LOG_DEBUG, "shutdown was sent");
			
		unregisterBox(clientName, path);

		pthread_rwlock_rdlock(&threads_waiting_mutex);
		// check if there are any other clients registered or other
		// threads waiting to register new ones
		if ( (boxesRegistered(path) == 0) && (threads_waiting == 0) ) {
			syslog(LOG_INFO, "Server is shut down by xbox_management_server");
			/* Nachricht an client senden dass server heruntergefahren wird */
			memset(answer, '\0', ANSWER_SIZE);
			strcpy(answer, "off");
			send(connection, answer, ANSWER_SIZE, 0); 
			serverShutdown();
		}
		else{
			/* Nachricht an client senden, dass server NICHT herunter gefahren wird */
			syslog(LOG_INFO, "Server recieved shut down message but there are still clients registered");
			memset(answer, '\0', ANSWER_SIZE);
			strcpy(answer, "on");
			send(connection, answer, ANSWER_SIZE, 0);
		}
		pthread_rwlock_unlock(&threads_waiting_mutex);	

	}

	else {
		syslog(LOG_ERR, "Unknown message");
	}
	free(answer);
}
