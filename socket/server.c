#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAXLEN 1024                                              // Maximum message length
#define LISTEN_QUE 8                                             // maximum client connections to handle

typedef struct message                                           // message structure
{
        uint8_t msg_type;                                            // message type
        uint16_t msg_len;                                         // message length
        char message[MAXLEN];                                    // message body
} msg;


int main ( int argc, char **argv )
{

        if ( argc != 2 )                                          // wrong format of arguments
        {
                perror (" Unknown input format. \n");
                perror (" Please execute in format: <executable code> <Server Port number> \n");
                exit (1);
        }

        int server_port = atoi( argv[1] );                         // port number
        int connection_fd, listen_fd;
        int conn_no = 0;
        char buffer[MAXLEN];
        msg msg_buffer;


        socklen_t client_len;
        pid_t child_pid;
                                                                    // taking sockaddr_in to cast later to sockaddr
        struct sockaddr_in client_addr, serv_addr;                  // sin_family/port/addr/zero[8];  internet specific addr struct
        listen_fd = socket ( AF_INET, SOCK_STREAM, 0 );

        if ( listen_fd < 0 )                                        // error in creation
        {
                perror (" ERROR in creation of TCP socket \n");
                exit(2);
        }

        serv_addr.sin_family = AF_INET;                              // intializing serv_addr
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(server_port);

        int flag = bind ( listen_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr) );       //Binding to socket
        if ( flag < 0 )
        {
                perror ("Problem in Binding to TCP socket! \n");
                exit(3);
        }

        flag = listen ( listen_fd, LISTEN_QUE );
        if ( flag < 0 )
        {
                perror("Problem in listening on TCP socket\n");
                exit(4);
        }

        printf("Server UP and running ...waiting for connections. \n");
        printf("Client NUM \t Child_PID \t Client IP ADDR \t TCP/UDP \t Client Port \t INFO \n");
        printf("--------------------------------------------------------------------------------------------------------------------\n");

        while ( 1 )
        {
                client_len = sizeof( client_addr );
                connection_fd = accept ( listen_fd, (struct sockaddr *) &client_addr, &client_len );
                conn_no += 1;

                if ( (child_pid = fork ()) == 0 )               // Creating Child process
                {                                               // forking

                    close ( listen_fd );
                    flag = recv( connection_fd, &msg_buffer, MAXLEN, 0 ) ;
                    if( flag == 0)
                    {
                            perror("The Client connection terminated prematurely!\n");
                            exit(5);
                    }

                    printf("%d \t\t %d \t\t %s \t\t TCP \t\t %d \t\t  Received Message Type: %d \n", conn_no, getpid(), inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), msg_buffer.msg_type );

                    serv_addr.sin_port = htons(0);
                    listen_fd = socket ( AF_INET, SOCK_DGRAM, 0 );
        	    if ( listen_fd < 0)
        	    {
        		    perror("Problem in creation of UDP socket \n");
        		    exit(2);
        	    }

                    flag = bind ( listen_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr) );
        	    if ( flag < 0 )
        	    {
        		    perror("Problem in binding of UDP socket \n");
        		    exit(3);
        	    }

                    struct sockaddr_in local_address;
        	    socklen_t addr_length = sizeof (local_address);
        	    getsockname ( listen_fd, (struct sockaddr*) &local_address, &addr_length );
                    printf("%d \t\t %d \t\t %s \t\t --- \t\t %d \t\t  UDP Port Given:\t  %d \n", conn_no, getpid(), inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), (int) ntohs(local_address.sin_port) );

                    msg_buffer.msg_type = 2;
                    msg_buffer.msg_len = ntohs(local_address.sin_port );                //network to host short
                    send(connection_fd, &msg_buffer, sizeof(msg_buffer), 0 );

        	    printf("%d \t\t %d \t\t %s \t\t TCP \t\t %d \t\t  Sent Message of Type:\t  %d\n", conn_no, getpid(), inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), msg_buffer.msg_type );
                    printf("%d \t\t %d \t\t %s \t\t --- \t\t %d \t\t  TCP Connection Closed:\t  %d\n", conn_no, getpid(), inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), close(connection_fd) );

                    recvfrom(listen_fd, &msg_buffer, sizeof(msg_buffer), 0, (struct sockaddr *)&client_addr, &client_len);
                    msg_buffer.message[msg_buffer.msg_len] = '\0';

                    printf("%d \t\t %d \t\t %s \t\t UDP \t\t %d \t\t  Received Message of Type: %d\n", conn_no, getpid(), inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), msg_buffer.msg_type);
        	    printf("%d \t\t %d \t\t %s \t\t UDP \t\t %d \t\t  Received Message:\t  %s \n", conn_no, getpid(), inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), msg_buffer.message);

        	    msg_buffer.msg_type=4;
        	    sendto(listen_fd, &msg_buffer, sizeof(msg_buffer), 0, (struct sockaddr *)&client_addr, client_len );

                    printf("%d \t\t %d \t\t %s \t\t UDP \t\t %d \t\t  Sent Message of Type:\t  %d\n", conn_no, getpid(), inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), msg_buffer.msg_type);
        	    printf("%d \t\t %d \t\t %s \t\t --- \t\t %d \t\t  Child Terminated:\t  Success\n", conn_no, getpid(), inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                    fflush(stdout);

                    return 0;

                }
                close(connection_fd);

            }

            close(listen_fd);
            return 0;
}
