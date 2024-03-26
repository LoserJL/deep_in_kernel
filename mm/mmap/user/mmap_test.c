#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define FILE_MAPED	"/dev/mmap_test"

int main(void)
{
	int fd;
	char *v_addr;
	int i;

	fd = open(FILE_MAPED, O_RDWR);
	if (fd < 0) {
		printf("Failed to open %s file\n", FILE_MAPED);
		return -1;
	}

	v_addr = (char *)mmap(NULL, 4096, PROT_READ | PROT_WRITE,
					MAP_PRIVATE, fd, 0);
	if (v_addr == MAP_FAILED)
		perror("mmap");

	for (i = 0; i < 100; i++)
		printf("v_addr[%d] = %d\n", i, v_addr[i]);

	return 0;
}