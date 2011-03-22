/** Server to manage xbox clients
 * @file xbox_management_server.c
 * @author Simon Barth <Simon.Barth@gmx.de>
 */

#include "xbox_management_server.h"

/* TODO: get IP's from DNS 
 */

int main(int argc, char **argv) {
	int sockfd, newSocket;
	char clientName[INET_ADDRESTRELEN];
	struct sockaddr_in clientAddress, serverAddress;
	socklen_t clientLength = sizeof(clientAddress);
	char* path;
	int nread = -1; 
	

	openlog(argv[0], LOG_CONS, LOG_USER);
	
	if( (path = (char *)malloc(sizeof(char)*33)) == NULL ) {
		syslog(LOG_ERR, "Couldn't allocate memory for filepath");
	}
	strcpy(path, PATH);


	if( (sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		/* loggen und so und fehlerbehandlugn */
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

		/* warten auf eingehende verbindungen */
		if( (newSocket = accept(sockfd, (struct sockaddr*) &clientAddress, &clientLength)) < 0 ) {
			/* logging and error handling */
			syslog(LOG_ERR, "error while accepting");
		}
		/* Hier neuen Thread starten */

		if( (inet_ntop(AF_INET, &clientAddress.sin_addr.s_addr, clientName, sizeof(clientName)) == NULL ) ) {
			/* logging and error handling */
			syslog(LOG_ERR, "error at determining client information");
		}
		
		syslog(LOG_INFO, "Connected to %s", clientName);

		if ( clientKnown(clientName) ) {
			nread = processCommunication(newSocket, clientName, path);
		}else{
			syslog(LOG_INFO, "Unknown Client");
		}

		if( nread == 0 ) {
			/* verbindung von client geschlossen */
			syslog(LOG_INFO, "Client %s has closed connection", clientName);
		}else{
			syslog(LOG_ERR, "read");
		}

		if( close(newSocket) < 0 ){
			/* logging and error handling */
			syslog(LOG_ERR, "Couldn't close connected Socket to %s", clientName);
		}
		/* Hier Thread beenden */

	}
	free(path);
	syslog(LOG_INFO, "Closing socket");
	closelog();
	exit(EXIT_SUCCESS);

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
void registerBox(const char *clientName, char *path) {
	
	if ( opendir(registry_path) == NULL ) {
		mkdir(registry_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
	}

	mode_t permissions = S_IRUSR|S_IWUSR|S_IRGRP;
	strcat(path, clientName);
	if ( open(path, O_CREAT, permissions ) < 0) {
		/* logging and error handling */
		syslog(LOG_ERR, "Couldn't create file representing Xbox");
	}
	else {
		syslog(LOG_DEBUG, "file created");
	}
	strcpy(path, PATH);
}

/** implementation of the management protocoll. 
 * @param socket_fd a file descriptor pointing to a connected socket to the client
 * @param clientName a constant character pointer, pointing to a c-string that contains the client IP-Address
 * 	in human readable form
 * @param path a char pointer to the path in which the clients are administered
 */
int processCommunication(const int socket_fd, const char *clientName, char *path) {
	char line[MAX_MSG];
	char answer[ANSWER_SIZE];
	int byteread;
	memset(line, '\0', MAX_MSG);
	while( (byteread=read(socket_fd, line, MAX_MSG) ) > 0) {
		/* hier das protokoll implememtieren */

		syslog(LOG_INFO, "Client is: %s and sent %s", clientName, line); 
		
		if( (strcmp("startup", line) ) == 0){
			syslog(LOG_DEBUG, "startup was sent");
			registerBox(clientName, path);
		}

		else if( (strcmp("shutdown", line) ) == 0){
			syslog(LOG_DEBUG, "shutdown was sent");
				
			unregisterBox(clientName, path);

			if ( boxesRegistered(path) == 0 ) {
				syslog(LOG_INFO, "Server is shut down by xbox_management_server");
				/* Nachricht an client senden dass server heruntergefahren wird */
				memset(answer, '\0', ANSWER_SIZE);
				strcpy(answer, "off");
				send(socket_fd, answer, ANSWER_SIZE, 0); 
				serverShutdown();
			}
				
			else{
				/* Nachricht an client senden, dass server NICHT herunter gefahren wird */
				syslog(LOG_INFO, "Server recieved shut down message but there are still clients registered");
				memset(answer, '\0', ANSWER_SIZE);
				strcpy(answer, "on");
				send(socket_fd, answer, ANSWER_SIZE, 0);
			}

		}

		else {
			syslog(LOG_ERR, "Unknown message");
		}
		
		memset(line, '\0', MAX_MSG);			
	}
	return byteread;
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
void unregisterBox(const char *clientName, char *path) {
	strcat(path, clientName);
	if ( remove(path) < 0) {
		/* logging and error handling */
		syslog(LOG_ERR, "Couldn't remove file representing Xbox");
	}
	strcpy(path, PATH);
}

/**
 * A small wrapper for countEntriesInDir() which returns the amount of registered clients.
 *
 * @param path a constant char pointer which points to a c-string containing the path of the directory
 * 	in which the clients are administered.
 * @return integer representing the number of registerd clients
 */
int boxesRegistered(const char *path) {
	return (countEntriesInDir(path) - 2);
}

/**
 * Shuts down the host which is running xbox_management_server.
 */
void serverShutdown() {
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
int countEntriesInDir(const char *dirname) {
        int n=0;
        struct dirent* d;
        DIR* dir = opendir(dirname);
        if (dir == NULL) return 0;
        while((d = readdir(dir))!=NULL) n++;
        closedir(dir);
        return n;
}
