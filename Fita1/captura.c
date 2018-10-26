
 /*!
    \file "FITA 1 - ADSTR"
    \brief "rt 1: Sistema de captura de la temperatura, control del sistema i registre de les
    alarmes."
    \author "Lluis Farnes \n e-mail:1393274@campus.euss.org"
    \date "26"/"10"/"2018"
 */

#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <getopt.h>
#include <string.h>

/*! comunicacio important a B115200  */
#define BAUDRATE B115200  //IMPORTANTE QUE SEA 115200
/*! activar fila dev/ttySO quan es vulgui comunicar amb Raspberry  */
//#define MODEMDEVICE "/dev/ttyS0"        //Conexió IGEP - Arduino
/*! activar fila dev/ttyACM0 quan es vulgui comunicar amb Raspberry  */
#define MODEMDEVICE "/dev/ttyACM0"         //Conexió directa PC(Linux) - Arduino
#define _POSIX_SOURCE 1 /* POSIX compliant source */

/* VARIABLES GLOBALS*/

//variables sql

	/**
	@brief Variable que determina el missatge d'error del missatge SQL.
	@brief Variable que determina si el missatge sql es OK.

 */
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char *sql;
	char *query = NULL;


	sqlite3_stmt *stmt;

	/**
	*@brief Variable conta els minuts que esta ences el ventilador
	*@brief Variable que mostra l'estat del ventilador (0:apagat,1:posem funcionament, 2: continuem en funcionament)
	*@brief Variable amb la temperatura de consigna
	*@brief Variable amb la temperatura actual
	*@brief Variable funciona de flag per demanar cada min dades a arduino
	*@brief Variable funciona de flag per encendre o apagar ventilador cada min dades a arduino
 */
 
int alarma=0;
int vent=0;
int Temperatura_regulacio=10;
int Temperatura_actual;
int x;

/*-----------------------*/

struct termios oldtio,newtio;

/**
 * @brief Funció callback que es crida quan sexecuta comando exec del sql
 */
