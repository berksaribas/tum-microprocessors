#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <smmintrin.h>
#include <pmmintrin.h>
// Compile : nasm -felf64 -g  toupper_avx2.asm && gcc -g -no-pie toupper_test.c toupper_avx2.o && ./a.out
void toupper_avx2(char* text, int len);

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

static void toupper_optimised_yunus(char* text){
    toupper_avx2(text,strlen(text));
}

int main(){
    char m_str[] = "Here is a sample string,asdjasdasd";
    char m_str2[] = "aaaaasasjdasa";
    char m_str3[] = "Here is a sample string,asdjasdasd";
    printf("Before naive: %s\n", m_str);
    toupper_simple(m_str);
    printf("After naive: %s\n", m_str);
	printf("Before yunus: %s\n", m_str3);
    toupper_optimised_yunus(m_str3);
    printf("After yunus: %s\n", m_str3);
    printf("Before berk: %s\n", m_str2);
    toupper_optimised_berk(m_str2);
    printf("After berk: %s\n", m_str2);

    return 0;
}

