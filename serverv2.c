#include <mysql.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <pthread.h>

// DEFINES 
#define TRUE 1
#define FALSE 0

typedef struct 
{
	char nombre[20];
	int socket;
}Tusuario;

typedef struct 
{
	Tusuario usuarios[100];
	int num; //numero de conectados
}TusuariosLista;

typedef struct 
{
	int numJ;
	int contador;
	int socket[100];
}Tpartida;

Tpartida tablaPartidas[100];
TusuariosLista usuarioLista; 
MYSQL *mysql_conn;    
pthread_mutex_t mutex;
int sockets[10];
int i;  

//Funcion para anadir ususarios a la lista sabiendo su nombre y su usuario
//Si no se puede anadir porque ya hay mas de 100 retorna -1
//Si se puede anadir, lo hace y retona 0
int anadirUsusario(char nombre[20], int socket, TusuariosLista *lista)
{ 
	if(lista->num == 100)
		return -1;
	else
	{
		strcpy(lista->usuarios[lista->num].nombre,nombre);
		lista->usuarios[lista->num].socket = socket;
		lista->num = lista->num + 1;
		return 0;
	}
}

//Procedimiento para saber los usuarios de la lista
//
void DameUsuarios(TusuariosLista *lista, char output[100])
{
	//string amb num,noms persones
	int i;
	sprintf(output,"6/%d,",lista->num);
	for (i=0; i < lista->num; i++)
		sprintf(output,"%s%s,",output,lista->usuarios[i].nombre);
}

//Funcion para saber la posicion de un usuario en  la lista sabiendo su nombre 
//Retorna la posicion del usuario en la lista si existe
//Si no existe retorna un -1
int DamePosicion(TusuariosLista *lista, char nombre[20])
{
	int found = 0;
	int i = 0;
	while(i < lista->num && !found)
	{
		if(strcmp(nombre,lista->usuarios[i].nombre)==0)
		{
			found = 1;
			return i;
		}
		i=i+1;
	}
	return -1;
}

//Funcion que nos indica el socket de un usuario sabiendo su nombre
//Si encuntra el usuario en la lista nos devuelve su socket
//Si no puede hacerlo nos devuelve un -1
int DameSocket(TusuariosLista *lista, char nombre[20])
{
	int found = 0;
	int i = 0;
	while(i < lista->num && !found)
	{
		if(strcmp(nombre,lista->usuarios[i].nombre)==0)
		{
			found = 1;
			return lista->usuarios[i].socket;
		}
		i=i+1;
	}
	return -1;
}
		
//Funcion que nos elimina a un jugador de la lista sabiendo su nombre
//Si el usuario existe lo elimina y devuelve un 0
//Si no encuentra ususario y no lo puede eliminar devuelve un -1 
int EliminaConectados(TusuariosLista *lista, char nombre[20])
{
	//borrem de la llista quan es desconecta
	int p = DamePosicion(lista,nombre);
	if (p==-1)
	return -1;
	else 
	{
		int i;
		for(i = p; i<lista->num-1; i++)
		{
			lista->usuarios[i]=lista->usuarios[i+1];
		}
		lista->num = lista->num - 1;
		return 0; 
	}	
}


//Funcion de login
//Consulta que el usuario exista en la base de datos 
//Si el usuario existe devuelve un 2
//Si el usuario no existe devuelve un 1
int login(char usuario[20],char password[20],MYSQL *mysql_conn)
{
	int err;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	char consulta[512];
	//Consultar los datos son correctos
	strcpy(consulta,"SELECT jugador.usuario, jugador.pasword FROM jugador WHERE jugador.usuario='");
	strcat(consulta, usuario);
	strcat(consulta, "' AND jugador.pasword = '");
	strcat(consulta, password);
	strcat(consulta, "'");
	//Detectar errores en la consulta 
	err=mysql_query(mysql_conn,consulta);
	if(err!=0)
	{
		printf("Error al consultar la base de datos %u %s\n",mysql_errno(mysql_conn),mysql_error(mysql_conn));
		exit(1);
	}
	//Conseguir los resultados
	result=mysql_store_result(mysql_conn);
	row=mysql_fetch_row(result);
	if(row==NULL)
	{
		return 1;
	}
	else
	{
		return 2;
	}
}


