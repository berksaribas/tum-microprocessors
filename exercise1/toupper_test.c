#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <smmintrin.h>
#include <pmmintrin.h>
#include <pthread.h>
#include <assert.h>
// Compile : nasm -felf64 -g toupper_avx2.asm && nasm -felf64 -g toupper_avx2_pthread.asm && gcc -fopenmp -pthread -g -no-pie toupper_test.c toupper_avx2.o toupper_avx2_pthread.o && ./a.out
// Compile with debug info : nasm -felf64 toupper_avx2.asm && nasm -felf64 toupper_avx2_pthread.asm && gcc -fopenmp -pthread -no-pie toupper_test.c toupper_avx2.o toupper_avx2_pthread.o && ./a.out
struct args {
	char* text;
	unsigned int len;
};
#define NUM_THREADS 8
#define THREAD_INFO_STATIC
void toupper_avx2(char* text, int len);
void* toupper_avx2_pthread(void* args);

void toupper_simple(char * text) {
	for(int i = 0; text[i] != '\0'; i++) {
		if(text[i] >= 'a' && text[i] <= 'z') {
			text[i] -= 32;	
		}
	}
	return;
}

static void toupper_optimised_berk(char * text) {
	int text_length = strlen(text);
	int iterations = text_length / 16;

	__m128i lower_limit = _mm_set1_epi8('a' - 1);
	__m128i upper_limit = _mm_set1_epi8('z' + 1);
	__m128i substract = _mm_set1_epi8(32);

	for(int i = 0; i < iterations; i++) {
		__m128i loaded_text = _mm_loadu_si128((__m128i*) text);

		__m128i greater = _mm_cmpgt_epi8(loaded_text, lower_limit);
		__m128i lower = _mm_cmplt_epi8(loaded_text, upper_limit);

		__m128i mask = _mm_and_si128(substract, _mm_and_si128(greater, lower) );
		_mm_storeu_si128( (__m128i*) (text), _mm_sub_epi8(loaded_text, mask) );

		text += 16;
	}

	int remainder = text_length % 16;
	for(int i = 0; i < remainder; i++) {
		if(text[i] >= 'a' && text[i] <= 'z') {
			text[i] -= 32;
		}
	}
}

static void toupper_optimised_berk_openmp(char * text) {
	int text_length = strlen(text);

	__m128i lower_limit = _mm_set1_epi8('a' - 1);
	__m128i upper_limit = _mm_set1_epi8('z' + 1);
	__m128i substract = _mm_set1_epi8(32);

	int iterations = text_length / 16;

	#pragma omp parallel for schedule(static)
	for(int i = 0; i < iterations; i++) {
		__m128i loaded_text = _mm_loadu_si128((__m128i*) (text + i*16) );

		__m128i greater = _mm_cmpgt_epi8(loaded_text, lower_limit);
		__m128i lower = _mm_cmplt_epi8(loaded_text, upper_limit);

		__m128i mask = _mm_and_si128(substract, _mm_and_si128(greater, lower) );
		_mm_storeu_si128( (__m128i*) (text + i*16), _mm_sub_epi8(loaded_text, mask) );
	}

	int remainder = text_length % 16;
	int curr_index = iterations * 16;
	for(int i = 0; i < remainder; i++) {
		if(text[curr_index + i] >= 'a' && text[curr_index + i] <= 'z') {
			text[curr_index + i] -= 32;
		}
	}
}

static void toupper_optimised_yunus(char * text) {
	 toupper_avx2(text,strlen(text));
}

static void toupper_optimised_otto_prefetch(char * text) {
	int text_index = 0;
  	char * text_end = text + strlen(text);

	__m128i lower_limit = _mm_set1_epi8('a' - 1);
	__m128i upper_limit = _mm_set1_epi8('z' + 1);
	__m128i substract = _mm_set1_epi8(32);

	while(text < text_end) {
    	__builtin_prefetch(text + 16 * 25);

		__m128i loaded_text = _mm_load_si128((__m128i*) text);

		__m128i greater = _mm_cmpgt_epi8(loaded_text, lower_limit);
		__m128i lower = _mm_cmplt_epi8(loaded_text, upper_limit);

		__m128i mask = _mm_and_si128(substract, _mm_and_si128(greater, lower) );
		_mm_store_si128( (__m128i*) (text), _mm_sub_epi8(loaded_text, mask) );

		text += 16;
	}
}

