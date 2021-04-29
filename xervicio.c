#include <string.h>//inet_addr(serv_host_addr)
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>			//Librerias necesarias para el funcionamiento del servicio
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PARAMETROS_PETICION 3

//SE DEFINEN LOS ERRORES POSIBLES CON EL NOMBRE Y SU CODIGO
#define LONG_FILE 1024
#define BAD_REQUEST 400
#define NOT_FOUND 404
#define NOT_IMPLEMENTED 501
#define HTTP_VERSION_NOT_SUPPORTED 505

//SE DEFINEN LAS FUNCIONES
char * directorio();//La funcion retorna el directorio actual del programa servicio.
void error(int codigo);//Recibe por parametro el codigo del error y le retorna al cliente la pagina que le corresponde al error.
void responder(char peticion[]);//Valida la peticion del cliente y la respuesta a la peticion.
char *extension(char archivo[]);//Recibe el recurso que debe enviarse al cliente, retorna un puntero al inicio de la extencion y si el archivo no tiene extencion definida se retorna "".
char *mime(char extencion[]);//Recibe el puntero a la extencion del archivo y retorna el tipo MIME que corresponde a la extencion.
char *dir;

int main(int argc, char ** argv){
	char peticion[1024];
	char buffer[1024];
	int tamanio;
	dir = argv[0];
	
	if((tamanio = read(0, buffer, sizeof(buffer))) > 0){//Copia la informacion en el buffer para generar una respuesta
		buffer[tamanio] = '\0';			
		strcpy(peticion, buffer);
		strcpy(buffer, "");
		responder(peticion);//Se responde a la  peticion
    }	
	return 0;
}

void responder(char peticion[]){
	char metodo[10];//En el se guardara la peticion.
	char recurso[200] = ".";//Se utilizara para obtener el recurso. 
	char protocolo[10];//Se guarda en el el protocolo con la version.
	char cabecera[50];//Version codigo de estado y mensaje asociado al codigo
	char cont_type[50];//Es el tipo de archivo 
	char cont_len[50];//La longitud del archivo a enviar
	char aux[100];//Auxiliar
	char respuesta[LONG_FILE];//se concatena la cabecera, el tipo de archivo, la longitud y el contenido del archivo
	strcpy(respuesta, "");

    fprintf(stdout,"%s",peticion);
    fflush(stdout);

	if(sscanf(peticion, "%s %s %s", metodo, aux, protocolo) != PARAMETROS_PETICION){ //Se comprueba que la peticion cumpla con el formato.
		error(BAD_REQUEST);
		return;
	}
	
	if(strcmp(protocolo, "HTTP/1.0") != 0 && (strcmp(protocolo, "HTTP/1.1") != 0)){//Se comprueba que se realizo con la version 1.0 de HTTP.
		error(HTTP_VERSION_NOT_SUPPORTED);
		return;
	}
	
	if(strcmp(metodo, "GET") != 0 && strcmp(metodo, "HEAD") != 0){//Se comprueba el metodo de peticion
		error(NOT_IMPLEMENTED);
		return;
	}
/*
	strcpy(aux, recurso);
	strcpy(recurso, directorio());//Se agrega al inicio el directorio del servidor para encontrar el recurso
	strcat(recurso, aux);
*/
    strcat(recurso, aux);
    //int fd = open(recurso, O_RDONLY);
    struct stat sb;
    int fd =stat(recurso,&sb);

    if(S_ISDIR(sb.st_mode)){
    	if(recurso[strlen(recurso)-1] == '/'){//Si recurso esta vacio, se retorna la pagina por defecto "index.html".
			strcat(recurso, "index.html");
		} else strcat(recurso, "/index.html");
		fd =stat(recurso,&sb); //volves a revisar el nuevo recurso
    }
    if(fd == -1){//Si no se encontro el archivo
		error(NOT_FOUND);
		return;
    }
//Si no ocurre un error previo se supone que esta todo OK
	//Se crean las cabeceras.
	strcpy(cabecera, "HTTP/1.0 200 OK");
	sprintf(cont_type, "Content-Type: %s", mime(extension(recurso)));
	sprintf(cont_len, "Content-Length: %ld", sb.st_size);
	
	//Se concatena las cabeceras, creando la respuesta
	strcpy(respuesta, cabecera);
	strcat(respuesta, "\n");
	strcat(respuesta, cont_type);
	strcat(respuesta, "\n");
	strcat(respuesta, cont_len);
	strcat(respuesta, "\n\n");
    write(0, respuesta, strlen(respuesta));//Se le envia la cabecera
    if(strcmp(metodo, "GET") == 0){//Si no se pide solo la cabecera
    	fd = open(recurso, O_RDONLY);
        char *p = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        write(0, p, sb.st_size);	
        munmap(p, sb.st_size);      
        close(fd);  
    }

	close(0);
	return;
}