//Funcion de SignUp
//Si puede anadir al jugador en la base de datos nos devuelve un 2
//Si no puede anadirlo devuelve un 1
int signup(char usuario[512],char password[512],MYSQL *mysql_conn)
{
	int err;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	char consulta1[512];
	
	//Hacemos la primera consulta
	//Obtener el id del ultimo jugador registrado
	strcpy (consulta1,"SELECT MAX(jugador.jugadorID) FROM jugador;");
	err=mysql_query(mysql_conn,consulta1);
	if(err!=0)
	{
		printf("Error al consultar la base de datos %u %s\n",mysql_errno(mysql_conn),mysql_error(mysql_conn));
		exit(1);
	}
	
	//GObtenemos el resulta
	result = mysql_store_result (mysql_conn);
	row = mysql_fetch_row (result);
	//El id del nuevo usuario es el +1
	int id_player= atoi(row[0])+1;
	printf("El id asignado es: %d\n",id_player);
	//Hacemos la segunda consulta (insertar usuario)
	char consulta2[512];
	printf("he hecho la consulta\n");
	sprintf(consulta2,"INSERT INTO jugador(jugadorID,usuario,pasword,veces,puntos) VALUES(%d,'%s','%s',0,0)",id_player,usuario,password);
	//Detectar errores en la consulta 
	err=mysql_query(mysql_conn,consulta2);
	printf("ya he hecho la segunda consulta\n");
	if(err!=0)
	{
		printf("Error al consultar la base de datos %u %s\n",mysql_errno(mysql_conn),mysql_error(mysql_conn));
		exit(1);
		return 1;
	}
	else
	{
		printf("La consulta no me ha dado error \n");
		//Conseguir los resultados
		return 2;
	}
	
}

int DeleteAccount(char usuario[20],char password[20], MYSQL *mysql_conn)
{
	int err;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	char consulta[512];
	sprintf(consulta,"DELETE FROM jugador WHERE usuario = '%s' AND pasword= '%s'  ;",usuario,password);

	//Detectar errores en la consulta 
	err=mysql_query(mysql_conn,consulta);
	if(err!=0)
	{
		printf("Error al consultar la base de datos %u %s\n",mysql_errno(mysql_conn),mysql_error(mysql_conn));
		exit(1);
		return 1;
	}
	else
	{
		return 2;
	}
}

//Funcion que nos indica los puntos totales de un jugador sabiendo su nombre
//Si encuentra el jugador devuelve su puntuacion
//Si no puede hacerlo devuelve un -1
int PuntosTotales(char jugador[20],MYSQL *mysql_conn)
{
	int err;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	//Hcaer la consulta
	char  consulta[512];
	sprintf(consulta,"SELECT SUM(relacion.puntuacion) FROM jugador, partida, relacion WHERE jugador.usuario ='%s' AND jugador.jugadorID = relacion.jugadorID AND relacion.partidaID = partida.partidaID",jugador);
	
	//Detectar errores en la consulta 
	err=mysql_query(mysql_conn,consulta);
	if(err!=0)
	{
		printf(consulta,"Error al consultar la base de datos %u %s\n",mysql_errno(mysql_conn),mysql_error(mysql_conn));
		exit(1);
	}
	//Conseguir los resultados
	result=mysql_store_result(mysql_conn);
	row=mysql_fetch_row(result);
	if(row==NULL)
	{
		return -1;
	}
	else
	{
		int result=atoi(row[0]);
		return result;
	}
}


