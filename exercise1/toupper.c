#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include "options.h"
#include <smmintrin.h>
#include <pmmintrin.h>
#include <pthread.h>
struct args {
	char* text;
	unsigned int len;
};
void toupper_avx2(char* text, int len);
void* toupper_avx2_pthread(void* args);
int debug = 0;
double *results;
double *ratios;
unsigned long   *sizes;
int no_sz = 1, no_ratio =1, no_version=1;

static inline
double gettime(void) {
  	struct timeval tv;
    gettimeofday(&tv, NULL);

    return ((double) tv.tv_sec) + ((double) tv.tv_usec / (1000000.0));
}

static void toupper_simple(char * text) {
	for(int i = 0; text[i] != '\0'; i++) {
		if(text[i] >= 'a' && text[i] <= 'z') {
			text[i] -= 32;
		}
	}
	return;
}


static void toupper_optimised_berk(char * text) {
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

static void toupper_optimised_yunus(char * text) {
	 toupper_avx2(text,strlen(text));
}

static void toupper_optimised_otto(char * text) {
	int text_index = 0;
	int text_length = strlen(text);

	__m128i lower_limit = _mm_set1_epi8('a');
	__m128i upper_limit = _mm_set1_epi8('z');
	__m128i substract = _mm_set1_epi8(32);

	while(text_index < text_length) {
		__m128i loaded_text = _mm_load_si128((__m128i*) text);

		__m128i greater = _mm_cmpgt_epi8(loaded_text, lower_limit);
		__m128i lower = _mm_cmplt_epi8(loaded_text, upper_limit);

		__m128i mask = _mm_and_si128(substract, _mm_and_si128(greater, lower) );
		_mm_store_si128( (__m128i*) (text), _mm_sub_epi8(loaded_text, mask) );

		text_index += 16;
		text += 16;
	}
}

static void toupper_optimised_otto_prefetch(char * text) {
	int text_index = 0;
  char * text_end = text + strlen(text);

	__m128i lower_limit = _mm_set1_epi8('a');
	__m128i upper_limit = _mm_set1_epi8('z');
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
	free(args);
}


/*****************************************************************/


// align at 16byte boundaries
void* mymalloc(unsigned long int size)
{
     void* addr = malloc(size+32);
     return (void*)((unsigned long int)addr /16*16+16);
}

char createChar(int ratio){
	char isLower = rand()%100;

	// upper case=0, lower case=1
	if(isLower < ratio)
		isLower =0;
	else
		isLower = 1;

	char letter = rand()%26+1; // a,A=1; b,B=2; ...

	return 0x40 + isLower*0x20 + letter;

}

char * init(unsigned long int sz, int ratio){
    int i=0;
    char *text = (char *) mymalloc(sz+1);
    srand(1);// ensures that all strings are identical
    for(i=0;i<sz;i++){
			char c = createChar(ratio);
			text[i]=c;
	  }
    text[i] = '\0';
    return text;
}

/*
 * ******************* Run the different versions **************
 */

typedef void (*toupperfunc)(char *text);

void run_toupper(int size, int ratio, int version, toupperfunc f, const char* name)
{
   double start, stop;
		int index;

		index =  ratio;
		index += size*no_ratio;
		index += version*no_sz*no_ratio;

    char *text = init(sizes[size], ratios[ratio]);


    if(debug) printf("Before: %.40s...\n",text);

    start = gettime();
    (*f)(text);
    stop = gettime();
    results[index] = stop-start;

    if(debug) printf("After:  %.40s...\n",text);
}
// NOTE: Due to alignment requirements, # of test must be even!
struct _toupperversion {
    const char* name;
    toupperfunc func;
} toupperversion[] = {
    { "simple",    toupper_simple },
    { "optimised_berk", toupper_optimised_berk },
    { "optimised_yunus", toupper_optimised_yunus },
    { "optimised_otto", toupper_optimised_otto },
    { "optimised_otto_prefetch", toupper_optimised_otto_prefetch },
    { "optimised_yunus_threaded", toupper_optimised_yunus_pthread },
    { 0,0 },
	{ 0,0 }
};

void run(int size, int ratio)
{
	int v;
	for(v=0; toupperversion[v].func !=0; v++) {
		run_toupper(size, ratio, v, toupperversion[v].func, toupperversion[v].name);
	}
}

void printresults(){
	int i,j,k,index;
	printf("%s\n", OPTS);

	for(j=0;j<no_sz;j++){
		for(k=0;k<no_ratio;k++){
			printf("Size: %ld \tRatio: %f \tRunning time:", sizes[j], ratios[k]);
			for(i=0;i<no_version;i++){
				index =  k;
				index += j*no_ratio;
				index += i*no_sz*no_ratio;
				printf("\t%s: %f", toupperversion[i].name, results[index]);
			}
			printf("\n");
		}
	}
}

int main(int argc, char* argv[])
{
    unsigned long int min_sz=800000, max_sz = 0, step_sz = 10000;
		int min_ratio=50, max_ratio = 0, step_ratio = 1;
		int arg,i,j,v;
		int no_exp;

		for(arg = 1;arg<argc;arg++){
			if(0==strcmp("-d",argv[arg])){
				debug = 1;
			}
			if(0==strcmp("-l",argv[arg])){
					min_sz = atoi(argv[arg+1]);
					if(arg+2>=argc) break;
					if(0==strcmp("-r",argv[arg+2])) break;
					if(0==strcmp("-d",argv[arg+2])) break;
					max_sz = atoi(argv[arg+2]);
					step_sz = atoi(argv[arg+3]);
			}
			if(0==strcmp("-r",argv[arg])){
					min_ratio = atoi(argv[arg+1]);
					if(arg+2>=argc) break;
					if(0==strcmp("-l",argv[arg+2])) break;
					if(0==strcmp("-d",argv[arg+2])) break;
					max_ratio = atoi(argv[arg+2]);
					step_ratio = atoi(argv[arg+3]);
			}

		}
    for(v=0; toupperversion[v].func !=0; v++)
		no_version=v+1;
		if(0==max_sz)  no_sz =1;
		else no_sz = (max_sz-min_sz)/step_sz+1;
		if(0==max_ratio)  no_ratio =1;
		else no_ratio = (max_ratio-min_ratio)/step_ratio+1;
		no_exp = v*no_sz*no_ratio;
		results = (double *)malloc(sizeof(double[no_exp]));
		ratios = (double *)malloc(sizeof(double[no_ratio]));
		sizes = (long *)malloc(sizeof(long[no_sz]));

		for(i=0;i<no_sz;i++)
			sizes[i] = min_sz + i*step_sz;
		for(i=0;i<no_ratio;i++)
			ratios[i] = min_ratio + i*step_ratio;

		for(i=0;i<no_sz;i++)
			for(j=0;j<no_ratio;j++)
				run(i,j);

		printresults();
    return 0;
}