static int callbacksql(void *NotUsed, int argc, char **argv, char **azColName) {
   int i;
   for(i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

/**
 * @brief Funció callback que crida el timer
 */
void callback(union sigval si)
{
    char * msg = (char *) si.sival_ptr;
	printf("----------------------------\n");
	x=1;
	printf("X: %d\n",x);
    printf("%s\n",msg);
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
/**
 * @brief Funció per configurar el timer
 */
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

/*----------------------------COMUNICACIONES CON ARDUINO-------------------------------*/
/**
 * @brief Funció per configurar la comunicacio serie
 */

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

/**
 * @brief Funció per envir missatges a l'arduino
 * @param
 * @param
 * @param
 */
void enviar(char *missatge, int res, int fd)
{

	res = write(fd,missatge,strlen(missatge));

	if (res <0) {tcsetattr(fd,TCSANOW,&oldtio); perror(MODEMDEVICE); exit(-1); }

}


/**
 * @brief Funció per poder rebere missatges de l'arduino
 * @param
 * @param
 * @param
 */
void rebre (char *buf, int fd)
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

//Funció per tancar la comunicació sèrie
void TancarSerie(int fd)
{
	tcsetattr(fd,TCSANOW,&oldtio);
	close(fd);
}
/*-------------------------------------------------------------------------------*/
/**
 * @brief Funcio per comparar la temperatura amb la temperatura de regulacio i pujar dades a la base de dades
 * @param Temperatura actual, donada per el arduino
 * @param Temperatura de regulacio a 10 graus
 */
 int regulacio_Temp (int T,int Treg){
			/**
	@brief Emmagatzema data i hora de p_data.
 */
	char fecha[80];
			/**
	@brief Temps que el ventilador esta activat i es posa a la taula ALARMA de SQL.
 */
	static int tmpsalarma=0;
		/**
	@brief Variable on es concatena el missatge a posar a la base dades.
 */
	char sentencia[100];
        //Definim una variable de tipus time_t
	time_t temps;

        //Capturem el temps amb la funcio time(time_t *t);
	temps = time(NULL);
        //El valor de retorn es a una variable de tipus timei_t, on posaràl temps en segons des de 1970-01-01 00:00:00 +0000 (UTC)

	// struct tm {
	//     int tm_sec;         /* seconds */
	//     int tm_min;         /* minutes */
	//     int tm_hour;        /* hours */
	//     int tm_mday;        /* day of the month */
	//     int tm_mon;         /* month */
	//     int tm_year;        /* year */
	//     int tm_wday;        /* day of the week */
	//     int tm_yday;        /* day in the year */
	//     int tm_isdst;       /* daylight saving time */
	//};

	// Defineix punter a una estructura tm
        struct tm * p_data;

	int y;
	/*CONTROL VARIABLE TAULA TEMPERATURA + ESTAT VENTILADOR*/
	//T=temp. actual ha d'estar ja processada
	//T=temperatura de regulacio
	//vent= Estat del ventilador (0:apagat,1:posem funcionament, 2: continuem en funcionament)
	//alarma= Flag per saltar alarma

	if (T>=Treg && vent==0) { // comprovem si s'ha d'encendre el ventilador
							//Si el ventilador no estava ences posem variable ventilador a 1
		//printf("Encendre ventilador\n");/*<---------ENVIAR ORDRE ARDUINO ENCENDRE VENT*/
		//encendre_ventilador();
		vent=1;
		y=2;
		alarma=0; //resetejem contador minuts ventilador
		}
	else if(T>=Treg && vent==1){
		vent=2;
		alarma++;
		}
	else if (T>=Treg && vent==2){
		alarma++;
		}
	else if (T<Treg && (vent==2||vent==1)){
		printf("Apagar ventilador\n");/*<---------ENVIAR ORDRE ARDUINO APAGAR VENT*/
		tmpsalarma=0;
		printf("temps alarma%d:\n",tmpsalarma);
		//apagar_ventilador();
		y=1;
		vent=0;
	}
	else {vent=0;}

	//printf("Taula temperatura\n"); /*<---------ESCRIURE TAULA TEMPERATURES SQL*/
	printf("Temp: %d\n",T);
	printf("Estat vent: %d\n",vent);
/*---------------------------------------------------------------------------------------------*/
	//Funcion localtime() per traduir segons UTC a la hora:minuts:segons de la hora local
	//struct tm *localtime(const time_t *timep);
    p_data = localtime( &temps );

	strftime(fecha, 80,"%d/%m/%Y %H:%M:%S",p_data);

	printf("p_data: '%s'\n ", fecha);

	sprintf(sentencia, "insert into TEMPERATURA (DATA,TEMPERATURA, VENT) values ('%s',%d,%d); ",fecha,T, vent);         /* 1 */

	rc = sqlite3_exec(db, sentencia, callbacksql, 0, &zErrMsg);

	   if( rc != SQLITE_OK ){
	   fprintf(stderr, "SQL error: %s\n", zErrMsg);
		  sqlite3_free(zErrMsg);
	   } else {
		  fprintf(stdout, "Datos subidos tabla TEMPERATURA correctamente\n");
	   }


/*--------------------------------------------------------------------------------------------*/

/*CONTROL VARIABLE TAULA ALARMES */

	if (alarma >= 5){

		tmpsalarma=tmpsalarma+5;
		printf("temps alarma%d:\n",tmpsalarma);

		printf("!! Salta alarma -> Taula alarmes\n"); /*<---------ESCRIURE TAULA ALARMES SQL*/

		//Funcion localtime() per traduir segons UTC a la hora:minuts:segons de la hora local
		//struct tm *localtime(const time_t *timep);
		p_data = localtime( &temps );

		strftime(fecha, 80,"%d/%m/%Y %H:%M:%S",p_data);

		printf("p_data ALARMA: '%s'\n ", fecha);

		sprintf(sentencia, "insert into ALARMES (DATA,TEMPS_ON) values ('%s',%d); ",fecha, tmpsalarma );         /* 1 */

		rc = sqlite3_exec(db, sentencia, callbacksql, 0, &zErrMsg);

	   if( rc != SQLITE_OK ){
	   fprintf(stderr, "SQL error: %s\n", zErrMsg);
		  sqlite3_free(zErrMsg);
	   } else {
		  fprintf(stdout, "Datos subidos tabla ALARMA correctamente\n");
	   }
		alarma=0;
	}

/*--------------------------------------------------------------------------------------------*/
	return y;
}



int main(int argc, char ** argv)
{
	int c;
	char *nom_database=NULL;
	char basedatos[50]="database.db";
	

	/** ADquisición de los parametros de lanzamiento del programa*/
	/** Pendiente agregar opción para la adquisición de la temperatura objetivo a mantener*/

	while ((c = getopt(argc, argv, "d:t:h")) != -1) {
		switch (c) {
		case 'd':
			//printf("%s\n",argv);
			sqlite3_close(db);
			nom_database = optarg;
			sprintf(basedatos,"%s.db",nom_database);
			printf("S'ha cambiat la base de dades\nNova base de dades:\t%s\n",basedatos);

			break;
		case 't':
			Temperatura_regulacio=atoi(optarg);
			printf("S'ha canviat la temperatura de regulacio\nNova temperatura de regulacio:\t%d\n",Temperatura_regulacio);
			break;
		case 'h':
			printf("\nUso: main [opciones] -n [nombre_archivo.db]...\n\n");
			printf("-d [nom] Poner solo el nombre sin (.db) Precisa 'nombre_base_datos.db'\n");
			printf("-t [temperatura regulacion]	Temperatura regulacion . Default = 10º.\n\n");
			exit(1);
		case '?':
			printf("Opció desconeguda prem '-h' per veure l'ajuda.\n");
			break;
		default:
			abort();
		}
	}


	 //Declaració variables funció main
	int i=0, fd,m=1,res=0, lectures=0, pos=0;
	float array[3600];
	int mostres = 0;
	char missatge[255];
	char buf[255];
	memset(buf,'\0',256);
	fd = ConfigurarSerie();
	int y=0;

	/* Open database */
   rc = sqlite3_open(basedatos, &db);

   if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(0);
   } else {
      fprintf(stdout, "Opened database successfully\n");
   }

   /* Create TAULA TEMPERATURA SQL statement */
   sql = "CREATE TABLE IF NOT EXISTS TEMPERATURA("  \
         "DATA 			 DATATIME  NOT NULL," \
         "TEMPERATURA    INT    NOT NULL," \
         "VENT	         INT 	NOT NULL );";

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callbacksql, 0, &zErrMsg);

   if( rc != SQLITE_OK ){
   fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   } else {
      fprintf(stdout, "Table TEMPERATURA created successfully\n");
   }

	/* Create TAULA ALARMES SQL statement */
   sql = "CREATE TABLE IF NOT EXISTS ALARMES("  \
         "DATA 			DATATIME   ," \
         "TEMPS_ON	    INT 	NOT NULL );";

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callbacksql, 0, &zErrMsg);

   if( rc != SQLITE_OK ){
   fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   } else {
      fprintf(stdout, "Table ALARMES created successfully\n");
   }


	sprintf(missatge,"AM1059Z");
	printf("%s\n",missatge);//es verfifica pel terminal el missatge enviat
	enviar(missatge, res, fd);

	printf("mensaje enviado\n");

	rebre(buf, fd);

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
		// Per seguretat s'apaga el ventilador abans de començar
		
		sprintf(missatge,"AS120Z");//s'executen les probes amb el led13
		printf("Apaguem ventilador: %s\n",missatge);//es verfifica pel terminal el missatge enviat
		enviar(missatge, res, fd);
		printf("mensaje enviado\n");
		memset(buf,'\0',256);
		rebre(buf, fd);
		printf("mensaje recibido\n");
		printf("%s\n",buf);
		
		// es configurar timer per demanar la temperatura al arduino cada 60s
		timer_t rutina;
		set_timer(&rutina, 1, 10, callback,(void *) "rutina");

		//bucle per demanar temperatura i posar en base de dades
		while(1){
			memset(buf,'\0',256);//es neteja el buf per poder llegir els valors correctament

		if (x==1){

			//PEDIR TEMPERATURA AL ARDUINO
			sprintf(missatge,"ACZ");//confimació ok
			enviar(missatge,res,fd);
			memset(buf,'\0',256);
			rebre(buf, fd);//comprobació del missatge rebut (només compara que acabi amb la Z)
			printf("%s\n",buf);

			//Calculo de la temperatura
			Temperatura_actual=(buf[3]-48)*100+(buf[4]-48)*10+(buf[5]-48)+(buf[6]-48)*0.1;
			Temperatura_actual= (5* Temperatura_actual *100)/1024 ;

			//printf	("TEMPERATURA CALCULADA = %d \n",Temperatura_actual);

			y = regulacio_Temp (Temperatura_actual,Temperatura_regulacio);/*<---------ES COMPARA T AMB TREG I POSA BASE DADES*/

			if (y==1){ //En cas que la temperatura sobrepassa la temperatura de regulacio--> Encen ventilador
				sprintf(missatge,"AS120Z");//s'executen les probes amb el led13
				printf("Apaguem ventilador: %s\n",missatge);//es verfifica pel terminal el missatge enviat
				enviar(missatge, res, fd);
				printf("mensaje enviado\n");
				memset(buf,'\0',256);
				rebre(buf, fd);
				printf("mensaje recibido\n");
				printf("%s\n",buf);
				y=0;
				}
			else if (y==2){// Si el ventilador estava engegat i la temperatura < temperatura regulacio --> apaga ventilador
				sprintf(missatge,"AS121Z"); // s'executen les probes amb el led13
				printf("Encenem ventilador: %s\n",missatge);//es verfifica pel terminal el missatge enviat
				enviar(missatge, res, fd);
				printf("mensaje enviado\n");
				memset(buf,'\0',256);
				rebre(buf, fd);
				printf("mensaje recibido\n");
				printf("%s\n",buf);
				y=0;
			}
			else{}

			x=0;
		}else{}
		}

	}

	//criedem a la funció per tancar la comunicació sèrie
	TancarSerie(fd);
	sqlite3_close(db);
    return 0;
}
