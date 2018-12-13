/****************************************************************************/
/* Plantilla para implementación de funciones del cliente (rcftpclient)     */
/* $Revision: 1.6 $ */
/* Aunque se permite la modificación de cualquier parte del código, se */
/* recomienda modificar solamente este fichero y su fichero de cabeceras asociado. */
/****************************************************************************/

/**************************************************************************/
/* INCLUDES                                                               */
/**************************************************************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include "rcftp.h" // Protocolo RCFTP
#include "rcftpclient.h" // Funciones ya implementadas
#include "multialarm.h" // Gestión de timeouts
#include "vemision.h" // Gestión de ventana de emisión
#include "misfunciones.h"

/**************************************************************************/
/* VARIABLES GLOBALES                                                     */
/**************************************************************************/
#warning HAY QUE PONER EL NOMBRE (Y BORRAR EL WARNING)
// elegir 1 o 2 autores y sustituir "Apellidos, Nombre" manteniendo el formato
char* autores="Autor: Marcuello Baquero, Victor\nAutor: Marco Beisty, Diego"; // un solo autor
//char* autores="Autor: Apellidos, Nombre\nAutor: Apellidos, Nombre" // dos autores

// variable para indicar si mostrar información extra durante la ejecución
// como la mayoría de las funciones necesitaran consultarla, la definimos global
extern char verb;


// variable externa que muestra el número de timeouts vencidos
// Uso: Comparar con otra variable inicializada a 0; si son distintas, tratar un timeout e incrementar en uno la otra variable
extern volatile const int timeouts_vencidos;


/**************************************************************************/
/* Obtiene la estructura de direcciones del servidor */
/**************************************************************************/
struct addrinfo* obtener_struct_direccion(char *dir_servidor, char *servicio, char f_verbose){

	struct addrinfo hints, //estructura hints para especificar la solicitud
					*servinfo; // puntero al addrinfo devuelto
	int status; // indica la finalización correcta o no de la llamada getaddrinfo
	int numdir=1; // contador de estructuras de direcciones en la lista de direcciones de servinfo
	struct addrinfo *direccion; // puntero para recorrer la lista de direcciones de servinfo

	// genera una estructura de dirección con especificaciones de la solicitud
	if (f_verbose) printf("1 - Especificando detalles de la estructura de direcciones a solicitar... \n");
	// sobreescribimos con ceros la estructura para borrar cualquier dato que pueda malinterpretarse
	memset(&hints, 0, sizeof hints); 
   
	if (f_verbose) { printf("\tFamilia de direcciones/protocolos: "); fflush(stdout);}
	hints.ai_family=AF_UNSPEC;// sin especificar: AF_UNSPEC; IPv4: AF_INET; IPv6: AF_INET6; etc.
	if (f_verbose) { 
		switch (hints.ai_family) {
			case AF_UNSPEC: printf("IPv4 e IPv6\n"); break;
			case AF_INET: printf("IPv4)\n"); break;
			case AF_INET6: printf("IPv6)\n"); break;
			default: printf("No IP (%d)\n",hints.ai_family); break;
		}
	}
   
	if (f_verbose) { printf("\tTipo de comunicación: "); fflush(stdout);}
	hints.ai_socktype=SOCK_DGRAM; // especificar tipo de socket                                                
	if (f_verbose) { 
		switch (hints.ai_socktype) {
			case SOCK_STREAM: printf("flujo (TCP)\n"); break;
			case SOCK_DGRAM: printf("datagrama (UDP)\n"); break;
			default: printf("no convencional (%d)\n",hints.ai_socktype); break;
		}
	}
   
	// pone flags específicos dependiendo de si queremos la dirección como cliente o como servidor
	if (dir_servidor!=NULL) {
		// si hemos especificado dir_servidor, es que somos el cliente y vamos a conectarnos con dir_servidor
		if (f_verbose) printf("\tNombre/dirección del equipo: %s\n",dir_servidor); 
	} else {
		// si no hemos especificado, es que vamos a ser el servidor
		if (f_verbose) printf("\tNombre/dirección del equipo: ninguno (seremos el servidor)\n"); 
		hints.ai_flags=AI_PASSIVE; // poner flag para que la IP se rellene con lo necesario para hacer bind
	}
	if (f_verbose) printf("\tServicio/puerto: %s\n",servicio);
	

	// llamada a getaddrinfo para obtener la estructura de direcciones solicitada
	// getaddrinfo pide memoria dinámica al SO, la rellena con la estructura de direcciones, y escribe en servinfo la dirección donde se encuentra dicha estructura
	// la memoria *dinámica* creada dentro de una función NO se destruye al salir de ella. Para liberar esta memoria, usar freeaddrinfo()
	if (f_verbose) { printf("2 - Solicitando la estructura de direcciones con getaddrinfo()... "); fflush(stdout);}
	status = getaddrinfo(dir_servidor,servicio,&hints,&servinfo);
	if (status!=0) {
		fprintf(stderr,"Error en la llamada getaddrinfo: %s\n",gai_strerror(status));
		exit(1);
	} 
	if (f_verbose) { printf("hecho\n"); }


