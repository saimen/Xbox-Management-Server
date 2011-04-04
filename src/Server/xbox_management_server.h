/**
 * @mainpage Xbox management server. 
 * This is a small home project of mine to start my fileserver only when needed and also shut it down when it's not needed, automatically.
 * 
 * @section install_sec Installation
 *
 * @subsection step1 Step 1: BlaBla
 * 
 * @subsection step2 Step 2: BlaBlaBla
 */


/**
 * @file xbox_management_server.h
 * @brief This header-file includes all necessary headers and makes all defines needed by xbox_management_server.c
 *
 * @author Simon Barth <Simon.Barth@gmx.de>
 */

/**
 * @def INET_ADDRESTRELEN
 * The length of an IP-Address
 */

/**
 * @def MAX_MSG
 * Buffer size for maximum message length
 */

/**
 * @def ANSWER_SIZE
 * maximum length of the answer
 */

/**
 * @def SERVER_PORT
 * Port on wich the server listens
 */

/**
 * @def STARTUP_MESSAGE_SIZE
 * The size of a startup message including the terminal symbol '\0'
 */

/**
 * @def SHUTDOWN_MESSAGE_SIZE
 * The size of a shutdown message including the terminal symbol '\0'
 */

/**
 * @def PATH
 * The path where the files that represent registered clients are saved to
 */

/**
 * @def PATHLEN
 * The length of the string representing the path with the '\0' character
 */

/**
 * @def XBOX_ESSZIMMER
 * IP-Address of a client
 */

/**
 * @def XBOX_SIMON
 * IP-Address of a client
 */

/**
 * @def MACBOOK_SIMON
 * IP-Address of test-client
 */

/**
 * @def SERVER_IP
 * IP-Address of the management server
 */

#ifndef XBOX_MANAGEMENT_SERVER_H
#define XBOX_MANAGEMENT_SERVER_H XBOX_MANAGEMENT_SERVER_H


#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h> //memset()
#include <stdio.h> //perror()
#include <fcntl.h> //open()
#include <syslog.h> 
#include <dirent.h> //scandir()
#include <sys/stat.h> // setting permissions
#include <pthread.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define INET_ADDRESTRELEN	16
#define MAX_MSG			1024
#define ANSWER_SIZE		4
#define SERVER_PORT		35002


#define STARTUP_MESSAGE_SIZE	8
#define SHUTDOWN_MESSAGE_SIZE	9

#define PATH			"/var/run/clients/"
#define PATHLEN			18

#define XBOX_ESSZIMMER		"192.168.1.115"
#define XBOX_SIMON		"192.168.1.105"
#ifdef TEST_ZW
#define MACBOOK_SIMON		"192.168.1.2"
#define SERVER_IP		"192.168.1.2"
#else
#ifdef TEST
#define MACBOOK_SIMON		"192.168.1.100"
#define SERVER_IP		"192.168.1.100"
#else
#define MACBOOK_SIMON		"192.168.1.100"
#define SERVER_IP		"192.168.1.111"
#endif
#endif

struct data;

void* startThread(void *arg);
bool clientKnown(const char *clientName);
void registerBox(const char *clientName,const char *path);
int processCommunication(struct data *arg);
void unregisterBox(const char *clientName,const char *path);
int boxesRegistered(const char *path);
void serverShutdown();
static int countEntriesInDir(const char* dirname);
static void processMessage(const char *line, const struct data *arg);


#endif
