/**
 * Implementación del mutex 
/* Funciones auxiliares para mutex */

static void iniciar_tabla_mutex(){
	int i;
	for(i = 0; i < NUM_MUT; i++){
		tabla_mutex[i].estado = 0; /* indica que el mutex esta libre */
	}
}

static int buscar_descriptor_libre(){
	int i;
	for(i = 0; i < NUM_MUT_PROC; i++){
		if(p_proc_actual->descriptores_mutex[i] == NULL)
			return i; /* devuelve el n˙mero del descriptor */
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
 * Llamada al sistema crear_mutex
 * par·metro nombre en registro 1
 * par·metro tipo en registro 2
 */
int sis_crear_mutex(){
	char * nombre = (char*)leer_registro(1);
	int tipo = (int)leer_registro(2);
	if(strlen(nombre) > MAX_NOM_MUT){
		printk("(SIS_CREAR_MUTEX) Error: Nombre de mutex demasiado largo\n");
		return -1;
	}
	if(buscar_nombre_mutex(nombre) >= 0){
		printk("(SIS_CREAR_MUTEX) Error: Nombre de mutex ya existente\n");
		return -2;
	}
	int descr_mutex_libre = buscar_descriptor_libre();
	if(descr_mutex_libre < 0){
		printk("(SIS_CREAR_MUTEX) Error: No hay descriptores de mutex libres\n");
		return -3;
	}else{
		printk("(SIS_CREAR_MUTEX) Devolviendo descriptor numero %d\n", descr_mutex_libre);
	}
	int pos_mutex_libre = buscar_mutex_libre();
	if(pos_mutex_libre < 0){
		printk("(SIS_CREAR_MUTEX) No hay mutex libre, bloqueando proceso\n");
		// bloquear a la espera de que quede alguno libre
		BCP * proc_a_bloquear=p_proc_actual;	
		proc_a_bloquear->estado=BLOQUEADO; 					
		int nivel_int = fijar_nivel_int(3);
		//printk("(SIS_CREAR_MUTEX) Eliminando proceso con PID %d de la cola de listos\n", proc_a_bloquear->id);
		eliminar_primero(&lista_listos); 
		//printk("(SIS_CREAR_MUTEX) Insertando proceso con PID %d en la lista de bloqueados mutex libre\n", proc_a_bloquear-> id);
		insertar_ultimo(&lista_bloqueados_mutex_libre, proc_a_bloquear); 			
		p_proc_actual=planificador();
		fijar_nivel_int(nivel_int);
		cambio_contexto(&(proc_a_bloquear->contexto_regs),
			 	&(p_proc_actual->contexto_regs));
	}else{
		strcpy(tabla_mutex[pos_mutex_libre].nombre, nombre); // se copia el nombre al mutex
		tabla_mutex[pos_mutex_libre].tipo = tipo; // se asigna el tipo
		tabla_mutex[pos_mutex_libre].estado = 1; // se cambia el estado a OCUPADO
		tabla_mutex[pos_mutex_libre].num_bloqueos = 0;
		tabla_mutex[pos_mutex_libre].num_procesos_bloqueados = 0;
		p_proc_actual->descriptores_mutex[descr_mutex_libre] = &tabla_mutex[pos_mutex_libre]; // el descr apunta al mutex libre en la tabla
	}			
	return descr_mutex_libre;
}

