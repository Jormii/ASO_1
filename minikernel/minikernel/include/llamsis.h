/**
 * minikernel/kernel/include/llamsis.h
 * Minikernel. Version 1.0
 * Fernando Perez Costoya
 */

/**
 * Fichero de cabecera que contiene el numero asociado a cada llamada
 * SE DEBE MODIFICAR PARA INCLUIR NUEVAS LLAMADAS
 */

#ifndef _LLAMSIS_H
#define _LLAMSIS_H

#define NSERVICIOS 11   // Numero de llamadas al sistema disponibles

#define CREAR_PROCESO 0
#define TERMINAR_PROCESO 1
#define ESCRIBIR 2
#define OBTENER_ID 3
#define DORMIR_PROCESO 4
#define CREAR_MUTEX 5
#define ABRIR_MUTEX 6
#define LOCK_MUTEX 7
#define UNLOCK_MUTEX 8
#define CERRAR_MUTEX 9
#define LEER_CARACTER 10

#endif /* _LLAMSIS_H */