//Funcion que nos indica le tiempo total de juego de un jugador sabiendo su nombre
//Si encuentra el jugador devuelve su tiempo total
//Si no puede hacerlo devuelve un -1
int VecesGanadasUnJugador(char jugador[20],MYSQL *mysql_conn)
{
	int err;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	//Hacer la consulta
	char consulta [512];
	sprintf(consulta,"SELECT veces FROM jugador WHERE usuario='%s'", jugador);
	
	//Detectar errores en la consulta 
	err=mysql_query(mysql_conn,consulta);
	if(err!=0)
	{
		printf(consulta,"Error al consultar la base de datos %u %s\n",mysql_errno(mysql_conn),mysql_error(mysql_conn));
		exit(1);
	}
	//Conseguir los resultados
	result=mysql_store_result(mysql_conn);
	row=mysql_fetch_row(result);
	if(row==NULL)
	{
		return -1;
	}
	else
	{
		int result=atoi(row[0]);
		return result;
	}
}


//Funcion que da el ranking del primer jugador (Jugador con mas puntuacion de la tabla)
//Devuelve un -1 si da error o sino un 1
//Guarda el nombre del jugador de la posicion 1 de ranking
int RankingPrimero(MYSQL *mysql_conn,char resultado[50])
{
	int err;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	//Hacer la consulta
	char consulta [512];
	sprintf(consulta,"SELECT usuario FROM jugador WHERE puntos=(SELECT MAX(puntos) FROM jugador)");
	//Detectar errores en la consulta 
	err=mysql_query(mysql_conn,consulta);
	if(err!=0)
	{
		printf(consulta,"Error al consultar la base de datos %u %s\n",mysql_errno(mysql_conn),mysql_error(mysql_conn));
		exit(1);
	}
	//Conseguir los resultados
	result=mysql_store_result(mysql_conn);
	row=mysql_fetch_row(result);
	if(row==NULL)
	{
		return -1;
	}
	else
	{
		strcpy(resultado,row[0]);
		printf("El row[o] es:%s\n",row[0]);
		printf("El resultado es:%s\n",resultado);
		return 1;
	}
}

//Funcion que da el ranking del primer jugador (Jugador con menos puntuacion de la tabla)
//Devuelve un -1 si da error o sino un 1
//Guarda el nombre del jugador de la ultima posicion del ranking
int RankingUltimo(MYSQL *mysql_conn,char resultado[50])
{
	int err;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	//Hacer la consulta
	char consulta [512];
	sprintf(consulta,"SELECT usuario FROM jugador WHERE puntos=(SELECT MIN(puntos) FROM jugador)");
	//Detectar errores en la consulta 
	err=mysql_query(mysql_conn,consulta);
	if(err!=0)
	{
		printf(consulta,"Error al consultar la base de datos %u %s\n",mysql_errno(mysql_conn),mysql_error(mysql_conn));
		exit(1);
	}
	//Conseguir los resultados
	result=mysql_store_result(mysql_conn);
	row=mysql_fetch_row(result);
	if(row==NULL)
	{
		return -1;
	}
	else
	{
		strcpy(resultado,row[0]);
		printf("El row[o] es:%s\n",row[0]);
		printf("El resultado es:%s\n",resultado);
		return 1;
	}
}


