#include<fontconfig/fontconfig.h>
#include<stdlib.h>
#include<stdio.h>
#include<stdint.h>
#include<string.h>
#include<stdbool.h>
#include<ctype.h>
#include<wchar.h>
#include<wctype.h>
#include<locale.h>
#include<unistd.h>
#include<getopt.h>

enum {
		OP_NONE = 0,
		OP_ALL,
		OP_SORT,
		OP_FONTFAMILY,
		OP_HELP,
		OP_END
};

char** whichfont(long int unicodepoint, char* argv[], int k_optind, int ops, const char* fontfamily){
	FcPattern *pattern;
	FcCharSet *charset;
	FcObjectSet	*os = 0;
	const FcChar8 *format = NULL;
	
	
	pattern = FcPatternCreate();
	if (!pattern)
	{
		printf ("Unable to parse the pattern\n");
		return NULL;
	}
	charset = FcCharSetCreate();
	
	FcCharSetAddChar(charset, (FcChar32) unicodepoint);
	FcPatternAddCharSet(pattern, FC_CHARSET, charset);

	while (argv[++k_optind])
	{
		if (!os)
		{
			os = FcObjectSetCreate ();
		}
		FcObjectSetAdd (os, argv[k_optind]);
	}
	
	if (ops == OP_FONTFAMILY && fontfamily) {
        	FcPatternAddString(pattern, FC_FAMILY, (FcChar8*)fontfamily);
    	}
    	else{
    		FcPatternAddString(pattern, FC_FAMILY, (FcChar8*)"sans-serif");
    	}

	FcConfigSubstitute (0, pattern, FcMatchPattern);
    	FcDefaultSubstitute (pattern);

	FcFontSet *fs;
	fs = FcFontSetCreate ();

	if(ops==OP_ALL || ops==OP_SORT){
		//with -a or -s
		FcResult font_result; //error handling if any, so we need this font_result
		FcFontSet *font_set;
		font_set = FcFontSort (0, pattern, (ops == OP_ALL) ? FcFalse : FcTrue, 0, &font_result);
		if (!font_set || font_set->nfont == 0) {
			printf("Font not found\n");
			return NULL;
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
	else{//for -f and if no option
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
	char **stringList = NULL;
	int stringCount = 0;

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
				stringList = realloc(stringList, (stringCount + 1) * sizeof(char *));
				stringList[stringCount] = strdup((const char *)s);
				stringCount++;
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
	stringList = realloc(stringList, (stringCount + 1) * sizeof(char *));
	stringList[stringCount] = NULL;
	return stringList;
}

char* wcharToString(int charvalue){
	//definin hash table
	const char *charMapping[] = {
		[0] = "\"\\0\"",
		[7] = "\"\\a\"",
		[8] = "\"\\b\"",
		[9] = "\"\\t\"",
		[10] = "\"\\n\"",
		[11] = "\"\\v\"",
		[12] = "\"\\f\"",
		[13] = "\"\\r\"",
		[27] = "\"\\e\"",
		[92] = "\\\\"	
	};
	if(charMapping[charvalue] != NULL){
		return strdup(charMapping[charvalue]);
	}
	else{
		return strdup("non-printable character");
	}
	
}

int main(int argc, char *argv[]){
	if (argc < 2){
		printf("Need argument UTF-8 character or unicode\n");
		printf("no argument is given\nexiting program...\n");
		return 1;
	}
	
	setlocale(LC_ALL, "");
	char *input_char = NULL;
	char **mystringList = NULL;
	char **mystringListCopy = NULL;
	char *fontfamily = NULL;
	int hexprint = 0;
	
	int ops = OP_NONE;

	struct option longopts[] = {
        {"all", no_argument, NULL, 'a'},
        {"sort", no_argument, NULL, 's'},
        {"font", no_argument, NULL, 'f'},
        {"unicode", no_argument, NULL, 'u'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    	};
    	
	

	int opt;
	while((opt = getopt_long(argc,argv, "asfuh", longopts, NULL)) != -1){
		switch (opt)
		{
		case 'a':
			if (ops==OP_SORT || ops==OP_FONTFAMILY || ops==OP_HELP){
				printf("-a is not acceptable with -s or -f or -h\n");
				return 1;
			}
			ops = OP_ALL;
			break;
		case 's':
			if (ops==OP_ALL || ops==OP_FONTFAMILY || ops==OP_HELP){
				printf("-s is not acceptable with -a or -f or -h\n");
				return 1;
			}
			ops = OP_SORT;
			break;
		case 'f':
			if (ops==OP_ALL || ops==OP_SORT || ops==OP_HELP){
				printf("-f is not acceptable with -a or -s or -h\n");
				return 1;
			}
			ops = OP_FONTFAMILY;
                	break;
                case 'u':
                	hexprint = 1;
                	break;
		case 'h':
			if (ops==OP_ALL || ops==OP_SORT || ops==OP_FONTFAMILY){
				printf("-h is not acceptable with -a or -s or -f\n");
				return 1;
			}
			ops = OP_HELP;
			break;
		default:
			printf("invalid option argument is there\n");
			return 1;
		}
	}
	
	if (ops == OP_HELP) {
		printf("Usage: whichfont [OPTION] <INPUT> [OTHER PARAMETERS]\n");
		printf("fontname feature usage: whichfont -f 'FONTNAME' <INPUT> [OTHER PARAMETERS]\n");
		printf("<Input>:\n");
		printf("  1. UTF-8 character or Unicode\n");
		printf("[Options]:\n");
		printf("  -a	--all		display all the available matches for the specified font attribute(s)\n");
		printf("  -s	--sort		display sorted list of matches\n");
		printf("  -f	--font		specify the fontname\n");
		printf("  -u	--unicode	display unicodepoint\n");
		printf("  -h	--help		display this help and exit\n");
		printf("If you wanna give other parameters then follow this page:\n");
		printf("https://www.freedesktop.org/software/fontconfig/fontconfig-devel/x19.html\n");
		printf("Learn more about whichfont: https://github.com/sudipshil9862/whichfont/blob/main/README.md\n");
		return 0;
	}
	
	
	
	int k_optind;
	k_optind = optind;
	
	if(ops == OP_FONTFAMILY){
		if(argc == k_optind){
			printf("input character or unicode is needed\n");
			return 1;
		}
		if(argc == k_optind+1){
			printf("please specify both fontname and unicode\n");
			return 1;
		}
	}
	else{
		if(argc == k_optind){
			printf("input character or unicode is needed\n");
			return 1;
		}
	}
	
	
	if(ops == OP_FONTFAMILY){
		fontfamily = (char *)malloc(strlen(argv[k_optind]) + 1); // +1 for the null terminator
		if (fontfamily != NULL) {
			strcpy(fontfamily, argv[k_optind]);
		} else {
			// handling memory allocation failure
			fprintf(stderr, "Memory allocation failed for fontfamily.\n");
			exit(EXIT_FAILURE);
		}
		k_optind++;
		//printf("k_optind: %d, characters: %s\n", k_optind, argv[k_optind]);
		//printf("argc: %d, k_optind: %d\n", argc, k_optind);
		input_char = argv[k_optind];
	}
	else{
		input_char = argv[k_optind];
	}
	
	//printf("input_char: %s\n", input_char);

	int len_inputchar = strlen(input_char);

	int has_digit = 0;
	int has_letter = 0;
	if(len_inputchar > 1){
		for (int i = 0; input_char[i] != '\0'; i++) {
			if (isdigit(input_char[i])) {
				has_digit = 1;
			} else if (isalpha(input_char[i])) {
				has_letter = 1;
			}
		}
	}

	bool hexBool =  len_inputchar >= 2 && (input_char[0] == '0' && (input_char[1] == 'x' || input_char[1] == 'X'));
	bool unicodeBool = len_inputchar >= 2 && input_char[1] == '+' && (input_char[0] == 'U' || input_char[0] == 'u');
	long int unicodepoint;
	if ((hexBool || unicodeBool) || (has_digit==1 && has_letter==0)){
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
			}
			
		}
	}	
	else
	{
		if(ops == OP_ALL || ops == OP_SORT){
			wchar_t wc;
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
				char **mystringList = whichfont((unsigned int) wc, argv, k_optind, ops, fontfamily);
				printf("\n");
				if (!iswprint((wint_t)wc)) {
					//printf("\"not a printable character\" ");
					char* charString = wcharToString((unsigned int)wc);
					printf("%s ", charString);
                			free(charString);
					//printf("\"\\x%02X\" ", (unsigned int)wc);
				}
				else{
					printf("\"%lc\" ", wc);
				}
				if(hexprint == 1){
					printf("unicode: U+%04X\n",(unsigned int) wc);
				}
				else{
					printf("\n");
				}
				int m = 0;
				while (mystringList[m]!=NULL) {
					printf("%s", mystringList[m]);
					free(mystringList[m]);
					m++;
				}
				free(mystringList);
				p += count;
			}
			return 0;
		}
		wchar_t wc;
		wchar_t* wcList = (wchar_t*) malloc(100 * sizeof(wchar_t));
		int wcCount = 0;
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
			mystringList = whichfont((unsigned int) wc, argv, k_optind, ops, fontfamily);
			
			if (mystringListCopy)
			{
				int areEqual = 1; // Assume they are equal
				for (int i = 0; mystringList[i] != NULL || mystringListCopy[i] != NULL; i++) {
					if (strcmp(mystringList[i], mystringListCopy[i]) != 0) {
						areEqual = 0; // Not equal
						break;
					}
				}
				if(!areEqual){
					for (int i = 0; i < wcCount; i++) {
        					printf("\n");
        					/*
						if (!iswprint((wint_t)wcList[i])) {
							//printf("\"not a printable character\" ");
							printf("\"\\x%02X\" ", (unsigned int)wc);
						}
						else{
							printf("\"%lc\" ", wcList[i]);
						}
						*/
						if (!iswprint((wint_t)wcList[i])) {
							char* charString = wcharToString((unsigned int)wcList[i]);
							printf("%s ", charString);
							free(charString);
						}
						else{
							printf("\"%lc\" ", wcList[i]);
						}
						if(hexprint == 1){
							printf("unicode: U+%04X",(unsigned int) wcList[i]);
						}
    					}
					printf("\n");
					int m = 0;
					while (mystringListCopy[m]) {
						printf("%s", mystringListCopy[m]);
						free(mystringListCopy[m]);
						m++;
					}
					free(mystringListCopy[m]);
					free(wcList);
					wcList = NULL;
					wcCount = 0;
					wcList = (wchar_t*) malloc(100 * sizeof(wchar_t));
					int stringCount = 0;
					mystringListCopy = realloc(mystringListCopy, (stringCount + 1) * sizeof(char *));
					while (mystringList[stringCount]) {
						mystringListCopy[stringCount] = strdup(mystringList[stringCount]);
						stringCount++;
					}
					mystringListCopy[stringCount] = NULL;
				}
			}
			else{
				int stringCount = 0;
				while (mystringList[stringCount]) {
					mystringListCopy = realloc(mystringListCopy, (stringCount + 1) * sizeof(char *));
					mystringListCopy[stringCount] = strdup(mystringList[stringCount]);
					stringCount++;
				}
				mystringListCopy[stringCount] = NULL;
			}
			wcList[wcCount++] = wc;
			p += count;
		}
		printf("\n");
		for (int i = 0; i < wcCount; i++) {
			/*
			if (!iswprint((wint_t)wcList[i])) {
				//printf("\"not a printable character\" ");
				printf("\"\\x%02X\" ", (unsigned int)wc);
			}
			else{
				printf("\"%lc\" ", wcList[i]);
			}
			*/
			if (!iswprint((wint_t)wcList[i])) {
				char* charString = wcharToString((unsigned int)wcList[i]);
				printf("%s ", charString);
				free(charString);
			}
			else{
				printf("\"%lc\" ", wcList[i]);
			}
			if(hexprint == 1){
				printf("unicode: U+%04X\n",(unsigned int) wcList[i]);
			}
			else{
				printf("\n");
			}
		}
		int m = 0;
		while (mystringListCopy[m]) {
			printf("%s", mystringListCopy[m]);
			free(mystringListCopy[m]);
			m++;
		}
		free(mystringList);
		free(mystringListCopy);
		free(wcList);
		return 0;
	}

	char *endptr;
	unicodepoint = strtol(input_char, &endptr, 16);
	if (endptr == input_char || *endptr != '\0' || unicodepoint < 0 || unicodepoint > (hexBool ? 0x7FFFFFFF : 0x10FFFF)) {
		printf("invalid unicodepoint\n");
		return 1;
	}
	mystringList = whichfont(unicodepoint, argv, k_optind, ops, fontfamily);
	int n = 0;
	while (mystringList[n]) {
		printf("%s", mystringList[n]);
		free(mystringList[n]);
		n++;
	}
	free(mystringList);
	return 0;
}
