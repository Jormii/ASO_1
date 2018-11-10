/**
 * kernel/kernel.c
 * Minikernel version 1.0
 * Fernando Perez Costoya
 */

#include "kernel.h"	// Contiene definiciones usadas por este modulo
#include <string.h>

static void iniciar_tabla_proc()
{
	for (int i = 0; i != MAX_PROC; ++i)
	{
		tabla_procs[i].estado = NO_USADA;
	}
}

static int buscar_BCP_libre()
{
	for (int i = 0; i != MAX_PROC; ++i)
	{
		if (tabla_procs[i].estado == NO_USADA)
		{
			return i;
		}
	}
	return -1;
}

static void insertar_ultimo(lista_BCPs *lista, BCP *proc)
{
	if (lista->primero == NULL)
	{
		lista->primero = proc;
	}
	else
	{
		lista->ultimo->siguiente = proc;
	}
	lista->ultimo = proc;
	proc->siguiente = NULL;
}

static void eliminar_primero(lista_BCPs *lista)
{
	if (lista->ultimo == lista->primero)
	{
		lista->ultimo = NULL;
	}
	lista->primero = lista->primero->siguiente;
}

static void eliminar_elem(lista_BCPs *lista, BCP *proc)
{
	BCP *paux = lista->primero;

	if (paux == proc)
	{
		eliminar_primero(lista);
	}
	else 
	{
		for ( ; ((paux) && (paux->siguiente!=proc)); paux = paux->siguiente);
		{
			if (paux) 
			{
				if (lista->ultimo == paux->siguiente)
				{
					lista->ultimo = paux;
				}
				paux->siguiente = paux->siguiente->siguiente;
			}
		}
	}
}

static void espera_int()
{
	printk("[ESPERA_INT()]\n");
	printk("\tNo hay procesos en la cola de procesos listos. Esperando una interrupción\n");

	// Baja al minimo el nivel de interrupcion mientras espera
	int nivel = fijar_nivel_int(NIVEL_1);
	halt();
	fijar_nivel_int(nivel);
}

static BCP* planificador()
{
	while (cola_listos.primero==NULL)
	{
		// No hay nada que hacer
		espera_int();
	}
	return cola_listos.primero;
}

static void liberar_proceso()
{
	printk("[LIBERAR_PROCESO()]\n");

	// Cerrar los mutex implicados
	for (int i = 0; i != NUM_MUT_PROC; ++i)
	{
		mutex *mutex_cerrar = p_proc_actual->descriptores_mutex[i];
		if (mutex_cerrar != NULL)
		{
			if (mutex_cerrar->num_procesos_bloqueados > 0)
			{
				// Buscar un proceso al que otorgar el mutex
				BCP *p_proc_bloq_mutex = cola_bloqueados_mutex_lock.primero;
				int encontrado = 0;
				while (p_proc_bloq_mutex != NULL)
				{
					for (int i = 0; i != NUM_MUT_PROC; ++i)
					{
						if (p_proc_bloq_mutex->descriptores_mutex[i] != NULL)
						{
							if (p_proc_bloq_mutex->descriptores_mutex[i]->id_proc_bloq == p_proc_actual->id)
							{
								encontrado = 1;
								goto ENCONTRADO3;
							}
						}
					}
					p_proc_bloq_mutex = p_proc_bloq_mutex->siguiente;
				}

				ENCONTRADO3:
				// Desbloquear el proceso si se ha encontrado
				if (encontrado == 1)
				{
					int nivel = fijar_nivel_int(NIVEL_3);

					eliminar_elem(&cola_bloqueados_mutex_libre, p_proc_actual);
					insertar_ultimo(&cola_listos, p_proc_bloq_mutex);

					fijar_nivel_int(nivel);

					mutex_cerrar->num_procesos_bloqueados--;
					mutex_cerrar->id_proc_bloq = p_proc_bloq_mutex->id;
				}
				else
				{
					mutex_cerrar->id_proc_bloq = -1;
				}
				mutex_cerrar->estado = MUTEX_ESTADO_CREADO;
			}
			else
			{
				// Eliminar el mutex y liberar un proceso bloqueado
				strcpy(mutex_cerrar->nombre, "\0");
				mutex_cerrar->estado = MUTEX_ESTADO_LIBRE;
				mutex_cerrar->tipo = -1;
				mutex_cerrar->num_locks = 0;
				mutex_cerrar->num_procesos_bloqueados = 0;
				mutex_cerrar->id_proc_bloq = -1;

				BCP *p_proc = cola_bloqueados_mutex_libre.primero;
				if (p_proc != NULL)
				{
					int nivel = fijar_nivel_int(NIVEL_3);

					eliminar_elem(&cola_bloqueados_mutex_libre, p_proc);
					insertar_ultimo(&cola_listos, p_proc);

					fijar_nivel_int(nivel);
				}
			}
		}
	}

	liberar_imagen(p_proc_actual->info_mem);	// Liberar mapa de memoria

	p_proc_actual->estado = TERMINADO;
	eliminar_elem(&cola_listos, p_proc_actual); // Se elimina el proceso de la cola de listos

	// Se realiza el cambio de contexto
	BCP *p_proc_anterior = p_proc_actual;
	p_proc_actual = planificador();

	printk("\nEl proceso %d ha finalizado su ejecución. Cambio de contexto al proceso %d\n",
			p_proc_anterior->id, p_proc_actual->id);

	liberar_pila(p_proc_anterior->pila);
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	return;		// No se deberia llegar aqui
}