int ActualizarBaseDatos(int partidaID, char ganador[20],MYSQL *mysql_conn ) 
{
	int err;
	MYSQL_RES *result;
	MYSQL_ROW row;
	printf("He entrado en actualizar base de datos\n");
	//Consultamos los puntos que tiene 
	char consultapuntos[512];
	int puntos;
	sprintf(consultapuntos,"SELECT puntos FROM jugador WHERE usuario='%s'",ganador);
	printf("La consulta de puntos es:%s \n",consultapuntos);
	err=mysql_query(mysql_conn,consultapuntos);
	if(err!=0)
	{
		printf("Error al consultar la base de datos %u %s\n",mysql_errno(mysql_conn),mysql_error(mysql_conn));
		exit(1);
		return 1;
	}

	//Obtenemos el resulta
	result = mysql_store_result (mysql_conn);
	row = mysql_fetch_row (result);
	if(row[0]==NULL)
	{
		puntos= 100;
	}
	else
	{
		puntos=atoi(row[0])+100;
	}
	char addpuntos[512];
	sprintf(addpuntos,"UPDATE jugador SET puntos=%d WHERE usuario='%s'",puntos,ganador);
	err=mysql_query(mysql_conn,addpuntos);
	if(err!=0)
	{
		printf("Error al consultar la base de datos %u %s\n",mysql_errno(mysql_conn),mysql_error(mysql_conn));
		exit(1);
		return 1;
	}
	
	int  veces=puntos/100;
	char addveces[512];
	sprintf(addveces,"UPDATE jugador SET veces=%d WHERE usuario='%s'",veces,ganador);
	err=mysql_query(mysql_conn,addveces);
	if(err!=0)
	{
		printf("Error al consultar la base de datos %u %s\n",mysql_errno(mysql_conn),mysql_error(mysql_conn));
		exit(1);
		return 1;
	}
	char consultapartidaID[512];
	
	//Hacemos la primera consulta
	//Obtener el id del ultimo jugador registrado
	strcpy (consultapartidaID,"SELECT MAX(partida.partidaID) FROM partida;");
	err=mysql_query(mysql_conn,consultapartidaID);
	if(err!=0)
	{
		printf("Error al consultar la base de datos %u %s\n",mysql_errno(mysql_conn),mysql_error(mysql_conn));
		exit(1);
	}
	
	//GObtenemos el resulta
	result = mysql_store_result (mysql_conn);
	row = mysql_fetch_row (result);
	//El id del nuevo usuario es el +1
	partidaID= atoi(row[0])+1;
	
	char consultapartida[512];
	sprintf(consultapartida,"INSERT INTO partida(partidaID,ganador) VALUES(%d,'%s')",partidaID,ganador);
	err=mysql_query(mysql_conn,consultapartida);
	if(err!=0)
	{
		printf("Error al consultar la base de datos %u %s\n",mysql_errno(mysql_conn),mysql_error(mysql_conn));
		exit(1);
		return 1;
	}
	else
	{
		return 2;
	}
	
}

//Funcion que nos indica cuantas veces un jugador a ganado a otro
//Si encuentra las partidas entre ambos devuelve el numero de vecesa ganadas
//Si no puede hacerlo devuelve un -1
int VecesGanadas(char jugador1[20],char jugador2[20],MYSQL *mysql_conn)
{
	int err;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	//Hcaer la consulta
	char consulta [512];
	sprintf(consulta, "SELECT COUNT(partida.ganador) from partida WHERE partida.ganador='%s' AND partida.jugador2='%s'",jugador1,jugador2);
	
	//Detectar errores en la consulta 
	err=mysql_query(mysql_conn,consulta);
	if(err!=0)
	{
		printf(consulta,"Error al consultar la base de datos %u %s\n",mysql_errno(mysql_conn),mysql_error(mysql_conn));
		exit(1);
	}
	//Conseguir los resultados
	result=mysql_store_result(mysql_conn);
	row=mysql_fetch_row(result);
	if(row==NULL)
	{
		return -1;
	}
	else
	{
		int result=atoi(row[0]);
		return result;
	}
}

