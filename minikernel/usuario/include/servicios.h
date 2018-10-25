/*
 *  usuario/include/servicios.h
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene los prototipos de funciones de
 * biblioteca que proporcionan la interfaz de llamadas al sistema.
 *
 *      SE DEBE MODIFICAR AL INCLUIR NUEVAS LLAMADAS
 *
 */

#ifndef SERVICIOS_H
#define SERVICIOS_H

/* Evita el uso del printf de la bilioteca est�ndar */
#define printf escribirf

/* Funcion de biblioteca */
int escribirf(const char *formato, ...);

/* Llamadas al sistema proporcionadas */
int crear_proceso(char *prog);
int terminar_proceso();
int escribir(char *texto, unsigned int longi);

/*
	16 / 10 / 2018
*/
int obtener_id_pr();

/*
	19 / 10 / 2018
*/
int dormir(unsigned int s);

/**
 * 23 / 10 / 2018
 */
#define RECURSIVO 0
#define NO_RECURSIVO 1
int crear_mutex(char *nombre, int tipo);
int abrir_mutex(char *nombre);
int lock(unsigned int mutex_id);
int unlock(unsigned int mutex_id);
int cerrar_mutex(unsigned int mutex_id);

#endif /* SERVICIOS_H */

