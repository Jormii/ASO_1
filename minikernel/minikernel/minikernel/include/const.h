/**
 * minikernel/include/const.h
 * Minikernel. Version 1.0
 * Fernando Perez Costoya
 */

/**
 * Fichero de cabecera que contiene definiciones de constantes
 * NO SE DEBE MODIFICAR
 */

#ifndef _CONST_H
#define _CONST_H

#ifndef NULL
#define NULL (void *) 0		// Por si acaso no esta ya definida
#endif

#define MAX_PROC 10		// Dimension de la tabla de procesos
#define TAM_PILA 32768

/**
 * Posibles estados del proceso
 */
#define NO_USADA 0		// Entrada de tabla de procesos no usada
#define TERMINADO 0		// Proceso TERMINADO == entrada NO_USADA
#define LISTO 1
#define EJECUCION 2
#define BLOQUEADO 3

/**
 * Niveles de ejecucion del procesador. 
 */
#define NUM_NIVELES 3
#define NIVEL_1 1	// Interrupcion software
#define NIVEL_2 2	// Interrupcion de terminal
#define NIVEL_3 3	// Interrupcion de reloj

/**
 * Definicion de constantes relacionadas con vectores de interrupcion
 */
#define NVECTORES 6		// Numero de vectores de interrupcion disponibles

// Numeros de vector
#define EXC_ARITM 0		// Excepcion aritmetica
#define EXC_MEM 1       // Excepcion en acceso a memoria
#define INT_RELOJ 2     // Interrupcion de reloj
#define INT_TERMINAL 3  // Interrupcion de entrada de terminal
#define LLAM_SIS 4      // Vector usado para llamadas
#define INT_SW 5		// Vector usado para interrupciones software

/**
 * Definicion de constantes relacionadas con el reloj
 */
#define TICK 100				// Frecuencia de reloj requerida (ticks/segundo)
#define TICKS_POR_RODAJA 10		/* Constante usada en la implementacion de round robin. Fijar a
								3 para ver los efectos del round robin en las pruebas.
								10 por defecto */

/**
 * Constantes usada en la implementacion del mutex
 */
#define NUM_MUT 16			// Numero total de mutex en el sistema
#define NUM_MUT_PROC 4 		/* Numero maximo de mutex que puede tener
			  				abiertos un proceso */
#define MAX_NOM_MUT 8 		// Longitud maxima de un nombre de mutex

/**
 * Constantes usada en la implementacion del manejador del terminal
 */
#define TAM_BUF_TERM 8		// Tamano del buffer del terminal
#define DIR_TERMINAL 1		// Direccion de puerto de E/S del terminal

#endif // _CONST_H