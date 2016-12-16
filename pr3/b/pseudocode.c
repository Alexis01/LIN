mutex mtx;
condvar prod,cons;
int prod_count=0,cons_count=0;
cbuffer_t* cbuffer;

void fifoproc_open(bool abre_para_lectura) {
	lock(mtx);
	if( file->f_mode & FMODE_READ ){
		cons_count++;
		cond_signal(prod);
		while(prod_count == 0){
			cond_wait(cons,mtx);
		}
	}
	else{
		prod_count++;
		cond_signal(cons);
		while( cons_count == 0 ){
			cond_wait(prod,mtx);
		}
	}
	unlock(mtx);
}

int fifoproc_write(char* buff, int len) {
	char kbuffer[MAX_KBUF];

	if( len > MAX_CBUFFER_LEN || len > MAX_KBUF) { 
		return Error;
	}
	if( copy_from_user(kbuffer,buff,len) ) { 
		return Error;
	}
	lock(mtx);

	/* Esperar hasta que haya hueco para insertar (debe haber 	consumidores) */
	while( nr_gaps_cbuffer_t(cbuffer) < len && cons_count > 0 ){
		cond_wait(prod,mtx);
	}

	/* Detectar fin de comunicación por error (consumidor cierra FIFO antes) */
	if(cons_count==0){
		unlock(mtx); 
		return -EPIPE;
	}
	insert_items_cbuffer_t(cbuffer,kbuffer,len);
	/* Despertar a posible consumidor bloqueado */
	cond_signal(cons);
	unlock(mtx);
	return len;
}

int fifoproc_read(const char* buff, int len) {
	char kbuffer[MAX_KBUF];
	if( len > MAX_CBUFFER_LEN || len > MAX_KBUF){ 
		return Error;
	}
	lock(mtx);

	while( size_cbuffer_t(cbuffer) < len && prod_count > 0){
		cond_wait(cons,mtx);
	}

	if (prod_count==0 && size_cbuffer_t(cbuffer) == 0 ) {
		unlock(mtx); 
		return 0;
	}
	
	remove_items_cbuffer_t(cbuffer,kbuffer,len);

	/* Despertar a posible productor bloqueado */
	cond_signal(prod);
	unlock(mtx);

	if(copy_to_user(kbuffer,cbuffer,len)){
		 return Error;
	}

	return len;
}
void fifoproc_release(bool lectura) {
	
	lock(mtx);
	if(file->f_mode & FMODE_READ){
		cons_count--;
		cond_signal(prod);
	}
	else{
		prod_count--;
		cond_signal(cons);
	}
	
	if( prod_count == 0 && cons_count == 0 ){
		clear_cbuffer_t(cbuffer);
	}
	unlock(mtx);
}
