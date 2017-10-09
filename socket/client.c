#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLEN 1024                                              // Maximum message length

typedef struct message                                           // message structure
{
        uint8_t msg_type;                                        // message type
        uint16_t msg_len;                                        // message length
        char message[MAXLEN];                                    // message body
} msg;

int main ( int argc, char **argv )
{

        if ( argc != 3 )                                          // wrong format of arguments
        {
                perror (" Unknown input format. \n");
                perror (" Please execute in format: <executable code> <Server IP Address> <Server Port number> \n");
                exit (1);
        }

        int server_port = atoi( argv[2] );                         // port number
        int udp_port, socket_fd;
        struct sockaddr_in serv_addr;                              // sin_family/port/addr/zero[8];  internet specific addr struct
        msg msg_buffer;                                            // message
        socklen_t serverlen;                                       // size of address
        socket_fd = socket ( AF_INET, SOCK_STREAM, 0 );            // socket(family, type, protocol) returns success

        if ( socket_fd < 0 )                                        // error in creation
        {
                perror (" ERROR in creation of TCP socket \n");
                exit(2);
        }

        memset ( &serv_addr, 0, sizeof(serv_addr) );                 // intializing serv_addr
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
        serv_addr.sin_port = htons(server_port);

        // connecting and if error arises it will exit with status 3
        if ( connect ( socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr) ) < 0 )
        {
                perror ("Problem in extablishing connection to Server! \n");
                exit(3);
        }

        printf ("Server IP ADDR \t TCP/UDP \t Server Port \t INFO \n");
        printf("---------------------------------------------------------------------------------------------------\n");

        msg_buffer.msg_type = 1;            // creating message
        msg_buffer.msg_len = 0;

        send ( socket_fd, &msg_buffer, sizeof(msg_buffer), 0 );
        printf("%s \t TCP \t\t %d \t\tSended Message Type:\t  %d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port), msg_buffer.msg_type);

        if ( recv(socket_fd, &msg_buffer, sizeof(msg_buffer), 0 ) == 0)         //no message recieved
        {
            perror("Server terminated prematurely \n");
            exit(4);
        }

        // recieved message
        printf("%s \t TCP \t\t %d \t\tReceived Message Type: %d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port), msg_buffer.msg_type);

        udp_port = msg_buffer.msg_len;
        printf("%s \t TCP \t\t %d \t\tUDP Port Received for connection: %d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port), msg_buffer.msg_len);

        // closing TCP connectionb
        printf("%s \t ---\t\t %d \t\tTCP Connection Closed:\t  %d (0 implies success)\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port), close(socket_fd));

        // creating UDP socket
        if ((socket_fd = socket ( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )
        {
    	           perror("Unable to create UDP socket\n");
    	           exit(2);
        }

        serverlen = sizeof(serv_addr);
        serv_addr.sin_port = htons( udp_port );
        msg_buffer.msg_type=3;

        printf("%s \t UDP \t\t %d \t\tSending Message using UDP:\t  ", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
        fgets( msg_buffer.message, sizeof(msg_buffer.message), stdin );

        msg_buffer.msg_len = strlen ( msg_buffer.message );
        msg_buffer.message[msg_buffer.msg_len-1]='\0';

        sendto(socket_fd, &msg_buffer, sizeof(msg_buffer), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr) );
        printf("%s \t UDP \t\t %d \t\tSent Message Type:\t  %d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port), msg_buffer.msg_type);

        recvfrom( socket_fd, &msg_buffer, sizeof(msg_buffer), 0, (struct sockaddr *) &serv_addr, &serverlen );
        msg_buffer.message[msg_buffer.msg_len]='\0';

        printf("%s \t UDP \t\t %d \t\tReceived MessageType: %d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port), msg_buffer.msg_type );
        printf("%s \t UDP \t\t %d \t\tAcknowledgement Received: Success\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port) );

        printf("%s \t --- \t\t %d \t\tTerminating UDP Connection:  Success\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port) );


        return 0;
}
