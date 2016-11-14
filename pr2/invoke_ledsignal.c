#include <linux/errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>


#ifdef __i386__
#define __NR_LEDCTL 354
#else
#define __NR_LEDCTL 317
#endif

long ledctl(unsigned int mask){
	return (long) syscall(__NR_LEDCTL, mask);
}
int main(int argc, char* argv[]){
	unsigned int mask;
	int i, salir = 0;
	sscanf(argv[1], "0x%x", &mask);
	printf("%x\n", mask);
	ledctl(mask);
	return 0;
}