static void exc_arit()
{
	printk("[EXC_ARIT()]\n");
	if (!viene_de_modo_usuario())
	{
		panico("\tExcepción aritmética cuando se estaba dentro del kernel");
	}

	printk("\tExcepción aritmética producida por el proceso %d\n", p_proc_actual->id);
	liberar_proceso();
	return;		// No se deberia llegar aqui
}

static void exc_mem()
{
	printk("[EXC_MEM()]\n");
	if (!viene_de_modo_usuario())
	{
		panico("\tExcepción de memoria cuando se estaba dentro del kernel");
	}

	printk("\tExcepción de memoria producida por el proceso %d\n", p_proc_actual->id);
	liberar_proceso();
	return;		// No se deberia llegar aqui
}

static void int_terminal()
{
	printk("[INT_TERMINAL()]\n");
	char car = leer_puerto(DIR_TERMINAL);
	printk("\tTratando interrupción de terminal. Caracter: %c\n", car);
	return;
}

static void int_reloj()
{
	printk("[INT_RELOJ()]");
	printk("\tTratando interrupción de reloj\n");

	// Round robin
	if (cola_listos.primero != NULL)	// Evitar que el proceso esperando por interrupciones vuelva a ejecutarse
	{
		int id = p_proc_actual->id;
		int ciclos = p_proc_actual->ciclos_en_ejecucion;
		printk("\tAl proceso %d le restan %d ciclos en ejecución\n", id, ciclos);
		if (p_proc_actual->ciclos_en_ejecucion == 0)
		{
			int_sw();
		}
		p_proc_actual->ciclos_en_ejecucion--;
	}

	// Despertar a los procesos dormidos
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
			insertar_ultimo(&cola_listos, p_proc);

			p_proc = p_proximo;
		}
		else
		{
			p_proc->ciclos_dormido--;
			p_proc = p_proc->siguiente;
		}
	}
	return;
}

static void tratar_llamsis()
{
	// printk("[TRATAR_LLAMSIS()]\n");
	int res;
	int nserv = leer_registro(0);
	if (nserv < NSERVICIOS)
	{
		res = (tabla_servicios[nserv].fservicio)();
	}
	else
	{
		res = -1;		// Servicio no existente
	}
	escribir_registro(0, res);
	return;
}

static void int_sw()
{
	printk("[INT_SW()]\n");
	printk("\tTratando interrupción software\n");

	BCP *proceso_a_expulsar = p_proc_actual;
	proceso_a_expulsar->ciclos_en_ejecucion = TICKS_POR_RODAJA;
	printk("\tSe va a expulsar el proceso %d\n", proceso_a_expulsar->id);

	int nivel = fijar_nivel_int(NIVEL_3);

	eliminar_elem(&cola_listos, proceso_a_expulsar);
	insertar_ultimo(&cola_listos, proceso_a_expulsar);

	p_proc_actual = planificador();
	p_proc_actual->ciclos_en_ejecucion = TICKS_POR_RODAJA;

	insertar_ultimo(&cola_listos, proceso_a_expulsar);

	printk("\tEntra a ejecutarse el proceso %d\n", p_proc_actual->id);

	fijar_nivel_int(nivel);
	cambio_contexto(&(proceso_a_expulsar->contexto_regs), &(p_proc_actual->contexto_regs));
	return;
}