void *AtenderCliente(void *socket)
{	
	int sock_conn;
	sock_conn = *(int *) socket;
	
	MYSQL_ROW row;
	char buff[512];
	char respuesta[512];
	char buff3[512];
	char notification[512];
	char user[20];
	char userme[20];
	char pass[20];
	char jugador1[20];
	char jugador2[20];
	int  ret;
	char invitado[20];
	char chatmensaje[80];
	char enviadorMensaje[80];
	
	int fin=0;
	while(fin==0) 
	{
		//Recibimos el mensaje de cliente
		ret=read(sock_conn,buff, sizeof(buff)); 
		printf ("Recibido\n");
		
		//Anadimos la marca final de string para no escribir despues del buffer
		buff[ret]='\0'; 
		printf ("Se ha conectado: %s\n",buff);
		
		//Arrancamos el codigo del mensaje
		char *p = strtok( buff, "/");
		int codigo =  atoi (p);
		
		//Analizamos para cada codigo
		//Desconectar
		if(codigo == 0) 
		{
			fin=1;
			//Eliminamos el jugador de la lista
			EliminaConectados(&usuarioLista,user);
			//Actualizamos la lista
			DameUsuarios(&usuarioLista,notification); 
			int j;
			//Notificamos la nueva lista a todos los demas
			for (j=0;j<usuarioLista.num;j++) 
				write(usuarioLista.usuarios[j].socket,notification,strlen(notification)); 
		}
		//Log In
		if(codigo == 1)
		{
			//Arrancamos el usuario del mensaje
			p = strtok( NULL, "/"); 
			strcpy (user, p);
			//Arrancamos la contrasena del mensaje
			p = strtok( NULL, "/"); 
			strcpy (pass,p);
			//Comprobamos que existe el usuario en la base de datos
			int res=login(user,pass,mysql_conn);
			//Si el usuario no existe
			if(res==1)
			{
				//Informamos que no ha sido posible el log in
				sprintf(respuesta,"1/2");
				write (sock_conn,respuesta, strlen(respuesta));
			}
			//Si el usuasrio si existe
			else
			{
				//Informamos que ha sido posible hacer el log in
				sprintf(respuesta,"1/1");
				write (sock_conn,respuesta, strlen(respuesta));
				pthread_mutex_lock (&mutex);
				strcpy(userme,user);
				//Anadimos el usuario en la lista de conectados
				anadirUsusario(userme, sock_conn, &usuarioLista); 
				pthread_mutex_unlock (&mutex);
				//Actualozamos la lista de conectados
				DameUsuarios(&usuarioLista,buff3); 
				int j;
				//Notificamos a todos los usuarios conectados con la nueva lista
				for (j=0;j<usuarioLista.num;j++)  
					write(usuarioLista.usuarios[j].socket,buff3,strlen(buff3));
			}
		}
		//SignUp
		if(codigo == 2)
		{
			//Arrancamos el usuario del mensaje
			p = strtok( NULL, "/"); 
			strcpy (user, p);
			//Arrancamos la contrasena del mensaje
			p = strtok( NULL, "/"); 
			strcpy (pass,p);
			//Comprobar si el usuario ya existe
			int res=login(user,pass,mysql_conn);
			//Si el usuario no existe
			if(res==1)
			{
				//Registramos el ususario
				int sign = signup(user,pass,mysql_conn);
				//Si no se ha podido registrar 
				if(sign==1)
				{
					//Informamos que no ha sido registrado
					sprintf(respuesta,"2/2");
					write (sock_conn,respuesta, strlen(respuesta));
				}
				//Si se ha podido registrar
				else
				{
					//Informamos que ha podido ser registrado
					sprintf(respuesta,"2/1");
					write (sock_conn,respuesta, strlen(respuesta));
				}
			}
			//Si el usuario ya existe
			else 
			{
				//Informamos que no ha sido posible registarse
				sprintf(respuesta,"2/2");
				write (sock_conn,respuesta, strlen(respuesta));
			}
		}
		if(codigo==20)
		{
			//Arrancamos el usuario del mensaje
			p = strtok( NULL, "/"); 
			strcpy (user, p);
			//Arrancamos la contrasena del mensaje
			p = strtok( NULL, "/"); 
			strcpy (pass,p);
			//Comprobar si el usuario ya existe
			int del=DeleteAccount(user,pass,mysql_conn);
			if(del==1)
			{
				sprintf(respuesta,"20/1");
				write (sock_conn,respuesta, strlen(respuesta));
				//No se ha podido eliminar
			}
			if(del==2)
			{
				//Se ha podido eliminar
				//Informamos al jugador que su cuenta ha sido eliminada
				sprintf(respuesta,"20/2");
				write (sock_conn,respuesta, strlen(respuesta));
				
				//Eliminamos el jugador de la lista
				EliminaConectados(&usuarioLista,user);
				//Actualizamos la lista
				DameUsuarios(&usuarioLista,notification); 
				int j;
				//Notificamos la nueva lista a todos los demas
				for (j=0;j<usuarioLista.num;j++) 
					write(usuarioLista.usuarios[j].socket,notification,strlen(notification)); 
			}
		}
		//PuntosTotales
		if(codigo == 3)
		{
			//Arrancamos el ususario
			p = strtok( NULL, "/"); 
			strcpy (user, p);
			//Obtenemos los puntos totales
			int res=PuntosTotales(user,mysql_conn);
			//Ha habido algun error
			if(res==-1)
			{
				//Informamos que la consulta ha sido fallida
				sprintf(respuesta,"3/0");
				write (sock_conn,respuesta, strlen(respuesta));	
			}
			//Se han obtenido los puntos totales
			else
			{
				//Informamos de los puntos totales
				sprintf(respuesta,"3/%d",res);
				write (sock_conn,respuesta, strlen(respuesta));
			}
		}
		//Cuantas veces un jugador a ganado una partida
		if(codigo == 4)
		{
			//Arrancamos el usuario
			p = strtok( NULL, "/"); 
			strcpy (user, p);
			//Obtenemos el tiempo total del jugador
			int res=VecesGanadasUnJugador(user,mysql_conn);
			//Si no se ha obtenido
			if(res==-1)
			{
				//Informamos que la consulta no ha tenido exito
				sprintf(respuesta,"4/0");
				write (sock_conn,respuesta, strlen(respuesta));	
			}
			//Si se obtiene el tiempo total
			else
			{
				//Informamos del tiempo total del jugador
				sprintf(respuesta,"4/%d",res);
				write (sock_conn,respuesta, strlen(respuesta));
			}
		}
		if(codigo==30)
		{
			char resultado[50];
			int rankprimero = RankingPrimero(mysql_conn,resultado);
			if(rankprimero==-1)
			{
				//Informamos que no ha sido posible la consulta
				sprintf(respuesta,"30/NO");
				write (sock_conn,respuesta, strlen(respuesta));
			}
			else
			{
				//Informamos del jugador que va 1 en el ranking
				sprintf(respuesta,"30/%s",resultado);
				write (sock_conn,respuesta, strlen(respuesta));
			}
		}
		if(codigo==31)
		{
			char resultado[50];
			int rankultimo = RankingUltimo(mysql_conn,resultado);
			if(rankultimo==-1)
			{
				//Informamos que no ha sido posible la consulta
				sprintf(respuesta,"31/NO");
				write (sock_conn,respuesta, strlen(respuesta));
			}
			else
			{
				//Enviamos el nombre del ultimo jugador del ranking
				sprintf(respuesta,"31/%s",resultado);
				write (sock_conn,respuesta, strlen(respuesta));
			}
		}
		//Cuantas veces uno a ganado a otro
		if(codigo == 5)
		{
			//Arrancamos el nombre del jugador ganador
			p = strtok( NULL, "/"); 
			strcpy (jugador1, p);
			//Arrancamos el nombre del jugador perdedor
			p = strtok( NULL, "/"); 
			strcpy (jugador2,p);
			//Obtenemos las veces que el jugador ha ganado al otro
			int res=VecesGanadas(jugador1,jugador2,mysql_conn);
			//Si la consulta no ha sido posible
			if(res==-1)
			{
				//Informamos que ha habido un error
				sprintf(respuesta,"5/0");
				write (sock_conn,respuesta, strlen(respuesta));
			}
			//Si la consulta ha sido posible
			else
			{
				//Informamos de las vecesa ganadas
				sprintf(respuesta,"5/%d",res);
				write (sock_conn,respuesta, strlen(respuesta));	
			}
		}
		//Enviar invitacion para jugar a un jugador
		// mensaje: 7/numerodejugadoresinvitados/invitado1/inivtado2/invitado3/...
		if (codigo==7) 
		{
			//Arrancamos el numero total de jugadores que habran en la Partida
			int socketInvitado;
			int idP;
			p = strtok(NULL,"/");
			int numPlayers = atoi(p);
			printf("El numero total de jugadores invotados es: %d\n",numPlayers);
			//Buscamos la posicion disponible en la tabla
			for (int i = 0; i < 100; i++) 
			{
				//Si numero de Jugadores de una partida es igual a -1 entonces inicializo la partida
				if (tablaPartidas[i].numJ == 0)
				{
					idP = i;
					break;
				}
			}
			
			tablaPartidas[idP].contador = numPlayers;
			tablaPartidas[idP].numJ = numPlayers + 1;
			for (int i = 0; i < numPlayers; i++)
			{
				//Arrancamos el nombre del jugador que quieren invitar
				p = strtok(NULL,"/");
				strcpy(invitado,p);
				//Comprobamos que el jugador existe
				int socketInvitado = DameSocket(&usuarioLista,invitado);
				if(socketInvitado>0)
				{
					tablaPartidas[idP].socket[i] = socketInvitado;
					//Enviamos al invitado la invitacion
					sprintf(respuesta,"8/%s/%d",userme,idP); 
					printf("La respuesta que se envia es: %s\n",respuesta);
					write(socketInvitado,respuesta,strlen(respuesta));
				}
			}
			//Me anado a mi mismo
			tablaPartidas[idP].socket[numPlayers] = sock_conn; 
		}
		//El jugador invitado envia la respuesta al invitador
		if(codigo==9)
		{
			char invitacionAnswer[20];
			int idP;
			//Arrrancamos la respuesta del invitado
			p = strtok(NULL,"/");
			strcpy(invitacionAnswer,p);
			//Arrancamos id de la partida
			p = strtok(NULL,"/");
			idP = atoi(p);
			//Si la respuesta es un SI
			if (strcmp(invitacionAnswer,"SI")==0)
			{
				//Informamos al invitador que se ha acceptado su invitacion
				tablaPartidas[idP].contador = tablaPartidas[idP].contador - 1;
			}
			else
			{
				for (int i = 0; i < tablaPartidas[idP].numJ; i ++)
				{
					//Empezamos la partida
					sprintf(respuesta,"10/NO/%d,",idP); 
					write(tablaPartidas[idP].socket[i],respuesta,strlen(respuesta));
				}
				//Libero esta id de Partida
				tablaPartidas[idP].numJ=0;
			}
			//Si el contador vale 0
			if (tablaPartidas[idP].contador == 0)
			{
				for (int i = 0; i < tablaPartidas[idP].numJ; i ++)
				{
					//Empezamos la partida
					int idJ = i;
					sprintf(respuesta,"10/SI/%d/%d/%d",idP,idJ,tablaPartidas[idP].numJ); 
					write(tablaPartidas[idP].socket[i],respuesta,strlen(respuesta));
				}
			}
		}
		//Enviamos un mensaje al oponente
		if(codigo==11)
		{
			//Arrancamos el nombre del oponente
			p = strtok(NULL,"/");
			strcpy(enviadorMensaje,p);
			//Arrancamos el mensaje que queremos enviar
			p = strtok(NULL, "/");
			strcpy(chatmensaje,p);
			//Arrancamos id de la partida
			p = strtok(NULL, "/");
			int idMatch = atoi(p);
			//Le enviamos el mensaje para elmensaje a todos
			for (int i = 0; i < tablaPartidas[idMatch].numJ; i ++)
			{
				sprintf(respuesta,"11/%s:%s",enviadorMensaje,chatmensaje); 
				write(tablaPartidas[idMatch].socket[i],respuesta,strlen(respuesta));
			}
		}
		//Me ennvian la cara del dado que ha salido 12/dado/idP
		if (codigo == 12) 
		{
			p = strtok(NULL,"/");
			int dado = atoi(p);
			p = strtok(NULL,"/");
			int idP = atoi(p);
			p = strtok(NULL,"/");
			int idJ = atoi(p);

			for(int i = 0; i < tablaPartidas[idP].numJ; i++)
			{
				sprintf(respuesta,"12/%d/%d/%d/%d/",dado,idP,idJ,i);
				write(tablaPartidas[idP].socket[i],respuesta,strlen(respuesta));
				printf ("%s\n", respuesta);
			}
		}
		if(codigo==22) //Se ha acabado la partida
		{
			//Arrancamos el nombre del ganador
			p = strtok(NULL,"/");
			char ganador[20];
			strcpy(ganador,p);
			//Arrancamos id de la partida
			p = strtok(NULL,"/");
			int idPartida= atoi(p);
			pthread_mutex_lock(&mutex);
			//Actualizamos base de datos 
			int correcto=ActualizarBaseDatos(idPartida,ganador,mysql_conn); 
			pthread_mutex_unlock(&mutex);
			if(correcto==2)
			{
				sprintf(respuesta,"22/%s",ganador);
			}
			else
			{
				sprintf(respuesta,"23/2/");
			}
			for(int i = 0; i < tablaPartidas[idPartida].numJ; i++)
			{
				write(tablaPartidas[idPartida].socket[i],respuesta,strlen(respuesta));
			}
		}
		//Alguien se quiere desconectar en mitad de Partida
		//90/jugador que quiere desconectarse/idp
		if(codigo==90)
		{
			//Arrancamos el nombre del que se quiere desconectar
			p = strtok(NULL,"/");
			char jugadorDesconectado[20];
			strcpy(jugadorDesconectado,p);
			p = strtok(NULL,"/");
			int idPartidaDesconetada = atoi(p);
			p =  strtok(NULL,"/");
			int idJugDes=atoi(p);
			for(int i = 0; i < tablaPartidas[idPartidaDesconetada].numJ; i++)
			{
				if(i!=idJugDes)
				{
					sprintf(respuesta,"90/%s",jugadorDesconectado);
					write(tablaPartidas[idPartidaDesconetada].socket[i],respuesta,strlen(respuesta));
				}
			}
			sprintf(respuesta,"91/");
			write (sock_conn,respuesta, strlen(respuesta));
		}
		printf ("%s\n", respuesta);
	}
	close(sock_conn); 
}
	
