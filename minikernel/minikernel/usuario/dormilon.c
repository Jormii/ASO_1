/*
 * usuario/dormilon.c
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 * Programa de usuario que duerme dos veces.
 */

#include "servicios.h"

int main(){
	int segs, id;

	id=obtener_id_pr();
	printf("dormilon (%d): comienza\n", id);

	/* primero duerme 1 segundo */
	int dormirT = 1;
	printf("dormilon (%d) duerme %d segundo/s\n", id, dormirT);
	dormir(dormirT);

	/* despues duerme numero de segundos dependiendo de su pid */
	segs=id+1;
	printf("dormilon (%d) duerme %d segundos\n", id, segs);
	dormir(segs);

	printf("dormilon (%d): termina\n", id);
	return 0;
}
