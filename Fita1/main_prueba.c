/**
  @Author Diego Caraccio
 */

/*!
 * (c) EUSS 2018
 *
 * Author    Diego Caraccio
   * Copyright (C) 2018 Diego Caraccio
   *e-mail: 1393272@campus.euss.org+
   * 
 * Exemple d'utilització dels timers de la biblioteca librt
 * Crea dos timers que es disparen cada segon de forma alternada
 * Cada cop que es disparen imprimeixen per pantalla un missatge
 * 
 * Per compilar: gcc main.c -lrt -lpthread -o main
 */
 
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>                                                        
#include <termios.h>       
#include <sys/ioctl.h> 
#include <time.h>   

#define BAUDRATE B115200                                                
//#define MODEMDEVICE "/dev/ttyS0"        //Conexió IGEP - Arduino
#define MODEMDEVICE "/dev/ttyACM0"         //Conexió directa PC(Linux) - Arduino (ACM0) Arduchino (USB0)                                   
#define _POSIX_SOURCE 1 /* POSIX compliant source */   

/* VARIABLES GLOBALS*/
	/**
	@brief Variable conta els minuts que esta ences el ventilador
 */
int alarma=0;
	/**
 *@brief Variable que mostra l'estat del ventilador (0:apagat,1:posem funcionament, 2: continuem en funcionament)
 */
int vent=0;
	/**
 * @brief Variable amb la temperatura de consigna
 */
int Temperatura_regulacio=25;
	/**
 * @brief Variable amb la temperatura actual
 */
int Temperatura_actual;
	/**
 * @brief Variable funciona de flag per donar alarma: el ventilador porta 5 min consecutius funcionant
 */
int x;

struct termios oldtio,newtio;  

//Declaració variables funció main                                                                   
	int i=0, fd, res=0,m=1, lectures=0, pos=0;                                                     
	int temps; 
	float array[3600];
	float graus=0, maxim=0, minim=99;
	int mostres = 0;
	int comp=0;
	char buf[255];
	char missatge[255];
	int comparacio=0;

//configuració de la comunicació sèrie
int	ConfigurarSerie(void)
{
	int fd;                                                           


	fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY );                             
	if (fd <0) {perror(MODEMDEVICE); exit(-1); }                            

	tcgetattr(fd,&oldtio); /* save current port settings */                 

	bzero(&newtio, sizeof(newtio));                                         
	//newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;             
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;             
	newtio.c_iflag = IGNPAR;                                                
	newtio.c_oflag = 0;                                                     

	/* set input mode (non-canonical, no echo,...) */                       
	newtio.c_lflag = 0;                                                     

	newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */         
	newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */ 

	tcflush(fd, TCIFLUSH);                                                  
	tcsetattr(fd,TCSANOW,&newtio);
	
		
 	sleep(3); //Per donar temps a que l'Arduino es recuperi del RESET
		
	return fd;
}  

//Funció per tancar la comunicació sèrie
void TancarSerie(int fd)
{
	tcsetattr(fd,TCSANOW,&oldtio);
	close(fd);
}

//Función para envirar mensajes al arduino              
void enviar(char *missatge, int res, int fd)
{                                                       
	
	res = write(fd,missatge,strlen(missatge));

	if (res <0) {tcsetattr(fd,TCSANOW,&oldtio); perror(MODEMDEVICE); exit(-1); }
	
}

 //Funció per poder rebere missatges de l'arduino 
void rebre (char *buf, int fd, int temps)
{
	int res=0;
	int j=0;
	int bytes=0; 
	                                      
	
	//Mirem quans bytes ens ha enviat l'Arduino
  	ioctl(fd, FIONREAD, &bytes);
	
	res=0;
	//Rebem
	j=0;
	do	
	{
		res = res + read(fd,&buf[j],1);
		j++;
		
	} 
	while (buf[j-1] != 'Z');//Llegeix fins trobar la Z fi de trama
	
	
		
}


int main(int argc, char **argv)                                                               
{   
    
	memset(buf,'\0',256);
	fd = ConfigurarSerie();
	
	// Enviar el missatge 1, possada en marxa
	printf("Inserta el temps de mostreig\n");
	scanf("%i",&temps);
	printf("Inserta el número de mostres per fer la mitjana\n");
	scanf("%i",&mostres);
	sprintf(missatge,"AM1%02i%iZ",temps, mostres);

	printf("%s\n",missatge);//es verfifica pel terminal el missatge enviat
	enviar(missatge, res, fd);
	
	printf("mensaje enviado\n");
	
	rebre(buf, fd, temps);
	
	printf("mensaje recibido\n");
	
	
	for (i = 0; i < res; i++)
	{
		printf("%c",missatge[i]);
	}
	printf("\n");
	printf("%s\n",buf);	//es verifica que s'ha rebut el missatge correcte de l'arduino

//es comproba que el missatge rebut ha estat el correcte 'AMOZ'	
if (strncmp(buf,"AM0Z",4)==0)
{
	comparacio=0+temps;
	graus=0;	
	comp=0;
	
	//es realitza un do/while per fer un bucle que llegeixi dades constantment
	do
	{
		memset(buf,'\0',256);//es neteja el buf per poder llegir els valors correctament
		
		//cada vegada que comp sigui igual al temps establert per la consola es farà la comunicació de missatges
		if (comp==comparacio+temps)
		{
			//S'envien i es reben els diferetns missatges amb l'arduino
			
			sprintf(missatge,"ACZ");//confimació ok
			enviar(missatge,res,fd);
			memset(buf,'\0',256);
			rebre(buf, fd, temps);
			printf("%s\n",buf);//comprobació del missatge rebut
			comparacio=comp;
			graus=((buf[5]-'0')*10+(buf[6]-'0')+(buf[7]-'0')*0.1+(buf[8 	]-'0')*0.01);//es tradueixen els graus del missatge (string) a float
			printf("GRAUS: %.02f\n",graus);//temrpeatura amb deciamals per pantalla
			memset(buf,'\0',256);
			
			lectures++; //variable per saber el nombre de lectures fetes
			pos++; //posició de l'array circular
		}
			//si la posició de l'array circular arriba a 3600, es torna a 0
			if (pos==3600)
			{
				printf("array:%f",array[i]);
			pos=0;	
			}
			array[pos]=graus; //es guarda la temperatura a l'arrray circular 3600
			
		//comparem les adqusicions per guardar els màxims i mínims de temperatura
		/*if (maxim < graus)
			{
				maxim=graus;
				printf("El màxim és %f\n",maxim);
			}
			if (minim >graus)
			{
				minim=graus;
				printf("El mínim és %f\n",minim);
			}*/

			sleep(1);
			comp++; //s'incrementa la varibale comp cada segon(sleeep(1))->comptador
			//printf("comp: %i\n",comp); BORRAR
			
	} while (m==1);
	
}		 
	//criedem a la funció per tancar la comunicació sèrie                                                         
	TancarSerie(fd);
	
	return 0;
}


