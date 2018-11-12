/**
 *  usuario/include/servicios.h
 *  Minikernel. Version 1.0
 *  Fernando Perez Costoya
 */

/**
 * Fichero de cabecera que contiene los prototipos de funciones de
 * biblioteca que proporcionan la interfaz de llamadas al sistema.
 * SE DEBE MODIFICAR AL INCLUIR NUEVAS LLAMADAS
 */
#ifndef SERVICIOS_H
#define SERVICIOS_H

#define printf escribirf // Evita el uso del printf de la bilioteca estandar

// Constantes que permiten al usuario definir el tipo de mutex que quiere crear
#define NO_RECURSIVO 0
#define RECURSIVO 1

// Funciones de biblioteca
int escribirf(const char *formato, ...);

/**
 * Llamadas al sistema
 */
// Llamadas al sistema proporcionadas
int crear_proceso(char *prog);
int terminar_proceso();
int escribir(char *texto, unsigned int longi);

// Llamadas al sistema implementadas para la practica
int obtener_id_pr();
int dormir(unsigned int s);
int crear_mutex(char *nombre, int tipo);
int abrir_mutex(char *nombre);
int lock(unsigned int mutex_id);
int unlock(unsigned int mutex_id);
int cerrar_mutex(unsigned int mutex_id);
int leer_caracter();

#endif // SERVICIOS_H