//Tipos MIME
char * mime(char extencion[]){//Segun la extencion del archivo se le agrega el tipo MIME dentro de la cabezera.
	if(strcmp(extencion, "htm") == 0 || strcmp(extencion, "html") == 0) return "text/html";
	if(strcmp(extencion, "txt") == 0 || strcmp(extencion, "c") == 0) return "text/plain";
	if(strcmp(extencion, "css") == 0) return "text/css";
	if(strcmp(extencion, "gif") == 0) return "image/gif";
	if(strcmp(extencion, "png") == 0) return "image/png";
	if(strcmp(extencion, "jpg") == 0 || strcmp(extencion, "jpeg") == 0) return "image/jpeg";
	if(strcmp(extencion, "pdf") == 0) return "application/pdf";
		
	return "application/octet-stream";//Si la extencion es null, retorna "application/octet-stream"
}
char * extension(char archivo[]){
	char *ext = strrchr(archivo, '.');//Se obtiene un puntero a la ocurrencia "."
	if( ext != 0 )
		return ext + 1;
	return "";
}
void error(int codigo){ //Si ocurre algun error segun la peticion dada
	char nombre_archivo[40];
	strcpy(nombre_archivo, "");

	char cabecera[60];
	strcpy(cabecera, "");

	char contenido[LONG_FILE];
	strcpy(contenido, "");

	char respuesta[LONG_FILE];	
	strcpy(respuesta, "");

	FILE *archivo;

	char cont_type[60];
	char cont_len[60];

	switch(codigo){//Se determina cual es el tipo de error, se obtiene el nombre del error y se crea la cabecera.
		case 400:
			strcpy(nombre_archivo, "error/BadRequest.html");
			strcpy(cabecera, "HTTP/1.0 400 Bad Request");
			break; 
		case 404:
			strcpy(nombre_archivo, "error/NotFound.html");
			strcpy(cabecera, "HTTP/1.0 404 Not Found");
			break;
		case 501:
			strcpy(nombre_archivo, "error/NotImplemented.html");
			strcpy(cabecera, "HTTP/1.0 501 Not Implemented");
			break;
		case 505:
			strcpy(nombre_archivo, "error/HTTPVersionNotSupported.html");
			strcpy(cabecera, "HTTP/1.0 505 HTTP Version Not Supported");
			break;
	}

	archivo = fopen(nombre_archivo, "r");//Se abre el archivo html
	
	fread(contenido, sizeof(char), LONG_FILE, archivo);//La funcion lee y guarda
	fclose(archivo);//Se cierra el archivo

	sprintf(cont_type, "Content-Type: %s", "text/html");//Se crea la cabecera con el tipo de documento.
    sprintf(cont_len, "Content-Length: %ld", strlen(contenido));//Se crea la cabecera con el largo
	
						//Se genera la respuesta

	strcpy(respuesta, cabecera); //Se añade la cabecera
	strcat(respuesta, "\n");
	strcat(respuesta, cont_type);//Se añade la extencion MIME
	strcat(respuesta, "\n");
	strcat(respuesta, cont_len);//Se añade el tamaño del recurso
	strcat(respuesta, "\n\n");//Se añade el renglon en blanco que indica que lo siguiente es el contenido 
	strcat(respuesta, contenido);//Se le añade el archivo de respuesta
	
	write(0, respuesta, strlen(respuesta)); //Se envia la respuesta
	close(0);

	return;
}

char *directorio(){ //Identifica y concatena la direccion del recurso solicitado.
	static char direccion[90] = { }; 

	if(getcwd(direccion, sizeof(direccion)) != NULL)	{
		strcat(direccion, dir);//Se concatena la direccion de los archivos en el servidor
		return direccion;  //Se retorna la direccion
	}
	
	return "";
}