/*-------------------------------------------------------------------------------------------*/

void regulacio_Temp (int T,int Treg){
	
	
	/*CONTROL VARIABLE TAULA TEMPERATURA + ESTAT VENTILADOR*/
	//T=temp. actual ha d'estar ja processada 
	//T=temperatura de regulacio
	//vent= Estat del ventilador (0:apagat,1:posem funcionament, 2: continuem en funcionament)
	//alarma= Flag per saltar alarma 
	
	if (T>Treg && vent==0) { // comprovem si s'ha d'encendre el ventilador		
							//Si el ventilador no estava ences posem variable ventilador a 1 
		printf("Encendre ventilador\n");
		
		sprintf(missatge,"AS131Z");//missatge per encendre led13
		enviar(missatge, res, fd);
	
		printf("mensaje enviado\n");
	
		rebre(buf, fd, temps);
	
		printf("mensaje recibido\n");/*<---------ENVIAR ORDRE ARDUINO ENCENDRE VENT*/
		
		vent=1;
		alarma=0; //resetejem contador minuts ventilador
		}
	else if(T>Treg && vent==1){
		vent=2;
		alarma++;	
		}
	else if (T>Treg && vent==2){
		alarma++;
		}
	else if (T<Treg && (vent==2||vent==1)){
		printf("Apagar ventilador\n");
		
		sprintf(missatge,"AS130Z");//misatge per apagar el led13
		enviar(missatge, res, fd);
	
		printf("mensaje enviado\n");
	
		rebre(buf, fd, temps);
	
		printf("mensaje recibido\n");/*<---------ENVIAR ORDRE ARDUINO APAGAR VENT*/
		vent=0;
	}
	else {vent=0;}

	printf("Taula temperatura\n"); /*<---------ESCRIURE TAULA TEMPERATURES SQL*/
	printf("Temp: %d\n",T);
	printf("Estat vent: %d\n",vent);
/*--------------------------------------------------------------------------------------------*/

/*CONTROL VARIABLE TAULA ALARMES */

	if (alarma == 5){
		printf("!! Salta alarma -> Taula alarmes\n"); /*<---------ESCRIURE TAULA ALARMES SQL*/
		alarma=0;
	}

/*--------------------------------------------------------------------------------------------*/
	
}

typedef void (timer_callback) (union sigval);

/* Funció set_timer
 * 
 * Crear un timer
 * 
 * Paràmetres:
 * timer_id: punter a una estructura de tipus timer_t
 * delay: retard disparament timer (segons)
 * interval: periode disparament timer  (segons)
 * func: funció que s'executarà al disparar el timer
 * data: informació que es passarà a la funció func
 * 
 * */
int set_timer(timer_t * timer_id, float delay, float interval, timer_callback * func, void * data) 
{
    int status =0;
    struct itimerspec ts;
    struct sigevent se;

    se.sigev_notify = SIGEV_THREAD;
    se.sigev_value.sival_ptr = data;
    se.sigev_notify_function = func;
    se.sigev_notify_attributes = NULL;

    status = timer_create(CLOCK_REALTIME, &se, timer_id);

    ts.it_value.tv_sec = abs(delay);
    ts.it_value.tv_nsec = (delay-abs(delay)) * 1e09;
    ts.it_interval.tv_sec = abs(interval);
    ts.it_interval.tv_nsec = (interval-abs(interval)) * 1e09;

    status = timer_settime(*timer_id, 0, &ts, 0);
    return 0;
}

void callback(union sigval si)
{
    char * msg = (char *) si.sival_ptr;
	printf("Executa rutina\n");
	x=1;
	printf("X: %d\n",x);
    printf("%s\n",msg);
}

int main2(int argc, char ** argv)
{

    timer_t rutina;
 
    set_timer(&rutina, 1, 10, callback,(void *) "rutina"  );
    //getchar();
	while(1){
	if (x==1){
		printf("Escriu temperatura actual: ");
		scanf("%d/n",&Temperatura_actual);
		regulacio_Temp (Temperatura_actual,Temperatura_regulacio);/*<---------ES COMPARA T AMB TREG I POSA BASE DADES*/
		x=0;
	}else{}
	}
	
		
    return 0;
}
