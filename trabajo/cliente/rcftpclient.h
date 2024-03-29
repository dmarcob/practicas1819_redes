/****************************************************************************/
/* Cabeceras de funciones ya implementadas del cliente (rcftpclient)        */
/* $Revision: 1.5 $ */
/****************************************************************************/

/**
 * Lee argumentos en línea de comandos
 *
 * @param[in] argc Número de argumentos
 * @param[in] argv Lista de argumentos
 * @param[out] verb Flag de verbose
 * @param[out] alg Algoritmo a usar en el cliente
 * @param[out] window Tamaño de la ventana de emisión
 * @param[out] ttrans Tiempo de transmisión a simular
 * @param[out] timeout Tiempo de expiración a simular
 * @param[out] dest String con la dirección de destino
 * @param[out] port String con el servicio/número de puerto
 */
void initargs(int argc, char **argv, char *verb, int* alg, unsigned int* window, unsigned long* ttrans, unsigned long* timeout, char** dest, char** port);


/**
 * Lee de la entrada estándar un número específico de bytes y los guarda en un buffer.
 * Además de realizar la llamada read, realiza comprobaciones, muestra mensajes y toma datos
 * para posteriormente calcular la velocidad efectiva conseguida.
 *
 * @param[out] buffer Dirección a partir de la cual almacenar los datos leídos
 * @param[in] maxlen Número de bytes a leer
 * @return Número de bytes leídos
 */
int readtobuffer(char * buffer, int maxlen);


/**
 * Muestra info y calcula el tiempo transcurrido desde horainicio y la velocidad efectiva conseguida
 *
 * @param[in] horainicio Fecha/hora obtenida antes de ejecutar el algoritmo de envío
 */
void muestrainforesumen(struct timeval horainicio);


/**
 * Imprime el otro extremo del socket
 *
 * @param[in] caddr Dirección de memoria donde guardamos la dirección de un socket
 */
void print_peer(struct sockaddr_storage * caddr);

/**
 * Imprime un resumen del uso del programa
 * @param[in] progname Nombre del comando ejecutado
 */
void printuso(char *progname);
	
	
	//AÑADIDO
/**
 * Envia mensaje con protocolo rcftp al servidor
 * @param[in] sendbuffer Protocolo rcftp enviado
 * @param[out] servinfo Registro que contiene la direccion donde enviar el mensaje
 */

void enviamensaje(int sock, struct rcftp_msg* sendbuffer, struct addrinfo* servinfo );

/**
 * Recibe mensaje con protocolo rcftp del servidor
 * @param[in] sendbuffer Protocolo rcftp recibido
 * @param[out] servinfo Registro que contiene la direccion donde recibir el mensaje
 */
int recibemensaje(int sock, struct rcftp_msg* recvbuffer, struct addrinfo* servinfo );

int recibemensaje2(int sock, struct rcftp_msg* recvbuffer, struct addrinfo* servinfo );

/**
 * Verifica version y checksum en mensaje de confirmacion
 * @param[in] recvbuffer Protocolo rcftp recibido
 */

int esmensajevalido(struct rcftp_msg recvbuffer);

/**
 * Verifica flags en mensaje de confirmacion
 * @param[in] sendbuffer Protocolo rcftp enviado
 * @param[out] sendbuffer Protocolo rcftp recibido
 */
int eslarespuestaesperada(struct rcftp_msg recvbuffer,struct rcftp_msg sendbuffer);


int recibemensaje2(int sock, struct rcftp_msg* recvbuffer, struct addrinfo* servinfo) ;


int eslarespuestaesperada2(struct rcftp_msg recvbuffer,struct rcftp_msg sendbuffer,int ultimoMensaje);