int main(int argc, char *argv[])
{
	MYSQL_ROW row;
	int sock_conn, sock_listen, ret;
	struct sockaddr_in serv_adr;
			
	// INICIALITZACIONS:
	//Obrim i inicialitzem connexi? amb mysql
	mysql_conn = mysql_init(NULL);
	if (mysql_conn==NULL)
	{
		printf("Error al crear la conexion: %u %s\n", mysql_errno(mysql_conn), mysql_error(mysql_conn));
		exit(1);
	}
	mysql_conn = mysql_real_connect(mysql_conn,"shiva2.upc.es","root","mysql","isma13",0,NULL,0);
	if (mysql_conn==NULL)
	{
		printf ("Error al inicializar la conexion: %u %s\n",mysql_errno(mysql_conn),mysql_error(mysql_conn));
		exit(1);
	}
			
	// Obrim el socket
	if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("Error creant socket");
	// Fem el bind al port
	memset(&serv_adr, 0, sizeof(serv_adr));// inicialitza a zero serv_addr
	serv_adr.sin_family = AF_INET;
			
	// asocia el socket a cualquiera de las IP de la m?quina. 
	//htonl formatea el numero que recibe al formato necesario
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	// escucharemos en el port 50001
	serv_adr.sin_port = htons(50001);
	if (bind(sock_listen, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) < 0)
		printf ("Error al bind");
	//La cola de peticiones pendientes no podr? ser superior a 4
	if (listen(sock_listen, 2) < 0)
		printf("Error en el Listen");
	
	pthread_t thread[100]; //estructura especial definida a la llibreria 
	//nomes puc crear 100 thread
	
	//***Observar si esto es lo que falla***
	int sockets[10];
	i = 0;
	for(;;)
	{
		printf ("Escuchando\n");
		sock_conn = accept(sock_listen, NULL, NULL);
		sockets[i] = sock_conn;
				
		printf ("He recibido conexi?n\n");
		pthread_create( &thread[i], 
		NULL, 
		AtenderCliente, //nom de la funcio a ajecutar
		&sockets[i]); //socket per entregar al threat
		i=i+1;
	}
	mysql_close (mysql_conn);
}