static int crear_tarea(char *programa)
{
	int proceso = buscar_BCP_libre();
	if (proceso == -1)
	{
		return -1;	// No hay entrada libre
	}

	// A rellenar el BCP
	BCP *p_proc = &(tabla_procs[proceso]);

	// Crea la imagen de memoria leyendo el ejecutable
	void *pc_inicial;
	void *imagen = crear_imagen(programa, &pc_inicial);
	if (imagen)
	{
		p_proc->info_mem = imagen;
		p_proc->pila = crear_pila(TAM_PILA);
		fijar_contexto_ini(p_proc->info_mem, p_proc->pila, TAM_PILA,
			pc_inicial,
			&(p_proc->contexto_regs));
		p_proc->id = proceso;
		p_proc->estado = LISTO;
		p_proc->ciclos_dormido = -1;
		p_proc->ciclos_en_ejecucion = TICKS_POR_RODAJA;

		insertar_ultimo(&cola_listos, p_proc);
		return 0;
	}
	else
		return -1;	// Fallo al crear imagen
}

int sis_crear_proceso()
{
	printk("[SIS_CREAR_PROCESO()]\n");

	printk("\tProceso %d. Creando proceso\n", p_proc_actual->id);
	char *prog = (char*)leer_registro(1);
	return crear_tarea(prog);
}

int sis_escribir()
{
	char *texto = (char*)leer_registro(1);
	unsigned int longi = (unsigned int)leer_registro(2);

	escribir_ker(texto, longi);
	return 0;
}

int sis_terminar_proceso()
{
	printk("[SIS_TERMINAR_PROCESO()]\n");
	printk("\tFin del proceso %d\n", p_proc_actual->id);
	
	liberar_proceso();
	return 0;	// No deberia llegar aqui
}

int sis_obtener_id_pr()
{
	printk("[SIS_OBTENER_ID_PR()]\n");

	int id_proceso_actual = p_proc_actual->id;
	printk("ID del proceso actual: %d\n", id_proceso_actual);
	return id_proceso_actual;
}

int sis_dormir()
{
	printk("[SIS_DORMIR()]\n");

	int nivel = fijar_nivel_int(NIVEL_3);

	unsigned int segundos = (unsigned int)leer_registro(1);
	int ciclos = segundos * TICK;

	printk("\tSegundos: %u\tCiclos: %d\n", segundos, ciclos);
	printk("\tSe pone a dormir el proceso %d\n", p_proc_actual->id);

	p_proc_actual->estado = BLOQUEADO;
	p_proc_actual->ciclos_dormido = ciclos;
	eliminar_elem(&cola_listos, p_proc_actual);
	insertar_ultimo(&cola_bloqueados_dormir, p_proc_actual);

    BCP *proc_a_bloquear  = p_proc_actual;

	p_proc_actual = planificador();

	fijar_nivel_int(nivel);

	cambio_contexto(&(proc_a_bloquear->contexto_regs), &(p_proc_actual->contexto_regs));
	return 0;
}

