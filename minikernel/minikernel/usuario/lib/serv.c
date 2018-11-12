/**
 * usuario/lib/serv.c
 * Minikernel. Version 1.0
 * Fernando Perez Costoya
 */

/**
 * Fichero que contiene las definiciones de las funciones de interfaz
 * a las llamadas al sistema. Usa la funcion de apoyo llamsis
 * SE DEBE MODIFICAR AL INCLUIR NUEVAS LLAMADAS
 */
#include "llamsis.h"
#include "servicios.h"

/* Funcion del modulo "misc" que prepara el codigo de la llamada
   (en el registro 0), los parametros (en registros 1, 2, ...), realiza la
   instruccion de llamada al sistema  y devuelve el resultado 
   (que obtiene del registro 0) */
int llamsis(int llamada, int nargs, ... /* args */);

/**
 * Funciones interfaz a las llamadas al sistema
 */
int crear_proceso(char *prog)
{
	return llamsis(CREAR_PROCESO, 1, (long)prog);
}

int terminar_proceso()
{
	return llamsis(TERMINAR_PROCESO, 0);
}

int escribir(char *texto, unsigned int longi){
	return llamsis(ESCRIBIR, 2, (long)texto, (long)longi);
}

int obtener_id_pr()
{
	return llamsis(OBTENER_ID, 0);
}

int dormir(unsigned int s)
{
	return llamsis(DORMIR_PROCESO, 1, s);
}

int crear_mutex(char *nombre, int tipo)
{
	return llamsis(CREAR_MUTEX, 2, (long)nombre, (long)tipo);
}

int abrir_mutex(char *nombre)
{
	return llamsis(ABRIR_MUTEX, 1, (long)nombre);
}

int lock(unsigned int mutex_id)
{
	return llamsis(LOCK_MUTEX, 1, (long)mutex_id);
}

int unlock(unsigned int mutex_id)
{
	return llamsis(UNLOCK_MUTEX, 1, (long)mutex_id);
}

int cerrar_mutex(unsigned int mutex_id)
{
	return llamsis(CERRAR_MUTEX, 1, (long)mutex_id);
}

int leer_caracter()
{
	return llamsis(LEER_CARACTER, 0);
}