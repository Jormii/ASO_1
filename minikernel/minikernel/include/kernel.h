/*
 *  minikernel/include/kernel.h
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene definiciones usadas por kernel.c
 *
 *      SE DEBE MODIFICAR PARA INCLUIR NUEVA FUNCIONALIDAD
 *
 */

#ifndef _KERNEL_H
#define _KERNEL_H

#include "const.h"
#include "HAL.h"
#include "llamsis.h"

/**
 * DECLARACION DE LOS MUTEX
 * 23 / 10 / 2018
 */

// Structs
typedef struct mutex {
    char nombre[MAX_NOM_MUT];
    int estado;
	int abierto;
	int tipo;
	int num_bloqueos;
	int num_procesos_bloqueados;
} mutex;

/**
 * Constantes
 */
#define RECURSIVO 0
#define NO_RECURSIVO 1

#define MUTEX_LIBRE 0
#define MUTEX_BLOQUEADO 1

#define MUTEX_CERRADO 0
#define MUTEX_ABIERTO 1
mutex tabla_mutex[NUM_MUT];

/**
 * Declaracion de funciones auxiliares para los mutex
 */
static void iniciar_tabla_mutex();
static int buscar_descriptor_libre();
static int buscar_nombre_mutex(char *nombre_mutex);
static int buscar_mutex_libre();

/*
 *
 * Definicion del tipo que corresponde con el BCP.
 * Se va a modificar al incluir la funcionalidad pedida.
 *
 */
typedef struct BCP_t *BCPptr;

typedef struct BCP_t
{
	int id;						/* ident. del proceso */
	int estado;					/* TERMINADO|LISTO|EJECUCION|BLOQUEADO*/
	contexto_t contexto_regs;	/* copia de regs. de UCP */
	void * pila;				/* dir. inicial de la pila */
	BCPptr siguiente;			/* puntero a otro BCP */
	void *info_mem;				/* descriptor del mapa de memoria */

	int ciclos_dormido;			// 19 / 10 / 2018
	mutex *descriptores_mutex[NUM_MUT_PROC];		// 23 / 08 / 2018
} BCP;

/*
 *
 * Definicion del tipo que corresponde con la cabecera de una lista
 * de BCPs. Este tipo se puede usar para diversas listas (procesos listos,
 * procesos bloqueados en sem�foro, etc.).
 *
 */

typedef struct
{
	BCP *primero;
	BCP *ultimo;
} lista_BCPs;


/*
 * Variable global que identifica el proceso actual
 */

BCP *p_proc_actual = NULL;

/*
 * Variable global que representa la tabla de procesos
 */
BCP tabla_procs[MAX_PROC];

/*
 * Variable global que representa la cola de procesos listos
 */
lista_BCPs lista_listos = { NULL, NULL };

/*
	19 / 10 / 2018
*/
lista_BCPs cola_bloqueados_dormir = { NULL, NULL };

/*
	23 / 10 / 2018
*/
lista_BCPs cola_bloqueados_mutex_libre = { NULL, NULL };
lista_BCPs cola_bloqueados_mutex_lock = { NULL, NULL };

/*
 *
 * Definici�n del tipo que corresponde con una entrada en la tabla de
 * llamadas al sistema.
 *
 */
typedef struct
{
	int (*fservicio)();
} servicio;


/*
 * Prototipos de las rutinas que realizan cada llamada al sistema
 */
int sis_crear_proceso();
int sis_terminar_proceso();
int sis_escribir();
int sis_obtener_id_pr();		// 16 / 10 / 2018
int sis_dormir();				// 19 / 10 / 2018
/**
 * 23 / 10 / 2018
 */
int sis_crear_mutex();
int sis_abrir_mutex();
int sis_lock_mutex();
int sis_unlock_mutex();
int sis_cerrar_mutex();

/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */
servicio tabla_servicios[NSERVICIOS] =	{	{sis_crear_proceso},
											{sis_terminar_proceso},
											{sis_escribir},
											{sis_obtener_id_pr},
											{sis_dormir},
											{sis_crear_mutex},
											{sis_abrir_mutex},
											{sis_lock_mutex},
											{sis_unlock_mutex},
											{sis_cerrar_mutex}
										};

#endif /* _KERNEL_H */