int sis_crear_mutex()
{
	printk("[SIS_CREAR_MUTEX()\n");

	char *nombre_mutex = (char*)leer_registro(1);
	int tipo_mutex = (int)leer_registro(2);

	printk("\tArg1 (Nombre): %s, Arg2 (Tipo): %d\n", nombre_mutex, tipo_mutex);

	int longitud_nombre = strlen(nombre_mutex) + 1;
	if (longitud_nombre > MAX_NOM_MUT)
	{
		printk("\tError creando el mutex: la longitud del nombre argumento (%d) es superior al permitido (%d)", longitud_nombre, MAX_NOM_MUT);
		return -1;
	}

	if (buscar_nombre_mutex(nombre_mutex) >= 0)
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

	// Descriptor del sistema
	int mutex_id = buscar_mutex_libre();
	if (mutex_id < 0)
	{
		BCP *proceso_a_bloquear = p_proc_actual;
		
		proceso_a_bloquear->estado = BLOQUEADO;

		int nivel = fijar_nivel_int(NIVEL_3);

		eliminar_elem(&cola_listos, proceso_a_bloquear);
		insertar_ultimo(&cola_bloqueados_mutex_libre, proceso_a_bloquear);

		p_proc_actual = planificador();

		fijar_nivel_int(nivel);
		cambio_contexto(&(proceso_a_bloquear->contexto_regs), &(p_proc_actual->contexto_regs));
	}
	else
	{
		mutex *nuevo_mutex = &(tabla_mutex[mutex_id]);

		strcpy(nuevo_mutex->nombre, nombre_mutex);
		nuevo_mutex->estado = MUTEX_ESTADO_CREADO;
		nuevo_mutex->tipo = tipo_mutex;
		nuevo_mutex->num_locks = 0;
		nuevo_mutex->num_procesos_bloqueados = 0;
		nuevo_mutex->id_proc_bloq = -1;
		
		p_proc_actual->descriptores_mutex[descriptor] = nuevo_mutex;
	}

	printk("\tDescriptor devuelto: %d\n", descriptor);
	return descriptor;
}

int sis_abrir_mutex()
{
	printk("[SIS_ABRIR_MUTEX()]\n");

	char *nombre_mutex = (char*)leer_registro(1);
	printk("\tArg1 (Nombre): %s\n", nombre_mutex);

	int mutex_id = buscar_nombre_mutex(nombre_mutex);
	if (mutex_id < 0)
	{
		printk("\tError abriendo el mutex: no existe ningún mutex llamado %s\n", nombre_mutex);
		return -1;
	}

	int descriptor = buscar_descriptor_libre();
	if (descriptor < 0)
	{
		printk("\tError abriendo el mutex: se ha alcanzado el límite de mutex por proceso\n", nombre_mutex, descriptor);
		return -2;
	}

	printk("\tSe ha abierto el mutex %s. Descriptor: %d\n", nombre_mutex, descriptor);
	p_proc_actual->descriptores_mutex[descriptor] = &(tabla_mutex[mutex_id]);
	return descriptor;
}

int sis_lock_mutex()
{
	printk("[SIS_LOCK_MUTEX()]\n");

	unsigned int descriptor = (unsigned int)leer_registro(1);

	printk("\tArg1 (Descriptor): %u\n", descriptor);

	mutex *mutex_lock = p_proc_actual->descriptores_mutex[descriptor];

	if (mutex_lock == NULL)
	{
		printk("\tError en lock: el mutex del proceso %d con descriptor %u no existe\n", p_proc_actual->id, descriptor);
		return -1;
	}

	if (mutex_lock->estado == MUTEX_ESTADO_BLOQUEADO && mutex_lock->id_proc_bloq != p_proc_actual->id)
	{
		mutex_lock->num_procesos_bloqueados++;

		BCP *proceso_a_bloquear = p_proc_actual;
		
		proceso_a_bloquear->estado = BLOQUEADO;

		int nivel = fijar_nivel_int(NIVEL_3);

		eliminar_elem(&cola_listos, proceso_a_bloquear);
		insertar_ultimo(&cola_bloqueados_mutex_lock, proceso_a_bloquear);

		p_proc_actual = planificador();

		fijar_nivel_int(nivel);
		cambio_contexto(&(proceso_a_bloquear->contexto_regs), &(p_proc_actual->contexto_regs));
	}

	switch (mutex_lock->tipo)
	{
		case MUTEX_TIPO_NO_RECURSIVO:
			if (mutex_lock->estado == MUTEX_ESTADO_BLOQUEADO)
			{
				printk("\tAviso: El mutex no recursivo con descriptor %u ya había sido bloqueado\n", descriptor);
				return -2;
			}
			break;
		case MUTEX_TIPO_RECURSIVO:
			mutex_lock->num_locks++;
			printk("\tNúmero de locks realizados sobre el mutex con descriptor %u: %d\n", descriptor, mutex_lock->num_locks);
			break;
		default:
			printk("\tError con el mutex %u: el valor de TIPO es extraño (%d)\n", descriptor, mutex_lock->tipo);
			break;
	}

	mutex_lock->estado = MUTEX_ESTADO_BLOQUEADO;
	mutex_lock->id_proc_bloq = p_proc_actual->id;
	return 0;
}