	// imprime la estructura de direcciones devuelta por getaddrinfo()
	if (f_verbose) {
		printf("3 - Analizando estructura de direcciones devuelta... \n");
		direccion=servinfo;
		while (direccion!=NULL) { // bucle que recorre la lista de direcciones
			printf("    Dirección %d:\n",numdir);
			printsockaddr((struct sockaddr_storage*)direccion->ai_addr);
			// "avanzamos" direccion a la siguiente estructura de direccion
			direccion=direccion->ai_next;
			numdir++;
		}
	}

	// devuelve la estructura de direcciones devuelta por getaddrinfo()
	return servinfo;
}


/* copia aquí la función printsockaddr del programa migetaddrinfo */


/**************************************************************************/
/* Imprime una direccion */
/**************************************************************************/
void printsockaddr(struct sockaddr_storage * saddr) {

	struct sockaddr_in  *saddr_ipv4; // puntero a estructura de dirección IPv4
	// el compilador interpretará lo apuntado como estructura de dirección IPv4
	struct sockaddr_in6 *saddr_ipv6; // puntero a estructura de dirección IPv6
	// el compilador interpretará lo apuntado como estructura de dirección IPv6
	void *addr; // puntero a dirección. Como puede ser tipo IPv4 o IPv6 no queremos que el compilador la interprete de alguna forma particular, por eso void
	char ipstr[INET6_ADDRSTRLEN]; // string para la dirección en formato texto
	int port; // para almacenar el número de puerto al analizar estructura devuelta

	if (saddr==NULL) { 
		printf("La dirección está vacía\n");
	} else {
		printf("\tFamilia de direcciones: "); fflush(stdout);
		if (saddr->ss_family == AF_INET6) { //IPv6
			printf("IPv6\n");
			// apuntamos a la estructura con saddr_ipv6 (el typecast evita el warning), así podemos acceder al resto de campos a través de este puntero sin más typecasts
			saddr_ipv6=(struct sockaddr_in6 *)saddr;
			// apuntamos a donde está realmente la dirección dentro de la estructura
			addr = &(saddr_ipv6->sin6_addr);
			// obtenemos el puerto, pasando del formato de red al formato local
			port = ntohs(saddr_ipv6->sin6_port);
		} else if (saddr->ss_family == AF_INET) { //IPv4
			printf("IPv4\n");
			saddr_ipv4 = (struct sockaddr_in *)saddr; ;
			addr = &(saddr_ipv4->sin_addr); ;
			port = ntohs(saddr_ipv4->sin_port);;
		} else {
			fprintf(stderr, "familia desconocida\n");
			exit(1);
		}
		//convierte la dirección ip a string 
		inet_ntop(saddr->ss_family, addr, ipstr, sizeof ipstr);
		printf("\tDirección (interpretada según familia): %s\n", ipstr);
		printf("\tPuerto (formato local): %d \n", port);
	}
}


/**************************************************************************/
/* Configura el socket, devuelve el socket y servinfo */
/**************************************************************************/
int initsocket(struct addrinfo *servinfo, char f_verbose){
	int sock;

	printf("\nSe usará ÚNICAMENTE la primera dirección de la estructura\n");

	//crea un extremo de la comunicación y devuelve un descriptor
	if (f_verbose) { printf("Creando el socket (socket)... "); fflush(stdout); }
	sock = socket(servinfo->ai_family,servinfo->ai_socktype ,servinfo->ai_protocol);
	if (sock < 0) {
		perror("Error en la llamada socket: No se pudo crear el socket");
		/*muestra por pantalla el valor de la cadena suministrada por el programador, dos puntos y un mensaje de error que detalla la causa del error cometido */
		exit(1);
	}
	if (f_verbose) { printf("hecho\n"); }


	return sock;
}




