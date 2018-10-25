/*
 *  kernel/kernel.c
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero que contiene la funcionalidad del sistema operativo
 *
 */

#include "kernel.h"	/* Contiene defs. usadas por este modulo */
#include <string.h>

/**
 * Declaración de funciones que no pertenecen a "kernel.h"
 */
void imp_cola(lista_BCPs *cola, const char *nombre);
static void iniciar_tabla_proc();
static int buscar_BCP_libre();
static void insertar_ultimo(lista_BCPs *lista, BCP * proc);
static void eliminar_primero(lista_BCPs *lista);
static void espera_int();
static BCP * planificador();
static void liberar_proceso();
static void int_sw();
// INCLUIR EL RESTO

/**
 * FUNCIONES AUXILIARES PARA DEBUG
 * 
 */

// 23 / 08 / 2018
void imp_cola(lista_BCPs *cola, const char *nombre)
{
	BCP *p_proc = cola->primero;
	if (p_proc != NULL)
	{
		printk("\nCola de procesos %s\n", nombre);
	}

	while (p_proc != NULL)
	{
		printk("\tID: %d; ESTADO: %d\n", p_proc->id, p_proc->estado);
		p_proc = p_proc->siguiente;
	}
}


/*
 *
 * Funciones relacionadas con la tabla de procesos:
 *	iniciar_tabla_proc buscar_BCP_libre
 *
 */

/*
 * Funci�n que inicia la tabla de procesos
 */
static void iniciar_tabla_proc()
{
	int i;

	for (i=0; i<MAX_PROC; i++)
		tabla_procs[i].estado=NO_USADA;
}

/*
 * Funci�n que busca una entrada libre en la tabla de procesos
 */
static int buscar_BCP_libre()
{
	int i;

	for (i=0; i<MAX_PROC; i++)
		if (tabla_procs[i].estado==NO_USADA)
			return i;
	return -1;
}

/*
 *
 * Funciones que facilitan el manejo de las listas de BCPs
 *	insertar_ultimo eliminar_primero eliminar_elem
 *
 * NOTA: PRIMERO SE DEBE LLAMAR A eliminar Y LUEGO A insertar
 */

/*
 * Inserta un BCP al final de la lista.
 */
static void insertar_ultimo(lista_BCPs *lista, BCP * proc)
{
	if (lista->primero==NULL)
		lista->primero= proc;
	else
		lista->ultimo->siguiente=proc;
	lista->ultimo= proc;
	proc->siguiente=NULL;
}

/*
 * Elimina el primer BCP de la lista.
 */
static void eliminar_primero(lista_BCPs *lista)
{
	if (lista->ultimo==lista->primero)
		lista->ultimo=NULL;
	lista->primero=lista->primero->siguiente;
}

/*
 * Elimina un determinado BCP de la lista.
 */
static void eliminar_elem(lista_BCPs *lista, BCP * proc)
{
	BCP *paux=lista->primero;

	if (paux==proc)
		eliminar_primero(lista);
	else 
	{
		for ( ; ((paux) && (paux->siguiente!=proc));
			paux=paux->siguiente);
		if (paux) 
		{
			if (lista->ultimo==paux->siguiente)
				lista->ultimo=paux;
			paux->siguiente=paux->siguiente->siguiente;
		}
	}
}

/*
 *
 * Funciones relacionadas con la planificacion
 *	espera_int planificador
 */

/*
 * Espera a que se produzca una interrupcion
 */
static void espera_int()
{
	int nivel;

	// printk("-> NO HAY LISTOS. ESPERA INT\n");

	/* Baja al m�nimo el nivel de interrupci�n mientras espera */
	nivel=fijar_nivel_int(NIVEL_1);
	halt();
	fijar_nivel_int(nivel);
}

/*
 * Funci�n de planificacion que implementa un algoritmo FIFO.
 */
static BCP * planificador()
{
	while (lista_listos.primero==NULL)
		espera_int();		/* No hay nada que hacer */

	return lista_listos.primero;
}

/*
 *
 * Funcion auxiliar que termina proceso actual liberando sus recursos.
 * Usada por llamada terminar_proceso y por rutinas que tratan excepciones
 *
 */
static void liberar_proceso()
{
	BCP * p_proc_anterior;

	liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */

	p_proc_actual->estado=TERMINADO;
	eliminar_primero(&lista_listos); /* proc. fuera de listos */

	/* Realizar cambio de contexto */
	p_proc_anterior=p_proc_actual;
	p_proc_actual=planificador();

	printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);

	liberar_pila(p_proc_anterior->pila);
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
        return; /* no deber�a llegar aqui */
}

