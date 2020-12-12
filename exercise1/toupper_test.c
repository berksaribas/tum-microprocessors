#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <smmintrin.h>
#include <pmmintrin.h>
#include <pthread.h>
// Compile : nasm -felf64 -g toupper_avx2.asm && nasm -felf64 -g toupper_avx2_pthread.asm && gcc -pthread -g -no-pie toupper_test.c toupper_avx2.o toupper_avx2_pthread.o && ./a.out
// Compile with debug info : nasm -felf64 toupper_avx2.asm && nasm -felf64 toupper_avx2_pthread.asm && gcc -pthread -no-pie toupper_test.c toupper_avx2.o toupper_avx2_pthread.o && ./a.out
struct args {
	char* text;
	unsigned int len;
};
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

void toupper_optimised_berk(char * text) {
	int text_index = 0;
	int text_length = strlen(text);

	__m128i lower_limit = _mm_set1_epi8('a');
	__m128i upper_limit = _mm_set1_epi8('z');
	__m128i substract = _mm_set1_epi8(32);

	while(text_index < text_length) {
		__m128i loaded_text = _mm_loadu_si128((__m128i*) text);

		__m128i greater = _mm_cmpgt_epi8(loaded_text, lower_limit);
		__m128i lower = _mm_cmplt_epi8(loaded_text, upper_limit);

		__m128i mask = _mm_and_si128(substract, _mm_and_si128(greater, lower) );
		_mm_storeu_si128( (__m128i*) (text), _mm_sub_epi8(loaded_text, mask) );

		text_index += 16;
		text += 16;
	}
}

void toupper_optimised_yunus(char* text){
    toupper_avx2(text,strlen(text));
}

void toupper_optimised_yunus_pthread(char* text){
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
	unsigned int i;
	const unsigned int text_len = strlen(text);
	const unsigned int num_vectors = text_len / 32;
	const unsigned int vec_per_thread = num_vectors / ebx; 
	const unsigned int vec_per_thread_res = num_vectors % ebx;
    const unsigned int vec_len = vec_per_thread * 32;
	const unsigned int vec_len_res = vec_per_thread_res * 32;
	pthread_t* t_arr = (pthread_t*)malloc(ebx  * sizeof(pthread_t));
	struct args* args = (struct args*)malloc(ebx * sizeof(struct args));
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
	char* text_fin = text + (text_len - num_vectors * 32);
	for(int i = 0; text[i] != '\0'; i++) {
		if(text[i] >= 'a' && text[i] <= 'z') {
			text[i] -= 32;	
		}
	}
	for(i = 0; i < ebx; i++){
		pthread_join(t_arr[i], NULL);
	}
		pthread_join(t_residual, NULL);
	free(t_arr);
}

int main(){
   char m_str[] = "Here is a sample string,azzzsdjasdasd";
    char m_str2[] = "aaaaasasjzzzdasa";
    char m_str3[] = "Here is a sazxxwwxyyzmple string,asdjasdzzzwwzzxxasd";
    char m_str4[] = "HedHere is a sazxxwwxyyzmple string,asdjasdzzzwwzzxxasdHere is a sazxxwwxyyzmple string,asdjasdzzzwwzzxxasd";
    printf("Before naive: %s\n", m_str);
    toupper_simple(m_str);
    printf("After naive: %s\n", m_str);
	printf("Before yunus: %s\n", m_str3);
    toupper_optimised_yunus(m_str3);
    printf("After yunus: %s\n", m_str3);
    printf("Before berk: %s\n", m_str2);
    toupper_optimised_berk(m_str2);
    printf("After berk: %s\n", m_str2);
	printf("Before yunus_pthread: %s\n", m_str4);
	toupper_optimised_yunus_pthread(m_str4);
	printf("After yunus_pthread: %s\n", m_str4);

    return 0;
}