static void toupper_optimised_yunus_pthread(char* text){
#ifndef THREAD_INFO_STATIC
	unsigned int eax=11,ebx=0,ecx=1,edx=0;
	asm volatile("cpuid"
        : "=a" (eax),
          "=b" (ebx),
          "=c" (ecx),
          "=d" (edx)
        : "0" (eax), "2" (ecx)
        : 
	);
	//printf("Cores: %d\nThreads: %d\nActual thread: %d\n",eax,ebx,edx);
#else
	const int ebx = NUM_THREADS;
#endif
	unsigned int i;
	const unsigned int text_len = strlen(text);
	const unsigned int num_vectors = text_len / 32;
	const unsigned int vec_per_thread = num_vectors / ebx; 
	const unsigned int vec_per_thread_res = num_vectors % ebx;
    const unsigned int vec_len = vec_per_thread * 32;
	const unsigned int vec_len_res = vec_per_thread_res * 32;
#ifndef THREAD_INFO_STATIC
	pthread_t* t_arr = (pthread_t*)malloc(ebx  * sizeof(pthread_t));
	struct args* args = (struct args*)malloc(ebx * sizeof(struct args));
#else
	struct args args[8]; 
	pthread_t t_arr[8]; 
#endif
	pthread_t t_residual;
	for(i = 0; i < ebx; i++) {
		args[i].len = vec_len;
		args[i].text = text + i * vec_len;
		pthread_create(&t_arr[i], NULL, toupper_avx2_pthread, (void*) &args[i]);
	}
	struct args res_arg;
	res_arg.len = vec_len_res;
	res_arg.text = text + i * vec_len;
	pthread_create(&t_residual, NULL, toupper_avx2_pthread, (void*) &res_arg);
	
	
	// int rem = text_len - i * vec_len - vec_len_res;
	// printf("Rem: %d, Len:  %d\n", rem, text_len);
	// assert( rem < 32 );
	char* text_fin = res_arg.text + vec_len_res;
	for(int i = 0; text_fin[i] != '\0'; i++) {
		if(text_fin[i] >= 'a' && text_fin[i] <= 'z') {
			text_fin[i] -= 32;	
		}
	}

	for(i = 0; i < ebx; i++){
		pthread_join(t_arr[i], NULL);
	}
		pthread_join(t_residual, NULL);
#ifndef THREAD_INFO_STATIC
	free(t_arr);
	free(args);
#endif
}

int main() {
	char m_str[] = "Here is a sample string,azzzsdjasdasd";
    char m_str2[] = "aaaaasasjzzzdasa";
    char m_str3[] = "Here is a sazxxwwxyyzmple string,asdjasdzzzwwzzxxasd";
    char m_str4[] = "HedHere is a sazxxwwxyyzmple string,asdjasdzzzwwzzxxasdHere is a sazxxwwxyyzmple string,asdjasdzzzwwzzxxasd";
    char m_str5[] = "HedHere is a sazxxwwxyyzmple string,asdjasdzzzwwzzxxasdHere is a sazxxwwxyyzmple string,asdjasdzzzwwzzxxasd";
	// NOTE: If you want to test yunus' implementations you need to update asm code to load data in unaligned way
	// For example by changing vmovdqa to vmovdqu
	// (Main driver code allocates in aligned way, so alligned loads are the canon )
    printf("Before naive: %s\n", m_str);
    toupper_simple(m_str);
    printf("After naive: %s\n", m_str);
	printf("Before yunus: %s\n", m_str3);
    //toupper_optimised_yunus(m_str3);
    printf("After yunus: %s\n", m_str3);
    printf("Before berk: %s\n", m_str2);
    toupper_optimised_berk(m_str2);
    printf("After berk: %s\n", m_str2);
	printf("Before yunus_pthread: %s\n", m_str4);
	//toupper_optimised_yunus_pthread(m_str4);
	printf("After yunus_pthread: %s\n", m_str4);
	printf("Before otto: %s\n", m_str5);
	toupper_optimised_otto_prefetch(m_str5);
	printf("After otto: %s\n", m_str5);

    return 0;
}