/**************************************************************************/
/*  algoritmo 1 (basico)  */
/**************************************************************************/
void alg_basico(int socket, struct addrinfo *servinfo) {
				
	int ultimoMensaje = 0; //inicializamos ultimo mensaje a false
	int ultimoMensajeConfirmado = 0; //inicializamos ultimo mensaje confirmado a false
	struct rcftp_msg sendbuffer; //mensaje con protocolo rcftp a enviar
	struct rcftp_msg recvbuffer; //mensaje con protocolo rcftp a recibir
	char buffer[RCFTP_BUFLEN]; 
	int leido = readtobuffer(buffer,RCFTP_BUFLEN); //leido = bytes escritos en teclado
	if (leido == 0) { //alcanzado fin de fichero, CTRL+d
		ultimoMensaje = 1;
	}	
	
	//OPCION1-->sendbuffer = construir(buffer,leido);
	//OPCION2-->construirMensajeRCFTP(&sendbuffer,buffer,leido); 
	//OPCION3..>LA ACTUAL, no usar funcion
	if (ultimoMensaje == 1){
		
		sendbuffer.flags=F_FIN;
	}	
	else {
		sendbuffer.flags=F_NOFLAGS;
	}													//ENCAPSULAR EN UNA FUNCION
	//construir el mensaje válido ***********************************
	sendbuffer.version= RCFTP_VERSION_1;
	sendbuffer.numseq=htonl(0);
	sendbuffer.len=htons(leido);
	sendbuffer.next=htonl(0);
	sendbuffer.sum=0;
	strcpy(sendbuffer.buffer,buffer);			//COPIA por desgracia TAMBIÉN EL \n
	sendbuffer.sum=xsum((char*)&sendbuffer,sizeof(sendbuffer));  //char* porque lee byte a byte
	
	                            
	while (ultimoMensajeConfirmado == 0)  {
		//caso en que no ha llegado confirmacion a mensaje enviado con flag FIN marcado	
		fprintf(stderr,"ultimoMensajeConfirmado=%d",ultimoMensajeConfirmado);
		enviamensaje(socket, &sendbuffer, servinfo);  
	
		recibemensaje(socket, &recvbuffer, servinfo); 
			//si no cumple la conicion, envia constantemente la misma peticion hasta que se cumpla la condicion
			if (esmensajevalido(recvbuffer) && eslarespuestaesperada(recvbuffer,sendbuffer)) {
				//caso en que confirmacion es valida
				
				if (ultimoMensaje == 1) {
					//caso en que mensaje enviado estaba marcado con flag FIN y la confirmacion es correcta (tambien tiene marcada flag FIN)
					ultimoMensajeConfirmado = 1;
						
				}
				else {		
					
					//caso en que mensaje enviado no tiene marcado flag FIN
					int leido = readtobuffer(buffer,RCFTP_BUFLEN); //leido = bytes escritos en teclado
					fprintf(stderr,"LEIDO: %d",leido);
					if (leido == 0) {
						//si ha leido 0Bytes, entonces ha alcanzado fin de fichero (posteriormente envio mensaje con flag FIN)
						ultimoMensaje = 1;
						
					}
					if (ultimoMensaje == 1){								//ENCAPSULAR EN UNA FUNCION
						sendbuffer.flags=F_FIN;
					}	
					else {
						sendbuffer.flags=F_NOFLAGS;
					}	
							
					//construir el mensaje válido ***********************************
					sendbuffer.version= RCFTP_VERSION_1;
					sendbuffer.numseq=htonl(ntohl(recvbuffer.next));
					sendbuffer.len=htons(leido);
					sendbuffer.next=htonl(0);
					sendbuffer.sum=0;
					strcpy(sendbuffer.buffer,buffer);			//COPIA por desgracia TAMBIÉN EL \n
					sendbuffer.sum=xsum(&sendbuffer,sizeof(sendbuffer));  
			 
				}
			}
	}
	fprintf(stderr,"fueaaaaa");
}

/**************************************************************************/
/*  algoritmo 2 (stop & wait)  */
/**************************************************************************/
void alg_stopwait(int socket, struct addrinfo *servinfo) {
	/*int timeouts_procesados = 0;
        addtimeout();
        int esperar = 1;
        struct rcftp_msg sendbuffer;
        construirMensajeRCFTP(sendbuffer,vector,leido);
        struct rcftp_msg recvbuffer;
        while(esperar){
            ssize_t numDatosRecibidos = recibirmensaje(socket,&recvbuffer,sizeof(recvbuffer),&remote,sizeof(&remote));
            if(numDatosRecibidos > 0){
                canceltimeout();
                esperar = 0;
            }
            if(timeouts_vencidos != timeouts_procesados){
                esperar = 0;
                timeouts_procesados = timeouts_procesados + 1;
            }
        }
	printf("Comunicación con algoritmo stop&wait\n");
*/

	printf("Comunicación con algoritmo stop&wait\n");

#warning FALTA IMPLEMENTAR EL ALGORITMO STOP-WAIT
	printf("Algoritmo no implementado\n");
}

/**************************************************************************/
/*  algoritmo 3 (ventana deslizante)  */
/**************************************************************************/
void alg_ventana(int socket, struct addrinfo *servinfo,int window) {
	 /*char buffer[RCFTP_BUFLEN];
        int leido;
        struct rcftp_msg sendbuffer, recvbuffer;
        int ultimoMensajeConfirmado = 0;
        while(ultimoMensajeConfirmado != 0){
            /***BLOQUE DE ENVIO : Enviar datos si hay espacio en la ventana***/
           /* int espacioLibreEnVentanaEmision = hayEspacio(window);
            if(espacioLibreEnVentanaEmision && finDeFicheroNoAlcanzado){
                leido = readtobuffer(buffer,RCFTP_BUFLEN);
                construirMensajeRCFTP(
	printf("Comunicación con algoritmo go-back-n\n");
    */

	printf("Comunicación con algoritmo go-back-n\n");

#warning FALTA IMPLEMENTAR EL ALGORITMO GO-BACK-N
	printf("Algoritmo no implementado\n");
}