int sis_unlock_mutex()
{
	printk("[SIS_UNLOCK_MUTEX()]\n");

	unsigned int descriptor = (unsigned int)leer_registro(1);

	printk("\tArg1 (Descriptor): %u\n", descriptor);

	mutex *mutex_unlock = p_proc_actual->descriptores_mutex[descriptor];

	if (mutex_unlock == NULL)
	{
		printk("\tError en unlock: el mutex con descriptor %u no existe\n", descriptor);
		return -1;
	}

	if (mutex_unlock->estado != MUTEX_ESTADO_BLOQUEADO)
	{
		printk("\tError en unlock: el mutex con descriptor %u no está bloqueado\n", descriptor);
		return -2;
	}

	if (mutex_unlock->id_proc_bloq != p_proc_actual->id)
	{
		printk("\tError en unlock: el proceso %d puede hacer unlock sobre el mutex. Debe hacerlo %d\n", p_proc_actual->id, mutex_unlock->id_proc_bloq);
		return -3;
	}

	switch (mutex_unlock->tipo)
	{
		case MUTEX_TIPO_NO_RECURSIVO:
			break;
		case MUTEX_TIPO_RECURSIVO:
			mutex_unlock->num_locks--;
			mutex_unlock->num_locks = (mutex_unlock < 0) ? 0 : mutex_unlock->num_locks;
			break;
		default:
			printk("\tError con el mutex %u: el valor de TIPO es extraño (%d)\n", descriptor, mutex_unlock->tipo);
			break;
	}

	if (mutex_unlock->num_locks == 0 && mutex_unlock->num_procesos_bloqueados > 0)
	{
		// Buscar un proceso al que otorgar el mutex
		BCP *p_proc_bloq_mutex = cola_bloqueados_mutex_lock.primero;
		int encontrado = 0;
		while (p_proc_bloq_mutex != NULL)
		{
			for (int i = 0; i != NUM_MUT_PROC; ++i)
			{
				if (p_proc_bloq_mutex->descriptores_mutex[i] != NULL)
				{
					if (p_proc_bloq_mutex->descriptores_mutex[i]->id_proc_bloq == p_proc_actual->id)
					{
						encontrado = 1;
						goto ENCONTRADO;
					}
				}
			}
			p_proc_bloq_mutex = p_proc_bloq_mutex->siguiente;
		}

		ENCONTRADO:
		// Desbloquear el proceso si se ha encontrado
		if (encontrado == 1)
		{
			int nivel = fijar_nivel_int(NIVEL_3);

			eliminar_elem(&cola_bloqueados_mutex_libre, p_proc_actual);
			insertar_ultimo(&cola_listos, p_proc_bloq_mutex);

			fijar_nivel_int(nivel);

			mutex_unlock->num_procesos_bloqueados--;
			mutex_unlock->id_proc_bloq = p_proc_bloq_mutex->id;
		}
		else
		{
			mutex_unlock->id_proc_bloq = -1;
		}
		mutex_unlock->estado = MUTEX_ESTADO_CREADO;
	}

	return 0;
}

