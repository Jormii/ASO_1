/**
 * minikernel/include/kernel.h
 * Minikernel version 1.0
 * Fernando Perez Costoya
 */

#ifndef _KERNEL_H
#define _KERNEL_H

#include "const.h"
#include "HAL.h"
#include "llamsis.h"

/**
 * Constantes usadas por los mutex
 */
#define MUTEX_TIPO_NO_RECURSIVO 0
#define MUTEX_TIPO_RECURSIVO 1
#define MUTEX_ESTADO_LIBRE 0
#define MUTEX_ESTADO_CREADO 1
#define MUTEX_ESTADO_BLOQUEADO 2

/**
 * Declaracion de tipos
 */
typedef struct BCP_t BCP;
typedef struct servicio_t servicio;
typedef struct lista_t lista_BCPs;
typedef struct mutex_t mutex;
typedef struct terminal_t terminal;

/**
 * Declaracion de funciones
 */
// Operaciones sobre procesos, tabla de procesos y BCPs
static void iniciar_tabla_proc();			// Inicia la tabla de procesos
static int buscar_BCP_libre();				// Busca una entrada libre en la tabla de procesos
static int crear_tarea(char *programa);		// Crea un proceso reservando sus recursos. Usada por la llamada al sistema "crear_proceso"

// Operaciones sobre las listas. Primero eliminar un proceso. Despues eliminarlo
static void insertar_ultimo(lista_BCPs *lista, BCP *proceso);	// Insertar un BCP al final de la lista
static void eliminar_primero(lista_BCPs *lista);				// Elimina el primer BCP de la lista
static void eliminar_elem(lista_BCPs *lista, BCP *proceso);		// Elimina el BCP "proceso" de la lista

// Manejadores de excepciones
static void exc_arit();		// Tratamiento de excepciones de acceso a memoria
static void exc_mem();		// Tratamiento de excepciones aritmeticas

// Vinculadas a las RTI y planificacion
static BCP* planificador();		// Funcion de planificacion mediante un algoritmo FIFO
static void liberar_proceso();  /* Funcion auxiliar que termina proceso actual liberando sus recursos.
 								Usada por la llamada "terminar_proceso" y por rutinas que tratan excepciones */
static void espera_int();		// Espera a que se produzca una interrupcion
static void int_reloj();		// Tratamiento de interrupciones de reloj
static void int_sw();			// Tratamiento de interrupciones software
static void int_terminal();		// Tratamiento de interrupciones de terminal

// Vinculadas a los mutex
static void iniciar_tabla_mutex();			// Inicializa los mutex del sistema
static int buscar_descriptor_libre();		// Busca y devuelve un descriptor de mutex disponible en el proceso actual
static int buscar_nombre_mutex(char *nombre_mutex);		// Busca y devuelve el indice del mutex con el nombre argumento
static int buscar_mutex_libre();						// Busca y devuelve un indice libre en la tabla de mutex
static BCP* buscar_procesos_con_mutex(mutex *mutex);	/* Busca entre la lista de procesos bloqueados debidos a lock
														un proceso que haya abierto el mutex argumento */
static void otorgar_mutex(int descriptor, mutex *mutex_unlock);		// Otorga el mutex argumento a algun proceso bloqueado por el mismo
static void cerrar_mutex(int descriptor, mutex *mutex_cerrar);		// Cierra el mutex argumento

// Terminal
static void iniciar_terminal();		// Inicializa las variables de control del terminal

// Llamadas al sistema
static void tratar_llamsis();	// Tratamiento de llamadas al sistema
int sis_crear_proceso();		// Tratamiento de la llamada al sistema "crear_proceso". Llama a "crear_tarea"
int sis_terminar_proceso();		// Tratamiento de la llamada al sistema "terminar_proceso". Llama al "liberar_proceso"
int sis_escribir();				// Tratamiento de la llamada al sistema "escribir". Llama a "escribir_ker"
int sis_obtener_id_pr();		// Tratamiento de la llamada al sistema "obtener_id_pr"
int sis_dormir();				// Tratamiento de la llamada al sistema "dormir"
int sis_crear_mutex();			// Tratamiento de la llamada al sistema "crear_mutex"
int sis_abrir_mutex();			// Tratamiento de la llamada al sistema "abrir_mutex"
int sis_lock_mutex();			// Tratamiento de la llamada al sistema "lock_mutex"
int sis_unlock_mutex();			// Tratamiento de la llamada al sistema "unlock_mutex"
int sis_cerrar_mutex();			// Tratamiento de la llamada al sistema "cerrar_mutex"
int sis_leer_caracter();		// Tratamiento de la llamada al sistema "leer_caracter"

