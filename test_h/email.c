//Incluir libreria

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "email.h"
//---------------------

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define REQUEST_MSG_SIZE	1024
#define REPLY_MSG_SIZE		500
#define SERVER_PORT_NUM		25


void enviar_mail( char *From, char *RCPT, char *body_mail){
	struct sockaddr_in	serverAddr;
	char	    serverName[] = "172.20.0.21"; //Adreça IP on està el client
	int			sockAddrSize;
	int			sFd;
	int			mlen;
	int 		result;
	char		buffer[256];
	char		missatge[] = "HELO AE2012\n";
	char		missatge2[80];
	char		missatge3[80];
	char		missatge4[] = "DATA\n";
	char		missatge6[] = "quit\n";

	sprintf(missatge2, "MAIL FROM:%s\n", From);
	sprintf(missatge3, "RCPT TO:%s\n", RCPT);
	


	/*Crear el socket*/
	sFd=socket(AF_INET,SOCK_STREAM,0);

	/*Construir l'adreça*/
	sockAddrSize = sizeof(struct sockaddr_in);
	bzero ((char *)&serverAddr, sockAddrSize); //Posar l'estructura a zero
	serverAddr.sin_family=AF_INET;
	serverAddr.sin_port=htons (SERVER_PORT_NUM);
	serverAddr.sin_addr.s_addr = inet_addr(serverName);

	/*Conexió*/
	result = connect (sFd, (struct sockaddr *) &serverAddr, sockAddrSize);
	if (result < 0)
	{
		printf("Error en establir la connexió\n");
		exit(-1);
	}
	printf("\nConnexió establerta amb el servidor: adreça %s, port %d\n",	inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));


	/*Rebre missatge de benvinguda*/
	result = read(sFd, buffer, 256);
	printf("Missatge rebut del servidor(bytes %d): %s\n",	result, buffer);

	/*Enviar HELO*/
	strcpy(buffer,missatge); //Copiar missatge a buffer
	result = write(sFd, buffer, strlen(buffer));
	printf("Missatge enviat a servidor(bytes %d): %s\n",	result, missatge);

	/*Rebre mercuri.euss.es*/
	result = read(sFd, buffer, 256);
	buffer[result]=0;
	printf("Missatge rebut del servidor(bytes %d): %s\n",	result, buffer);
	
	/*Enviar mail from*/
	strcpy(buffer,missatge2); //Copiar missatge a buffer
	result = write(sFd, buffer, strlen(buffer));
	printf("Missatge enviat a servidor(bytes %d): %s\n",	result, missatge2);

	/*Rebre*/
	result = read(sFd, buffer, 256);
	buffer[result]=0;
	printf("Missatge rebut del servidor(bytes %d): %s\n",	result, buffer);
	
	/*Enviar rcpt*/
	strcpy(buffer,missatge3); //Copiar missatge a buffer
	result = write(sFd, buffer, strlen(buffer));
	printf("Missatge enviat a servidor(bytes %d): %s\n",	result, missatge3);

	/*Rebre*/
	result = read(sFd, buffer, 256);
	buffer[result]=0;
	printf("Missatge rebut del servidor(bytes %d): %s\n",	result, buffer);
	
	/*Enviar data*/
	strcpy(buffer,missatge4); //Copiar missatge a buffer
	result = write(sFd, buffer, strlen(buffer));
	printf("Missatge enviat a servidor(bytes %d): %s\n",	result, missatge4);

	/*Rebre*/
	result = read(sFd, buffer, 256);
	buffer[result]=0;
	printf("Missatge rebut del servidor(bytes %d): %s\n",	result, buffer);
	
	/*Enviar mensaje*/
	strcpy(buffer,body_mail); //Copiar missatge a buffer
	result = write(sFd, buffer, strlen(buffer));
	printf("Missatge enviat a servidor(bytes %d): %s\n",	result, body_mail);

	/*Rebre*/
	result = read(sFd, buffer, 256);
	buffer[result]=0;
	printf("Missatge rebut del servidor(bytes %d): %s\n",	result, buffer);
	
	/*Enviar quit*/
	strcpy(buffer,missatge6); //Copiar missatge a buffer
	result = write(sFd, buffer, strlen(buffer));
	printf("Missatge enviat a servidor(bytes %d): %s\n",	result, missatge6);

	/*Rebre*/
	result = read(sFd, buffer, 256);
	buffer[result]=0;
	printf("Missatge rebut del servidor(bytes %d): %s\n",	result, buffer);

	/*Tancar el socket*/
	close(sFd);

	}
