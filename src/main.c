#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>

#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "oscc.h"
#include "commander.h"
#include "can_protocols/steering_can_protocol.h"

#define COMMANDER_UPDATE_INTERVAL_MICRO (50000)
#define SLEEP_TICK_INTERVAL_MICRO (1000)

#define BUFSIZE 1024

static int error_thrown = OSCC_OK;
static bool ctrl_c = false;
static pthread_t udp_server_thread;
 
extern double brake_value;
extern double throttle_value;
extern double steering_value;

void error(char *msg)
{
	perror(msg);
	exit(1);
}

void *udp_server(void *udp_void_ptr)
{
	int sockfd;		/* socket */
	int portno;		/* port to listen on */
	int clientlen;		/* byte size of client's address */
	struct sockaddr_in serveraddr;	/* server's addr */
	struct sockaddr_in clientaddr;	/* client addr */
	struct hostent *hostp;	/* client host info */
	char *buf;		/* message buf */
	char *dest;
	char *str_brake;
	char *str_throttle;
	char *str_steering;
	char *hostaddrp;	/* dotted decimal host addr string */
	int optval;		/* flag value for setsockopt */
	int n;			/* message byte size */

	/* 
	 * check command line arguments 
	 */
	//if (argc != 2) {
	//	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	//	exit(1);
	//}
	//portno = atoi(argv[1]);
	portno = 6565;
	/* 
	 * socket: create the parent socket 
	 */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	/* setsockopt: Handy debugging trick that lets 
	 * us rerun the server immediately after we kill it; 
	 * otherwise we have to wait about 20 secs. 
	 * Eliminates "ERROR on binding: Address already in use" error. 
	 */
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
		   (const void *)&optval, sizeof(int));

	/*
	 * build the server's Internet address
	 */
	bzero((char *)&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short)portno);

	/* 
	 * bind: associate the parent socket with a port 
	 */
	if (bind(sockfd, (struct sockaddr *)&serveraddr,
		 sizeof(serveraddr)) < 0)
		error("ERROR on binding");

	/* 
	 * main loop: wait for a datagram, then echo it
	 */
	clientlen = sizeof(clientaddr);
	while (!ctrl_c) {

		/*
		 * recvfrom: receive a UDP datagram from a client
		 */
		
		buf = malloc(BUFSIZE);
		n = recvfrom(sockfd, buf, BUFSIZE, 0,
			     (struct sockaddr *)&clientaddr, &clientlen);
		if (n < 0)
			error("ERROR in recvfrom");
		dest = malloc(1024);
		// SET VALUES ------
		strncpy(dest, buf, 11);
		//printf("%s", (char*) buf); //atoi(&buf[0])
		//printf("------------");

		str_brake = malloc(512);
		str_throttle = malloc(512);
		str_steering = malloc(512);
		strncpy(str_brake, dest, 3);
		strncpy(str_throttle, dest + 4, 3);
		strncpy(str_steering, dest + 8, 3);
		
		//printf("%s", (char*) dest);
		brake_value = atof(str_brake);
		throttle_value = atof(str_throttle);
		steering_value = atof(str_steering)-1.0;
                
		printf("%f %f %f", atof(str_brake), atof(str_throttle), atof(str_steering)-1.0);
		printf("\n");
		//printf("------");

		/* 
		 * gethostbyaddr: determine who sent the datagram
		 */
/*
		hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
				      sizeof(clientaddr.sin_addr.s_addr),
				      AF_INET);
		if (hostp == NULL)
			error("ERROR on gethostbyaddr");
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL)
			error("ERROR on inet_ntoa\n");
		/*printf("server received %d bytes\n", n); */

		/* 
		 * sendto: echo the input back to the client 
		 */
/*
		n = sendto(sockfd, buf, n, 0,
			   (struct sockaddr *)&clientaddr, clientlen);
		if (n < 0)
			error("ERROR in sendto");
*/
	}
}
static unsigned long long get_timestamp_micro( )
{
    struct timeval time;

    gettimeofday( &time, NULL );

    return ( time.tv_usec );
}

static unsigned long long get_elapsed_time( unsigned long long timestamp )
{
    unsigned long long now = get_timestamp_micro( );
    unsigned long long elapsed_time = now - timestamp;

    return elapsed_time;
}

void signal_handler( int signal_number )
{
    if ( signal_number == SIGINT )
    {
        error_thrown = OSCC_ERROR;
		ctrl_c = true;
		pthread_join(udp_server_thread, NULL);
    }
}

int main( int argc, char **argv )
{

	if(pthread_create(&udp_server_thread, NULL, udp_server, NULL))
    {
		return 0;
	}
	
	//if(pthread_join(udp_server_thread, NULL))
    //{
	//	return 0;
    //}

    oscc_result_t ret = OSCC_OK;
    unsigned long long update_timestamp = get_timestamp_micro();
    unsigned long long elapsed_time = 0;

    int channel;

    errno = 0;

    if ( argc != 2 || ( channel = atoi( argv[1] ), errno ) != 0 )
    {
        printf( "usage %s channel\n", argv[0] );
        exit( 1 );
    }

    struct sigaction sig;
    sig.sa_handler = signal_handler;
    sigaction( SIGINT, &sig, NULL );
    ret = commander_init( channel );
	
    if ( ret == OSCC_OK )
    {
        printf( "\nControl Ready:\n" );
        printf( "    START - Enable controls\n" );
        printf( "    BACK - Disable controls\n" );
        printf( "    LEFT TRIGGER - Brake\n" );
        printf( "    RIGHT TRIGGER - Throttle\n" );
        printf( "    LEFT STICK - Steering\n" );

        while ( ret == OSCC_OK && error_thrown == OSCC_OK )
        {
            elapsed_time = get_elapsed_time( update_timestamp );

            if ( elapsed_time > COMMANDER_UPDATE_INTERVAL_MICRO )
            {
                update_timestamp = get_timestamp_micro();

                ret = check_for_controller_update();
            }

            // Delay 1 ms to avoid loading the CPU
            (void) usleep( SLEEP_TICK_INTERVAL_MICRO );
        }
        commander_close( channel );
    }

    return 0;
}