int sis_cerrar_mutex()
{
	printk("[SIS_CERRAR_MUTEX()]\n");

	unsigned int descriptor = (unsigned int)leer_registro(1);

	printk("\tArg1 (Descriptor): %u\n", descriptor);
	
	mutex *mutex_cerrar = &(tabla_mutex[descriptor]);

	if (mutex_cerrar == NULL)
	{
		printk("\tError cerrando el mutex: no existe el mutex con descriptor %u\n", descriptor);
		return -1;
	}

	if (mutex_cerrar->num_procesos_bloqueados > 0)
	{
		// Buscar un proceso al que otorgar el mutex
		BCP *p_proc_bloq_mutex = cola_bloqueados_mutex_lock.primero;
		int encontrado = 0;
		while (p_proc_bloq_mutex != NULL)
		{
			for (int i = 0; i != NUM_MUT_PROC; ++i)
			{
				if (p_proc_bloq_mutex->descriptores_mutex[i] != NULL)
				{
					if (p_proc_bloq_mutex->descriptores_mutex[i]->id_proc_bloq == p_proc_actual->id)
					{
						encontrado = 1;
						goto ENCONTRADO2;
					}
				}
			}
			p_proc_bloq_mutex = p_proc_bloq_mutex->siguiente;
		}

		ENCONTRADO2:
		// Desbloquear el proceso si se ha encontrado
		if (encontrado == 1)
		{
			int nivel = fijar_nivel_int(NIVEL_3);

			eliminar_elem(&cola_bloqueados_mutex_libre, p_proc_actual);
			insertar_ultimo(&cola_listos, p_proc_bloq_mutex);

			fijar_nivel_int(nivel);

			mutex_cerrar->num_procesos_bloqueados--;
			mutex_cerrar->id_proc_bloq = p_proc_bloq_mutex->id;
		}
		else
		{
			mutex_cerrar->id_proc_bloq = -1;
		}
		mutex_cerrar->estado = MUTEX_ESTADO_CREADO;
	}
	else
	{
		// Eliminar el mutex y liberar un proceso bloqueado
		strcpy(mutex_cerrar->nombre, "\0");
		mutex_cerrar->estado = MUTEX_ESTADO_LIBRE;
		mutex_cerrar->tipo = -1;
		mutex_cerrar->num_locks = 0;
		mutex_cerrar->num_procesos_bloqueados = 0;
		mutex_cerrar->id_proc_bloq = -1;

		BCP *p_proc = cola_bloqueados_mutex_libre.primero;
		if (p_proc != NULL)
		{
			int nivel = fijar_nivel_int(NIVEL_3);

			eliminar_elem(&cola_bloqueados_mutex_libre, p_proc);
			insertar_ultimo(&cola_listos, p_proc);

			fijar_nivel_int(nivel);
		}
	}
	p_proc_actual->descriptores_mutex[descriptor] = NULL;
	return 0;
}

static void iniciar_tabla_mutex()
{
	for(int i = 0; i != NUM_MUT; ++i)
	{
		tabla_mutex[i].estado = MUTEX_ESTADO_LIBRE;
		tabla_mutex[i].id_proc_bloq = -1;
	}
}

static int buscar_descriptor_libre()
{
	for (int i = 0; i != NUM_MUT_PROC; ++i)
	{
		if (p_proc_actual->descriptores_mutex[i] == NULL)
		{
			return i; // Devuelve el numero del descriptor
		}
	}
	return -1; // No hay descriptor libre
}

static int buscar_nombre_mutex(char *nombre_mutex)
{
	for (int i = 0; i != NUM_MUT; ++i)
	{
		if (strcmp(tabla_mutex[i].nombre, nombre_mutex) == 0)
		{
			return i;	// El nombre existe y devuelve su posicion (descriptor) en la tabla de mutex
		}
	}
	return -1; // El nombre no existe
}

static int buscar_mutex_libre()
{
	for (int i = 0; i != NUM_MUT; ++i)
	{
		if (tabla_mutex[i].estado == MUTEX_ESTADO_LIBRE)
			return i;	// El mutex esta libre y devuelve su posicion en la tabla de mutex
	}
	return -1; // No hay mutex libre
}

// Rutina de inicializacion invocada en el arranque
int main()
{
	// Se llega con las interrupciones prohibidas (NIVEL_3)
	instal_man_int(EXC_ARITM, exc_arit);
	instal_man_int(EXC_MEM, exc_mem);
	instal_man_int(INT_RELOJ, int_reloj);
	instal_man_int(INT_TERMINAL, int_terminal);
	instal_man_int(INT_SW, int_sw);
	instal_man_int(LLAM_SIS, tratar_llamsis);

	iniciar_cont_int();			// Inicializa el controlador de interrupciones
	iniciar_cont_reloj(TICK);	// Fija frecuencia del reloj
	iniciar_cont_teclado();		// Inicializa el controlador de teclado

	iniciar_tabla_proc();		// Inicia BCPs de tabla de procesos
	iniciar_tabla_mutex();		// Inicia mutex

	// Crea el proceso inicial
	if (crear_tarea((void *)"init") < 0)
		panico("No se ha encontrado el proceso inicial");
	
	// Activa proceso inicial
	p_proc_actual = planificador();
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	panico("SO reactivado inesperadamente");
	return 0;
}