/**
 * Definicion de los structs
 */
typedef struct BCP_t
{
	int id;										// Identificador del proceso
	int estado;									// TERMINADO | LISTO | EJECUCION | BLOQUEADO
	contexto_t contexto_regs;					// Copia de los registros de la CPU
	void * pila;								// Puntero al comienzo de la pila
	BCP *siguiente;								// Puntero al proximo proceso en la lista contenedora
	void *info_mem;								// Descritor del mapa de memoria
	int ciclos_dormido;							// Numero de ciclos que restan para que el proceso despierte de la llamada dormir()
	mutex *descriptores_mutex[NUM_MUT_PROC];	// Punteros a los mutex abiertos por este proceso
	int ciclos_en_ejecucion;					// Contador descendente que indica los ciclos que restan para que el round robin expulse a este proceso de ejecucion
	int descriptor_mutex_libre;					// Valor de un descriptor de mutex obtenido antes de que fuera bloqueado por sis_crear_mutex()
} BCP;

typedef struct servicio_t
{
	int (*fservicio)();
} servicio;

typedef struct lista_t
{
	BCP *primero;	// Puntero al primer elemento de la lista
	BCP *ultimo;	// Puntero al ultimo elemento de la lista
} lista_BCPs;

typedef struct mutex_t
{
	int id;
    char nombre[MAX_NOM_MUT];		// Nombre identificador y univoco del mutex
    int estado;						// Estado actual del mutex: LIBRE | CREADO | BLOQUEADO
	int tipo;						// Indica el tipo del mutex: RECURSIVO | NO_RECURSIVO
	int num_locks;					// Representa cuantos locks se han realizado sobre el mutex recursivo
	int num_procesos_bloqueados;	// Numero de procesos que el mutex ha bloqueado a causa de lock()
	int id_proc_bloq;				// ID del proceso que posee el mutex
	int n_procesos;					// Numero de procesos que han abierto este mutex
} mutex;

typedef struct terminal_t
{
	char buffer[TAM_BUF_TERM];		// Buffer que almacena los caracteres introducidos por teclado
	int indice;						// Puntero al indice del buffer donde se introducira el proximo caracter
	int indice_proc;				// Puntero al indice del buffer que debe leer el proceso que llama a leer_caracter()
	int elementos;					// Numero de caracteres que permanecen en el buffer
} terminal;

/**
 * Variables globales
 */
BCP *p_proc_actual = NULL;		// Puntero al proceso en ejecucion
BCP tabla_procs[MAX_PROC];		// Array que almacena los procesos iniciados
mutex tabla_mutex[NUM_MUT];		// Array con todos los mutex del sistema disponibles
lista_BCPs cola_listos = { NULL, NULL };					// Cola de procesos listos
lista_BCPs cola_bloqueados_dormir = { NULL, NULL };			// Cola de procesos bloqueados por la llamada al sistema dormir()
lista_BCPs cola_bloqueados_mutex_libre = { NULL, NULL };	// Cola de procesos bloqueados por aquellos procesos que no obtuvieron ningun proceso
lista_BCPs cola_bloqueados_mutex_lock = { NULL, NULL };		// Cola de procesos bloqueados por intentar hacer lock() sobre un mutex ya bloqueado
lista_BCPs cola_bloqueados_terminal = { NULL, NULL };		// Cola de procesos bloqueados por sis_leer_caracter()
terminal terminal_sis;

// Array que contiene los punteros a las funciones que manejan las llamadas al sistema
servicio tabla_servicios[NSERVICIOS] =	{	{sis_crear_proceso},
											{sis_terminar_proceso},
											{sis_escribir},
											{sis_obtener_id_pr},
											{sis_dormir},
											{sis_crear_mutex},
											{sis_abrir_mutex},
											{sis_lock_mutex},
											{sis_unlock_mutex},
											{sis_cerrar_mutex},
											{sis_leer_caracter}
										};
#endif // _KERNEL_H