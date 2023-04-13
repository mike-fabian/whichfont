#include<fontconfig/fontconfig.h>
#include<stdlib.h>
#include<stdio.h>
#include<stdint.h>
#include<string.h>
#include<stdbool.h>
#include<ctype.h>
#include<wchar.h>
#include<locale.h>
#include<unistd.h>
#include<getopt.h>

void whichfont(char* unicode_result, char* argv[], int defaultFamily, int all, int sort){
	FcPattern *pattern;
	FcCharSet *charset;
	FcObjectSet	*os = 0;
	const FcChar8 *format = NULL;
	int i;
	if(all || sort){
		i = 2;
	}
	else{
		i = 1;
	}

	if(argv[i]){
		pattern = FcPatternCreate();
		charset = FcCharSetCreate();
		FcCharSetAddChar(charset, (FcChar32) strtol(unicode_result, NULL, 16));
		FcPatternAddCharSet(pattern, FC_CHARSET, charset);

		if (!pattern)
		{
			printf ("Unable to parse the pattern\n");
			return;
		}
		while (argv[++i])
		{
			if (!os)
			{
				os = FcObjectSetCreate ();
			}
			FcObjectSetAdd (os, argv[i]);
		}
	}
	else{
		pattern = FcPatternCreate();
	}

	if(!pattern){
		printf ("Unable to parse the pattern\n");
		return;
	}

	FcConfigSubstitute (0, pattern, FcMatchPattern);
    FcDefaultSubstitute (pattern);

	FcFontSet *fs;
	fs = FcFontSetCreate ();

	if(all || sort){
		//with -a or -s
		FcResult font_result; //error handling if any, so we need this font_result
		FcFontSet *font_set;
		if(all){
			font_set = FcFontSort (0, pattern, FcFalse, 0, &font_result);
		}
		else if (sort)
		{
			font_set = FcFontSort (0, pattern, FcTrue, 0, &font_result);
		}
		if (!font_set || font_set->nfont == 0) {
			printf("Font not found\n");
			return;
		}

		int j;
		for (j = 0; j < font_set->nfont; j++)
		{
			FcPattern  *font_pattern;

			font_pattern = FcFontRenderPrepare (NULL, pattern, font_set->fonts[j]);
			if (font_pattern)
				FcFontSetAdd (fs, font_pattern);
		}
		FcFontSetDestroy(font_set);
	}
	else{
		//best one font
		FcPattern   *match;
		FcResult result;
		match = FcFontMatch(0, pattern, &result);
		if(match){
			FcFontSetAdd(fs, match);
		}
	}

	FcCharSetDestroy(charset);
	FcPatternDestroy(pattern);

	if (!format)
	{
		if (os){
			format = (const FcChar8 *) "%{=unparse}\n";
		}
		else{
			format = (const FcChar8 *) "%{=fcmatch}\n";
		}
	}

	//common code
	if (fs)
	{
		int	j;
		for (j = 0; j < fs->nfont; j++)
		{
			FcPattern *font;
			font = FcPatternFilter (fs->fonts[j], os);
			FcChar8 *s;
			s = FcPatternFormat (font, format);
			if(s){
				printf("%s", s);
				FcStrFree(s);
			}
			FcPatternDestroy (font);
		}
		FcFontSetDestroy (fs);
	}

	if(os){
		FcObjectSetDestroy(os);
	}
	FcFini();
}

int main(int argc, char *argv[]){
	if (argc < 2){
		printf("Need argument UTF-8 character or hex along with %s\n", argv[0]);
		printf("no argument is given\nexiting program...\n");
		return 1;
	}
	setlocale(LC_ALL, "");
	char *input_char = NULL;
	int defaultFamily = 0; //sans-serif
	int all = 0; //-a
	int sort = 0; //-s
	
	int opt;
	while((opt = getopt(argc,argv, "as")) != -1){
		switch (opt)
		{
		case 'a':
			all = 1;
			printf("-a argument is there\n");
			break;
		case 's':
			sort = 1;
			printf("-s argument is there\n");
			break;
		default:
			printf("invalid option argument is there\n");
			return 0;
		}
	}
	
	if(all || sort){
		if(argv[2]){
			input_char = argv[2];
		}
	}
	else if(optind < argc){
		input_char = argv[1];
	}
	else {
    	printf("No input argument found\n");
	}

	int len_inputchar = strlen(input_char);

	int has_digit = 0;
	int has_letter = 0;
	for (int i = 0; input_char[i] != '\0'; i++) {
		if (isdigit(input_char[i])) {
			has_digit = 1;
		} else if (isalpha(input_char[i])) {
			has_letter = 1;
		}
	}

	bool hexBool =  len_inputchar >= 2 && (input_char[0] == '0' && (input_char[1] == 'x' || input_char[1] == 'X'));
	bool unicodeBool = len_inputchar >= 2 && input_char[1] == '+' && (input_char[0] == 'U' || input_char[0] == 'u');

	if(hexBool || unicodeBool){
		input_char += 2;
		int len_input = strlen(input_char);

		if (len_input == 0) { //when input_char=0x or input_char=U+ then it should return false
			printf("empty input argument\n");
			return 0;
		}

		if (hexBool)
		{
			//input is hexadecimal
			if(len_input > 8){
				printf("invalid hexadecimal value\n");
				return 0;
			}
			for (int i = 0; i < len_input; i++) {
				if (!isxdigit(input_char[i])) {
					printf("invalid hexadecimal value\n");
					return 0;
				}
			}
			
			char *endptr;
			long int codepoint = strtol(input_char, &endptr, 16);
			if (endptr == input_char || *endptr != '\0' || codepoint < 0 || codepoint > 0x7FFFFFFF) {
				printf("invalid hexadecimal value\n");
				return 0;
			}
			
		}
		else if (unicodeBool)
		{
			// input is unicode
			
			char *endptr;
			long int codepoint = strtol(input_char, &endptr, 16);
			if (endptr == input_char || *endptr != '\0' || codepoint < 0 || codepoint > 0x10FFFF)
			{
				printf("%s is invalid Unicode code point\n", input_char);
				return 0;
			}
			
		}
		/*
		char *endptr;
		long int codepoint = strtol(input_char, &endptr, 16);
		if (endptr == input_char || *endptr != '\0' || codepoint < 0 || codepoint > 0x7FFFFFFF) {
			printf("invalid hexadecimal value\n");
			return 0;
		}
		*/
	}
	else if (has_digit==1 && has_letter==0)
	{
		//input is unicode
		char *endptr;
		long int codepoint = strtol(input_char, &endptr, 16);
		if (endptr == input_char || *endptr != '\0' || codepoint < 0 || codepoint > 0x10FFFF) {
			printf("%s is invalid Unicode code point\n", input_char);
			return 1;
		}
	}
	else
	{
		wchar_t wc;
		char* unicode_result = (char*) malloc(5 * sizeof(char));
		char* p = input_char;
		while (*p) {
			int count = mbtowc(&wc, p, MB_CUR_MAX);
			if (count < 0) {
				fprintf(stderr, "Error: invalid multibyte sequence\n");
				return 1;
			} else if (count == 0) {
				fprintf(stderr, "Error: unexpected end of string\n");
				return 1;
			}
			sprintf(unicode_result, "%04X", (unsigned int) wc);
			printf("\n");
			printf("Character: %lc\n", wc);
			printf("\n");
			whichfont(unicode_result, argv, defaultFamily, all, sort);
			p += count;
		}
		free(unicode_result);
		return 0;
	}
	whichfont(input_char, argv, defaultFamily, all, sort);
	return 0;
}