/*
 *
 * Funciones relacionadas con el tratamiento de interrupciones
 *	excepciones: exc_arit exc_mem
 *	interrupciones de reloj: int_reloj
 *	interrupciones del terminal: int_terminal
 *	llamadas al sistemas: llam_sis
 *	interrupciones SW: int_sw
 *
 */

/*
 * Tratamiento de excepciones aritmeticas
 */
static void exc_arit()
{
	if (!viene_de_modo_usuario())
		panico("excepcion aritmetica cuando estaba dentro del kernel");


	printk("-> EXCEPCION ARITMETICA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

	return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de excepciones en el acceso a memoria
 */
static void exc_mem()
{
	if (!viene_de_modo_usuario())
		panico("excepcion de memoria cuando estaba dentro del kernel");


	printk("-> EXCEPCION DE MEMORIA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de interrupciones de terminal
 */
static void int_terminal()
{
	char car;

	car = leer_puerto(DIR_TERMINAL);
	printk("-> TRATANDO INT. DE TERMINAL %c\n", car);

	return;
}

/*
 * Tratamiento de interrupciones de reloj
 */
static void int_reloj()
{
	printk("-> TRATANDO INT. DE RELOJ\n");

	/**
	 * 25 / 10 / 2018
	 */
	int id = p_proc_actual->id;
	int ciclos = p_proc_actual->ciclos_en_ejecucion;
	printk("\tAl proceso %d le restan %d ciclos en ejecución\n", id, ciclos);
	if (p_proc_actual->ciclos_en_ejecucion == 0)
	{
		int_sw();
		return;
	}
	p_proc_actual->ciclos_en_ejecucion--;
	/**/

	/*
		19 / 10 / 2018
	*/
	BCP *p_proc = cola_bloqueados_dormir.primero;

	if (p_proc)
	{
		printk("-> PROCESANDO PROCESOS BLOQUEADOS\n");
	}

	while (p_proc != NULL)
	{
		printk("\tID: %d, Ciclos dormido: %d\n!", p_proc->id, p_proc->ciclos_dormido);

		if (p_proc->ciclos_dormido == 0)
		{
			printk("\tID: %d se ha despertado\n", p_proc->id);

			BCP *p_proximo = p_proc->siguiente;

			p_proc->estado = LISTO;
			p_proc->ciclos_dormido = -1;
			eliminar_elem(&cola_bloqueados_dormir, p_proc);
			insertar_ultimo(&lista_listos, p_proc);

			p_proc = p_proximo;
		}
		else
		{
			p_proc->ciclos_dormido--;
			p_proc = p_proc->siguiente;
		}
	}
	printk("\n");
	return;
}

/*
 * Tratamiento de llamadas al sistema
 */
static void tratar_llamsis()
{
	int nserv, res;

	nserv=leer_registro(0);
	if (nserv<NSERVICIOS)
		res=(tabla_servicios[nserv].fservicio)();
	else
		res=-1;		/* servicio no existente */
	escribir_registro(0,res);
	return;
}

/*
 * Tratamiento de interrupciuones software
 */
static void int_sw()
{
	printk("-> TRATANDO INT. SW\n");

	/**
	 * 25 / 10 / 2018
	 */
	
	BCP *proceso_a_expulsar = p_proc_actual;
	proceso_a_expulsar->ciclos_en_ejecucion = TICKS_POR_RODAJA;
	printk("\tSe va a expulsar el proceso %d\n", proceso_a_expulsar->id);

	int nivel = fijar_nivel_int(NIVEL_3);

	eliminar_elem(&lista_listos, proceso_a_expulsar);

	p_proc_actual = planificador();
	p_proc_actual->ciclos_en_ejecucion = TICKS_POR_RODAJA;

	insertar_ultimo(&lista_listos, proceso_a_expulsar);

	printk("\tEntra a ejecutarse el proceso %d\n", p_proc_actual->id);

	fijar_nivel_int(nivel);
	cambio_contexto(&(proceso_a_expulsar->contexto_regs), &(p_proc_actual->contexto_regs));

	return;
}

/*
 *
 * Funcion auxiliar que crea un proceso reservando sus recursos.
 * Usada por llamada crear_proceso.
 *
 */
static int crear_tarea(char *prog)
{
	void *imagen, *pc_inicial;
	int error=0;
	int proc;
	BCP *p_proc;

	proc=buscar_BCP_libre();
	if (proc==-1)
		return -1;	/* no hay entrada libre */

	/* A rellenar el BCP ... */
	p_proc=&(tabla_procs[proc]);

	/* crea la imagen de memoria leyendo ejecutable */
	imagen=crear_imagen(prog, &pc_inicial);
	if (imagen)
	{
		p_proc->info_mem=imagen;
		p_proc->pila=crear_pila(TAM_PILA);
		fijar_contexto_ini(p_proc->info_mem, p_proc->pila, TAM_PILA,
			pc_inicial,
			&(p_proc->contexto_regs));
		p_proc->id=proc;
		p_proc->estado=LISTO;

		p_proc->ciclos_dormido = -1;	// 19 / 10 / 2018
		p_proc->ciclos_en_ejecucion = TICKS_POR_RODAJA;

		/* lo inserta al final de cola de listos */
		insertar_ultimo(&lista_listos, p_proc);
		error= 0;
	}
	else
		error= -1; /* fallo al crear imagen */

	return error;
}

/*
 *
 * Rutinas que llevan a cabo las llamadas al sistema
 *	sis_crear_proceso sis_escribir
 *
 */

/*
 * Tratamiento de llamada al sistema crear_proceso. Llama a la
 * funcion auxiliar crear_tarea sis_terminar_proceso
 */
int sis_crear_proceso()
{
	char *prog;
	int res;

	printk("-> PROC %d: CREAR PROCESO\n", p_proc_actual->id);
	prog=(char *)leer_registro(1);
	res=crear_tarea(prog);
	return res;
}

/*
 * Tratamiento de llamada al sistema escribir. Llama simplemente a la
 * funcion de apoyo escribir_ker
 */
int sis_escribir()
{
	char *texto;
	unsigned int longi;

	texto=(char *)leer_registro(1);
	longi=(unsigned int)leer_registro(2);

	escribir_ker(texto, longi);
	return 0;
}

/*
 * Tratamiento de llamada al sistema terminar_proceso. Llama a la
 * funcion auxiliar liberar_proceso
 */
int sis_terminar_proceso()
{

	printk("-> FIN PROCESO %d\n", p_proc_actual->id);
	
	liberar_proceso();

	return 0; /* no deber�a llegar aqui */
}


/*
	16 / 10 / 2018
*/
int sis_obtener_id_pr()
{
	int id_proceso_actual = p_proc_actual->id;
	printk("_> ID del proceso actual: %d\n", id_proceso_actual);
	return id_proceso_actual;
}

/*
	19 / 10 / 2018
*/
int sis_dormir()
{
	imp_cola(&lista_listos, "listos");

	int nivel = fijar_nivel_int(NIVEL_3);

	printk("[SIS_DORMIR()]\n");
	unsigned int segundos = (unsigned int)leer_registro(1);
	int ciclos = segundos * TICK;

	printk("\tSegundos: %u\tCiclos: %d\n", segundos, ciclos);
	printk("\tSe pone a dormir el proceso %d\n", p_proc_actual->id);

	p_proc_actual->estado = BLOQUEADO;
	p_proc_actual->ciclos_dormido = ciclos;
	eliminar_elem(&lista_listos, p_proc_actual);
	insertar_ultimo(&cola_bloqueados_dormir, p_proc_actual);

    BCP *proc_a_bloquear  = p_proc_actual;

	p_proc_actual = planificador();

	fijar_nivel_int(nivel);

	cambio_contexto(&(proc_a_bloquear->contexto_regs), &(p_proc_actual->contexto_regs));
	
	imp_cola(&lista_listos, "listos");
	imp_cola(&cola_bloqueados_dormir, "bloqueados dormir");
	
	printk("FIN [SIS_DORMIR()]\n");

	return 0;
}

/**
 * 23 / 10 / 2018
 */
int sis_crear_mutex()
{
	printk("[SIS_CREAR_MUTEX()\n");

	char *nombre = (char *)leer_registro(1);
	int tipo = (int)leer_registro(2);

	printk("\tArg1 (Nombre): %s, Arg2 (Tipo): %d\n", nombre, tipo);

	int lon = strlen(nombre);
	if (lon >= MAX_NOM_MUT)
	{
		printk("\tError creando el mutex: la longitud del nombre argumento(%d) es superior a %d", lon, MAX_NOM_MUT);
		return -1;
	}

	if (buscar_nombre_mutex(nombre) >= 0)
	{
		printk("\tError creando el mutex: ya existe un mutex con ese nombre\n");
		return -2;
	}

	int descriptor = buscar_descriptor_libre();
	if (descriptor < 0)
	{
		printk("\tError creando el mutex: no hay descriptores disponibles\n");
		return -3;
	}

	int pos_libre = buscar_mutex_libre();
	if (pos_libre < 0)
	{
		BCP *proceso_a_bloquear = p_proc_actual;
		
		proceso_a_bloquear->estado = BLOQUEADO;

		int nivel = fijar_nivel_int(NIVEL_3);

		eliminar_elem(&lista_listos, proceso_a_bloquear);
		insertar_ultimo(&cola_bloqueados_mutex_libre, proceso_a_bloquear);

		BCP *proceso_proximo = planificador();

		fijar_nivel_int(nivel);
		cambio_contexto(&(proceso_a_bloquear->contexto_regs), &(proceso_proximo->contexto_regs));
	}
	else
	{
		mutex nuevo_mutex = tabla_mutex[pos_libre];

		strcpy(nuevo_mutex.nombre, nombre);
		nuevo_mutex.estado = MUTEX_BLOQUEADO;
		nuevo_mutex.abierto = MUTEX_CERRADO;
		nuevo_mutex.tipo = tipo;
		nuevo_mutex.num_bloqueos = 0;
		nuevo_mutex.num_procesos_bloqueados = 0;
	}
	return descriptor;
}

int sis_abrir_mutex()
{
	/**
	 * 24 / 10 / 2018
	 */
	printk("[SIS_ABRIR_MUTEX()]\n");

	char *nombre = (char*)leer_registro(1);
	printk("\tArg1 (Nombre): %s\n", nombre);

	int descriptor = buscar_nombre_mutex(nombre);
	if (descriptor < 0)
	{
		printk("\tError abriendo el mutex: no existe ningún mutex llamado %s\n", nombre);
		return -1;
	}

	mutex mutex_a_abrir = tabla_mutex[descriptor];

	if (mutex_a_abrir.abierto == MUTEX_ABIERTO)
	{
		printk("\tError abriendo el mutex: el mutex %s(ID %d) ya está abierto\n", nombre, descriptor);
		return -2;
	}

	printk("\tSe ha abierto el mutex %s(ID %d)\n", nombre, descriptor);
	tabla_mutex[descriptor].abierto = MUTEX_ABIERTO;
	return descriptor;
}

int sis_lock_mutex()
{
	printk("[SIS_LOCK_MUTEX()]\n");

	unsigned int mutex_id = (unsigned int)leer_registro(1);

	printk("\tArg1 (Mutex ID): %u\n", mutex_id);

	mutex *mutex_lock = p_proc_actual->descriptores_mutex[mutex_id];

	if (mutex_lock == NULL)
	{
		int id = p_proc_actual->id;
		printk("\tError en lock: el mutex del proceso %d con descriptor %u no existe\n", id, mutex_id);
		return -1;
	}

	int tipo = mutex_lock->tipo;
	int estado = mutex_lock->estado;
	char *nombre = mutex_lock->nombre;

	if (tipo == NO_RECURSIVO && estado == MUTEX_BLOQUEADO)
	{
		printk("\tError en lock: el mutex no recursivo %s ya estaba bloqueado\n", nombre);
		printk("\tSe procede a bloquear el proceso %d\n", p_proc_actual->id);

		BCP *proceso_a_bloquear = p_proc_actual;
		
		proceso_a_bloquear->estado = BLOQUEADO;

		int nivel = fijar_nivel_int(NIVEL_3);

		eliminar_elem(&lista_listos, proceso_a_bloquear);
		insertar_ultimo(&cola_bloqueados_mutex_libre, proceso_a_bloquear);

		BCP *proceso_proximo = planificador();

		fijar_nivel_int(nivel);
		cambio_contexto(&(proceso_a_bloquear->contexto_regs), &(proceso_proximo->contexto_regs));
		return -2;
	}

	mutex_lock->estado = MUTEX_BLOQUEADO;
	mutex_lock->id_proc_bloq = p_proc_actual->id;
	printk("\tSe ha bloquedo el mutex %s\n", nombre);
	return 0;
}

int sis_unlock_mutex()
{
	printk("[SIS_UNLOCK_MUTEX()]\n");

	unsigned int mutex_id = (unsigned int)leer_registro(1);

	printk("\tArg1 (Mutex ID): %u\n", mutex_id);

	mutex *mutex_unlock = p_proc_actual->descriptores_mutex[mutex_id];

	if (mutex_unlock == NULL)
	{
		printk("\tError en unlock: el mutex con ID %u no existe\n", mutex_id);
		return -1;
	}

	if (mutex_unlock->abierto == MUTEX_CERRADO)
	{
		printk("\tError en unlock: el mutex %s no está abierto\n", mutex_unlock->nombre);
		return -2;
	}

	if (mutex_unlock->id_proc_bloq != p_proc_actual->id)
	{
		int id_proc = p_proc_actual->id;
		int id_mutex = mutex_unlock->id_proc_bloq;
		printk("\tError en unlock: el proceso %d no fue el último en realizar lock sobre el mutex, sino %d\n", id_proc, id_mutex);
		return -3;
	}

	mutex_unlock->estado = MUTEX_LIBRE;
	char *nombre = mutex_unlock->nombre;

	// HORRIPILANTE
	BCP *p_proc_bloq_mut = cola_bloqueados_mutex_lock.primero;
	while (p_proc_bloq_mut != NULL)
	{
		mutex *desc_mutex_bloq = p_proc_bloq_mut->descriptores_mutex;
		for (int i = 0; i != NUM_MUT_PROC; ++i)
		{
			mutex *mutex_proc_bloq = &desc_mutex_bloq[i];
			if (mutex_proc_bloq != NULL)
			{
				char *nombre_mutex_bloq = mutex_proc_bloq->nombre;
				if (strcmp(nombre_mutex_bloq, nombre))
				{
					// Desbloquear proceso
					BCP *proceso_a_desbloquear = p_proc_bloq_mut;
		
					proceso_a_desbloquear->estado = LISTO;
					mutex_unlock->id_proc_bloq = proceso_a_desbloquear->id;
					int nivel = fijar_nivel_int(NIVEL_3);

					eliminar_elem(&cola_bloqueados_mutex_lock, proceso_a_desbloquear);
					insertar_ultimo(&lista_listos, proceso_a_desbloquear);

					fijar_nivel_int(nivel);
					break;
				}
			}
			printk("\tEl break saltó aquí 1\n");
		}
		printk("\tEl break saltó aquí 2\n");
	}
	return 0;
}

int sis_cerrar_mutex()
{
	printk("[SIS_CERRAR_MUTES()]\n");

	unsigned int mutex_id = (unsigned int)leer_registro(1);

	printk("\tArg1 (Mutex ID): %u", mutex_id);

	mutex *mutex_cerrar = p_proc_actual->descriptores_mutex[mutex_id];

	if (mutex_cerrar == NULL)
	{
		printk("\tError cerrando el mutex: no existe el mutex con ID %u\n", mutex_id);
		return -1;
	}

	return 0;
}

/**
 * Implementacion funciones auxiliares mutex
 */
static void iniciar_tabla_mutex(){
	int i;
	for(i = 0; i < NUM_MUT; i++){
		tabla_mutex[i].id_proc_bloq = -1;
		tabla_mutex[i].estado = 0; /* indica que el mutex esta libre */
	}
}

static int buscar_descriptor_libre(){
	int i;
	for(i = 0; i < NUM_MUT_PROC; i++){
		if(p_proc_actual->descriptores_mutex[i] == NULL)
		{
			return i; /* devuelve el n˙mero del descriptor */
		}
	}
	return -1; /* no hay descriptor libre */
}

static int buscar_nombre_mutex(char *nombre_mutex){
	int i;
	for(i = 0; i < NUM_MUT; i++){
		if(strcmp(tabla_mutex[i].nombre, nombre_mutex) == 0)
			return i;	/* el nombre existe y devuelve su posicion en la tabla de mutex */
	}
	return -1; /* el nombre no existe */
}

static int buscar_mutex_libre(){
	int i;
	for(i = 0; i < NUM_MUT; i++){
		if(tabla_mutex[i].estado == 0)
			return i;	/* el mutex esta libre y devuelve su posicion en la tabla de mutex */
	}
	return -1; /* no hay mutex libre */
}

/*
 *
 * Rutina de inicializaci�n invocada en arranque
 *
 */
int main()
{
	/* se llega con las interrupciones prohibidas */

	instal_man_int(EXC_ARITM, exc_arit);
	instal_man_int(EXC_MEM, exc_mem);
	instal_man_int(INT_RELOJ, int_reloj);
	instal_man_int(INT_TERMINAL, int_terminal);
	instal_man_int(INT_SW, int_sw);
	instal_man_int(LLAM_SIS, tratar_llamsis);

	iniciar_cont_int();		/* inicia cont. interr. */
	iniciar_cont_reloj(TICK);	/* fija frecuencia del reloj */
	iniciar_cont_teclado();		/* inici cont. teclado */

	iniciar_tabla_proc();		/* inicia BCPs de tabla de procesos */
	iniciar_tabla_mutex();		// 23 / 10 / 2018

	/* crea proceso inicial */
	if (crear_tarea((void *)"init") < 0)
		panico("no encontrado el proceso inicial");
	
	/* activa proceso inicial */
	p_proc_actual=planificador();
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	panico("S.O. reactivado inesperadamente");
	return 0;
}
