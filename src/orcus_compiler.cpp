#include "orcus_compiler.hpp"
#include "orcus_bytecode.hpp"
#include "orcus.hpp"
#include <malloc.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

enum comment_type:uint8_t {
	COMMENT_NONE=0,
	COMMENT_LINE=1,
	COMMENT_MULTI=2,
};

enum token_flags {
	TOKEN_OPERATOR = 1 << 1,
	TOKEN_UNARY_OP = 1 << 2,
	TOKEN_BINARY_OP = 1 << 3,
	TOKEN_MISC_OP = 1 << 4,
	TOKEN_SELFMODIFY_OP = 1 << 5,
	TOKEN_VARIABLE = 1 << 6,
	TOKEN_TAKES_STATIC_NUM = 1 << 7,
	TOKEN_TAKES_STATIC_STRING = 1 << 8,
	TOKEN_STATIC_NUMBER = 1 << 9,
	TOKEN_STATIC_STRING = 1 << 10,
	TOKEN_STATIC_BOOLEAN = 1 << 11,
	TOKEN_STATIC_NIL = 1 << 12,
	TOKEN_ENV_VAR = 1 << 13,
	TOKEN_KEYWORD = 1 << 14,
	TOKEN_SCOPESPECIFIER = 1 << 15,
	TOKEN_LOOP_MODIFIER = 1 << 16,
};


enum compiler_error_codes {
	ORCUS_COMPERR_MEMORY_ALLOCATION,
	ORCUS_COMPERR_NO_MORE_TOKENS,

	ORCUS_COMPERR_MALFORMED_NUMBER,
	ORCUS_COMPERR_WRONG_SYMBOL,
	ORCUS_COMPERR_NO_OPERATION_FOUND, //if we don't do anything with a var, so instead of a=5, we just do a

	ORCUS_COMPERR_WHILE_NEEDS_DO,
	ORCUS_COMPERR_IF_NEEDS_THEN,
	ORCUS_COMPERR_LOOPKEYWORD_WRONG_SPOT,
	ORCUS_COMPERR_ARRAY_NOT_CLOSED,
	ORCUS_COMPERR_TABLE_NOT_CLOSED,
	ORCUS_COMPERR_CALL_NOT_CLOSED,
	ORCUS_COMPERR_NESTEDSTATMENT_NOT_CLOSED, // ()
	ORCUS_COMPERR_NO_VARIABLE, //for any variable of any type with any syntax, including variable statments such as a>5
	ORCUS_COMPERR_NO_NAMED_VARIABLE, //this is for implicit variable names, under TOKEN_ENV_VAR.
	ORCUS_COMPERR_REQUIRES_EXPLICIT_ASSIGNMENT, // for multi-set statments, eg a,b=
};


enum token_type {
	TOKEN_TYPE_OPERATOR = 1,
	TOKEN_TYPE_KEYWORD = 2,
	TOKEN_TYPE_STRING = 3,
	TOKEN_TYPE_NUMBER = 4,
};


struct compiler_token {
	orcus_int line_num = 0;
	orcus_int line_col = 0;
	char* chars = nullptr;
	int num_chars = 0;
	int token_flags = 0;
	int token_type = 0;
};

struct compiler_function {
	orcus_opcode* instructions = nullptr;
	orcus_int* instruction_tokens = nullptr; //which token each instruction points to
	orcus_int number_instructions = 0;
	orcus_int tokens_consumed = 0;
	
	
	orcus_int token_error_num = 0;
	int errorcode = 0;

	compiler_function* master = nullptr; //stores the top-level CF, for constants and stuff.

	//info for loops
	bool in_loop = false;
	orcus_int* loop_jumptostarts = nullptr;
	orcus_int num_loop_jumptostart = 0;
	orcus_int* loop_jumptoends = nullptr;
	orcus_int num_loop_jumptoend = 0;

	//constant strings
	orcus_int num_string_constants = 0;
	compiler_token* string_constant_tokens = nullptr;

	//goto
	orcus_int num_gotos = 0; //how many goto labels exist
	orcus_int** goto_positions = nullptr; //where each goto is in bytecode, as absolute instruction number. list of lists. positions[label][goto_num]
	orcus_int* goto_pos_count = nullptr; // how many gotos are present in each label
	bool* goto_defined; // if the goto was defined with a label
	compiler_token* goto_alias = nullptr; //the identifier used for the goto's label
	orcus_int* goto_labelpos = nullptr; //where the label is defined in bytecode, as an absolute instruction number

	//multiarg output
	orcus_int num_args_out = 0;
};



inline bool char_is_character(unsigned char c){
	return (c>=0x41 && c<=0x5A) || (c>=0x61 && c<=0x7A) || c=='_';
}

inline bool char_is_number(unsigned char c) {
	return (c >= 0x30 && c <= 0x39);
}
//semicolon is considered whitespace, not a symbol. is is to appease c users who go;
inline bool char_is_symbol(unsigned char c) {
	return ((c>=0x21 && c<=0x2F) || (c>= 0x3C && c<= 0x40) || (c>= 0x5B && c<= 0x60) || (c>= 0x7B && c<= 0x7E) || c==0x3A);
}

inline bool char_is_whitespace(unsigned char c) {
	return (!(char_is_symbol(c) || char_is_number(c) || char_is_character(c)));
}


inline compiler_token* new_compiler_token(orcus_int col_num, orcus_int line_num){
	compiler_token* out=(compiler_token*)malloc(sizeof(compiler_token));
	if(out){
		out->line_num= line_num;
		out->line_col= col_num;
		out->chars = nullptr;
		out->num_chars = 0;
		out->token_flags = 0;
		out->token_type=0;
	}
	return out;
}
inline void init_compiler_token(compiler_token* t,orcus_int col_num, orcus_int line_num) {
	memset(t, 0, sizeof(compiler_token));
	t->line_col = col_num;
	t->line_num = line_num;
}

inline bool append_char_to_compiler_token(compiler_token* token, char c){
	if (token->num_chars % 0xf == 0) {
		char* naddr = (char*)realloc(token->chars, sizeof(char) * (0xf + token->num_chars));
		if (!naddr)return false;
		token->chars = naddr;
	}
	token->chars[token->num_chars]=c;
	token->num_chars++;
	return true;
}

inline void advance_position_from_char(char c,char c_prev,orcus_int* col,orcus_int* line){
	(*col)++;
	if( (c=='\n' || c=='\r') && !(c_prev=='\r' && c=='\n') ){
		*col=1;
		(*line)++;
	}
}

inline bool add_token_or_destroy_array(compiler_token** array,orcus_int* size,compiler_token* toadd){
	if (!toadd->num_chars && toadd->token_type!=TOKEN_TYPE_STRING) {
		free(toadd->chars);
		return false;
	}
	if (*size % 0x1f == 0) {
		compiler_token* nptr = (compiler_token*)realloc(*array, sizeof(compiler_token) * (*size + 0x1f));
		if (!nptr) {
			for (orcus_int i = 0; i < (const orcus_int)*size; i++) {
				free((*array)[i].chars);
			}
			free(array);
			*size = 0;
			return true;
		}
		*array = nptr;
	}
	(*array)[*size]=*toadd;
	(*size)++;
	return false;
	
}


inline bool token_matches_chars(compiler_token* t,const char* s){
	if(strlen(s)!=t->num_chars)return false;
	for (orcus_int c = 0; c < t->num_chars; c++) {
		if (t->chars[c] != s[c])return false;
	}
	return true;
}

bool symbol_can_merge_token(compiler_token* t,const char c){
	if(!t->num_chars)return true;
	const char last_char = t->chars[t->num_chars-1];
	switch(last_char){
		//symbols that can be repeated with themselves only. ==
		case '=': // ==
		// "where's (), {}, and []?" you may ask. well, it's more helpful for the compiler if we read each segment individually, so we don't merge those.
			return last_char == c;
		//symbols that can be repeated with themselves and equals. += ++ && &=
		case '!': // !! !=
		case '&': // && &=
		case '|': // || |=
		case '~': // ~~ ~=
		case '+': // ++ +=
		case '-': // -- -=
		case '<': // << <=
		case '>': // >> >=
		case '.': // .. ..=
			return last_char == c || c == '=';
		//symbols that can only be repeated with equals
		case '*': // *=
		case '/': // /=
		case '\\': // \=
		case '%': // %=
		case '^': // ^=
			return c == '=';
		default: //otherwise, the symbol is not repeatable
			return false;
	}
}

inline bool char_is_hex(char c){
	return (c>='0' && c<='9') || (c>='a' && c<='f') || (c>='A' && c<='F');
}

inline unsigned char read_hex_string_as_num(char c1,char c2){ //big endian
	unsigned char out=0;
	if(c1>='0' && c1<='9'){
		out=c1-'0';
	}else if(c1>='a' && c1<='f'){
		out=10+c1-'a';
	}else if(c1>='A' && c1<='F'){
		out=10+c1-'A';
	}
	
	out<<=4;
	
	if(c2>='0' && c2<='9'){
		out|=c2-'0';
	}else if(c2>='a' && c2<='f'){
		out|=10+c2-'a';
	}else if(c2>='A' && c2<='F'){
		out|=10+c2-'A';
	}
	
	return out;
}

inline bool char_is_dec(char c){
	return (c>='0' && c<='9');
}

inline unsigned char read_dec_string_as_num(char c1,char c2,char c3){
	unsigned char out=0;
	if(c1>='0' && c1<='9'){
		out=c1-'0';
	}
	
	out*=10;
	
	if(c2>='0' && c2<='9'){
		out+=c2-'0';
	}
	
	out*=10;
	
	if(c3>='0' && c3<='9'){
		out+=c3-'0';
	}
	
	return out;
}

inline bool char_is_bin(char c) {
	return c == '1' || c == '0';
}

compiler_token* orcus_compile_tokenize_mstring(orcus_string* str,orcus_int* num_out){
	
	*num_out = 0;

	compiler_token cur_tok;
	init_compiler_token(&cur_tok, 1, 1);
	compiler_token* out=nullptr;
	
	bool reading_string=false;
	int8_t comment_mode=COMMENT_NONE;
	
	orcus_int line_num=1;
	orcus_int col_num=1;
	
	orcus_int i=0;
	const char* s=str->ptr;
	const orcus_int size=str->size;
	while(i<size){
		char c=s[i];
		char c_prev=i ? s[i-1] : '\0';

		if(comment_mode){
			advance_position_from_char(c,c_prev,&col_num,&line_num);
			i++;
			if(comment_mode==COMMENT_LINE && (c=='\n' || c=='\r') ){
				comment_mode=COMMENT_NONE;
			}else if(comment_mode==COMMENT_MULTI && c=='/' && c_prev=='*'){
				comment_mode=COMMENT_NONE;
			}
		}
		else{
				
			switch(cur_tok.token_type){
				case TOKEN_TYPE_STRING:
					if(c=='\"'){ //end string reading.
						if(add_token_or_destroy_array(&out,num_out,&cur_tok))return nullptr;
						advance_position_from_char(c, c_prev, &col_num, &line_num);
						init_compiler_token(&cur_tok, col_num, line_num);
						i++;
					}else if(c=='\\'){ //escapes.
						advance_position_from_char(c, c_prev, &col_num, &line_num);
						i++;
						char nc=i<size?s[i]:'\0';
						advance_position_from_char(nc, c, &col_num, &line_num);
						i++; //chars used: 2
						switch(nc){
							case 'a':
								append_char_to_compiler_token(&cur_tok,'\a');
								break;
							case 'b':
								append_char_to_compiler_token(&cur_tok,'\b');
								break;
							case 'e':
								append_char_to_compiler_token(&cur_tok,'\x1B'); //ASCII escape character
								break;
							case 'f':
								append_char_to_compiler_token(&cur_tok,'\f');
								break;
							case 'n':
								append_char_to_compiler_token(&cur_tok,'\n');
								break;
							case 'r':
								append_char_to_compiler_token(&cur_tok,'\r');
								break;
							case 't':
								append_char_to_compiler_token(&cur_tok,'\t');
								break;	
							case 'v':
								append_char_to_compiler_token(&cur_tok,'\v');
								break;
							case '\\':
								append_char_to_compiler_token(&cur_tok,'\\');
								break;
							case '\"':
								append_char_to_compiler_token(&cur_tok,'\"');
								break;
							case 'x':
							case 'X': //raw hex char reading
								{
									char hc1=i<size?s[i]:'\0';
									i++;
									char hc2=i<size?s[i]:'\0';
									i++;
									
									if(char_is_hex(hc1) && char_is_hex(hc2)){
										advance_position_from_char(hc1, nc, &col_num, &line_num);
										advance_position_from_char(hc2, hc1, &col_num, &line_num);
										append_char_to_compiler_token(&cur_tok,read_hex_string_as_num(hc1,hc2));
									}else if(char_is_hex(hc1)){
										append_char_to_compiler_token(&cur_tok,read_hex_string_as_num('0',hc1));
										advance_position_from_char(hc1, nc, &col_num, &line_num);
										i--;
									}else{
										append_char_to_compiler_token(&cur_tok,'\0');
										i-=2;
									}
								}
								break;
							case '0': //decimal char reading
							case '1':
							case '2':
							case '3':
							case '4':
							case '5':
							case '6':
							case '7':
							case '8':
							case '9':
								{
									char dc2=i<size?s[i]:'\0';
									i++;
									char dc3=i<size?s[i]:'\0';
									i++;
									
									if(char_is_dec(nc) && char_is_dec(dc2) && char_is_dec(dc3)){
										advance_position_from_char(dc2, nc, &col_num, &line_num);
										advance_position_from_char(dc3, dc2, &col_num, &line_num);
										append_char_to_compiler_token(&cur_tok,read_dec_string_as_num(nc,dc2,dc3));
									}else if(char_is_hex(nc) && char_is_hex(dc2)){
										advance_position_from_char(dc2, nc, &col_num, &line_num);
										append_char_to_compiler_token(&cur_tok,read_dec_string_as_num('0',nc,dc2));
										i--;
									}else{
										append_char_to_compiler_token(&cur_tok, read_dec_string_as_num('0','0',nc));
										i-=2;
									}
									
								}
								break;
							default: //UB: if we can't recognize the escape, just give the two chars themselves.
								append_char_to_compiler_token(&cur_tok,'\\');
								append_char_to_compiler_token(&cur_tok,nc);
						}
						
					}else{ //add char to string.
						advance_position_from_char(c, c_prev, &col_num, &line_num);
						append_char_to_compiler_token(&cur_tok,c);
						i++;
					}
					break;
				case TOKEN_TYPE_NUMBER:
					if(!char_is_number(c) && c!='.' && c!='x' && !char_is_hex(c) && c!='b') { //for decimal and hex
						if(add_token_or_destroy_array(&out,num_out,&cur_tok))return nullptr;
						init_compiler_token(&cur_tok, col_num, line_num);
					}else{
						append_char_to_compiler_token(&cur_tok,c);
						advance_position_from_char(c,c_prev,&col_num,&line_num);
						i++;
					}
					break;
				case TOKEN_TYPE_OPERATOR:
					if(c=='/' && c_prev=='/'){
						cur_tok.num_chars--; //remove the previous /
						comment_mode=COMMENT_LINE;
						advance_position_from_char(c,c_prev,&col_num,&line_num);
						i++;
					}else if(c=='*' && c_prev=='/'){
						cur_tok.num_chars--; //remove the previous /
						comment_mode=COMMENT_MULTI;
						advance_position_from_char(c,c_prev,&col_num,&line_num);
						i++;
					}else if(char_is_number(c) && (c_prev=='.' || c_prev=='-') ) { //numbers like .1 and -4
						cur_tok.token_type = TOKEN_TYPE_NUMBER;
					}else if(char_is_symbol(c) && symbol_can_merge_token(&cur_tok,c) ){
						append_char_to_compiler_token(&cur_tok,c);
						advance_position_from_char(c,c_prev,&col_num,&line_num);
						i++;
					}else{
						if(add_token_or_destroy_array(&out,num_out,&cur_tok))return nullptr;
						init_compiler_token(&cur_tok, col_num, line_num);
					}
					break;
				case TOKEN_TYPE_KEYWORD:
					if(char_is_character(c) || char_is_number(c) ){
						append_char_to_compiler_token(&cur_tok,c);
						advance_position_from_char(c,c_prev,&col_num,&line_num);
						i++;
					}else{
						if(add_token_or_destroy_array(&out,num_out,&cur_tok))return nullptr;
						init_compiler_token(&cur_tok, col_num, line_num);
					}
					break;
				default:
					if(char_is_character(c)){
						cur_tok.token_type=TOKEN_TYPE_KEYWORD;
					}else if(char_is_number(c)){
						cur_tok.token_type=TOKEN_TYPE_NUMBER;
					}else if(char_is_symbol(c)){
						if(c=='\"'){
							cur_tok.token_type=TOKEN_TYPE_STRING;
							advance_position_from_char(c, c_prev, &col_num, &line_num);
							i++;
						}else{
							cur_tok.token_type=TOKEN_TYPE_OPERATOR;
						}
					}else{
						advance_position_from_char(c,c_prev,&col_num,&line_num);
						i++;
					}
			}
		}
		
	}
	if (cur_tok.num_chars) {
		if (add_token_or_destroy_array(&out, num_out, &cur_tok))return nullptr;
	}

	//tokens extracted from string. now we need to run a processing step to get more useful data out of it.
	for(orcus_int i=0;i<(const orcus_int)*num_out;i++){
		compiler_token t = out[i];
		switch(t.token_type){
			case TOKEN_TYPE_STRING:
				t.token_flags = TOKEN_VARIABLE | TOKEN_STATIC_STRING;
				break;
			case TOKEN_TYPE_NUMBER:
				t.token_flags = TOKEN_VARIABLE | TOKEN_STATIC_NUMBER;
				break;
			case TOKEN_TYPE_KEYWORD:
				if(t.num_chars==2){
					if(token_matches_chars(&t,"if")){
						t.token_flags = TOKEN_KEYWORD;
					}else if(token_matches_chars(&t,"do")){
						t.token_flags = TOKEN_KEYWORD;
					}
				}else if(t.num_chars==3){
					if(token_matches_chars(&t,"nil")){
						t.token_flags = TOKEN_VARIABLE | TOKEN_STATIC_NIL;
					}else if(token_matches_chars(&t,"end")){
						t.token_flags = TOKEN_KEYWORD;
					}else if(token_matches_chars(&t,"inf")){
						t.token_flags = TOKEN_VARIABLE | TOKEN_STATIC_NUMBER;
					}
				}else if(t.num_chars==4){
					if(token_matches_chars(&t,"then")){
						t.token_flags = TOKEN_KEYWORD;
					}else if(token_matches_chars(&t,"true")){
						t.token_flags = TOKEN_VARIABLE | TOKEN_STATIC_BOOLEAN;
					}else if(token_matches_chars(&t,"else")){
						t.token_flags = TOKEN_KEYWORD;
					}else if(token_matches_chars(&t,"goto")){
						t.token_flags = TOKEN_KEYWORD;
					}
				}else if(t.num_chars==5){
					if(token_matches_chars(&t,"false")){
						t.token_flags = TOKEN_VARIABLE | TOKEN_STATIC_BOOLEAN;
					}else if(token_matches_chars(&t,"while")){
						t.token_flags = TOKEN_KEYWORD;
					}else if(token_matches_chars(&t,"local")){
						t.token_flags = TOKEN_SCOPESPECIFIER;
					}else if (token_matches_chars(&t, "break")) {
						t.token_flags = TOKEN_LOOP_MODIFIER;
					}
				}else if(t.num_chars==6){
					if(token_matches_chars(&t,"global")){
						t.token_flags = TOKEN_SCOPESPECIFIER;
					}else if(token_matches_chars(&t,"elseif")){
						t.token_flags = TOKEN_KEYWORD;
					}else if(token_matches_chars(&t,"return")){
						t.token_flags = TOKEN_KEYWORD;
					}
				}else if(t.num_chars==8){
					if(token_matches_chars(&t,"function")){
						t.token_flags = TOKEN_KEYWORD | TOKEN_VARIABLE;
					}else if(token_matches_chars(&t,"continue")){
						t.token_flags = TOKEN_LOOP_MODIFIER;
					}
				}
				if(!t.token_flags)t.token_flags = TOKEN_VARIABLE | TOKEN_ENV_VAR;
				break;
			case TOKEN_TYPE_OPERATOR:
				if(t.num_chars==1){
					switch(t.chars[0]){
						case '=':
							t.token_flags = TOKEN_OPERATOR | TOKEN_MISC_OP;
							break;
						case '-':
							t.token_flags = TOKEN_OPERATOR | TOKEN_BINARY_OP | TOKEN_UNARY_OP; //unary minus, too
							break;
						case '+':
						case '*':
						case '/':
						case '^':
						case '%':
						case '\\':
						case '&':
						case '|':
						case '~':
						case '>':
						case '<':
							t.token_flags = TOKEN_OPERATOR | TOKEN_BINARY_OP | TOKEN_TAKES_STATIC_NUM;
							break;
						case '!':
							t.token_flags = TOKEN_OPERATOR | TOKEN_UNARY_OP | TOKEN_TAKES_STATIC_NUM;
							break;
						case '#':
							t.token_flags = TOKEN_OPERATOR | TOKEN_UNARY_OP;
							break;
						case '[':
						case '{':
							t.token_flags = TOKEN_OPERATOR | TOKEN_MISC_OP | TOKEN_VARIABLE;
							break;
						case '(':
						case ')':
						case '}':
						case ']':
						case ':':
						case '?':
						case '.':
						case ',':
							t.token_flags = TOKEN_OPERATOR | TOKEN_MISC_OP;
							break;
					}
				}else if(t.num_chars==2){
					switch(t.chars[0]){
						case '&':
						case '~':
						case '|':
							if(t.chars[0]==t.chars[1]){
								t.token_flags = TOKEN_OPERATOR | TOKEN_BINARY_OP;
								break;
							}
						case '+':
						case '-':
							if(t.chars[0]==t.chars[1]){
								t.token_flags = TOKEN_OPERATOR | TOKEN_TAKES_STATIC_NUM | TOKEN_UNARY_OP | TOKEN_SELFMODIFY_OP;
								break;
							}
						case '%':
						case '/':
						case '*':
						case '\\':
						case '^':
							if(t.chars[1]=='='){
								t.token_flags = TOKEN_OPERATOR | TOKEN_TAKES_STATIC_NUM | TOKEN_BINARY_OP | TOKEN_SELFMODIFY_OP;
							}
							break;
						case '!':
							if(t.chars[1]=='!'){
								t.token_flags = TOKEN_OPERATOR | TOKEN_UNARY_OP;
							}else if(t.chars[1]=='='){
								t.token_flags = TOKEN_OPERATOR | TOKEN_BINARY_OP;
							}
							break;
						case '>':
						case '<':
							if(t.chars[0]==t.chars[1] || t.chars[1]=='='){
								t.token_flags = TOKEN_OPERATOR | TOKEN_BINARY_OP | TOKEN_TAKES_STATIC_NUM;
							}
							break;
						case '.':
							if(t.chars[1]=='.'){
								t.token_flags = TOKEN_OPERATOR | TOKEN_BINARY_OP;
							}
							break;
						case '=':
							if(t.chars[1]=='='){
								t.token_flags = TOKEN_OPERATOR | TOKEN_BINARY_OP;
							}
							break;
					}
				}else if(t.num_chars==3){
					if(token_matches_chars(&t,"..=")){
						t.token_flags = TOKEN_OPERATOR | TOKEN_BINARY_OP | TOKEN_SELFMODIFY_OP;
					} else if(token_matches_chars(&t,"<<=")){
						t.token_flags = TOKEN_OPERATOR | TOKEN_BINARY_OP | TOKEN_SELFMODIFY_OP;
					}else if(token_matches_chars(&t,">>=")){
						t.token_flags = TOKEN_OPERATOR | TOKEN_BINARY_OP | TOKEN_SELFMODIFY_OP;
					}else if (token_matches_chars(&t, "&&=")) {
						t.token_flags = TOKEN_OPERATOR | TOKEN_BINARY_OP | TOKEN_SELFMODIFY_OP;
					}else if (token_matches_chars(&t, "||=")) {
						t.token_flags = TOKEN_OPERATOR | TOKEN_BINARY_OP | TOKEN_SELFMODIFY_OP;
					}else if (token_matches_chars(&t, "~~=")) {
						t.token_flags = TOKEN_OPERATOR | TOKEN_BINARY_OP | TOKEN_SELFMODIFY_OP;
					}
				}
				break;
		}
		out[i]=t;
	}
	return out;
}
//end of tokenization code, probably.

//adding break keyword
inline bool compiler_add_loop_endjump(compiler_function* f,orcus_int instruction_point) {
	orcus_int* n=(orcus_int*)realloc(f->loop_jumptoends, sizeof(orcus_int) * (f->num_loop_jumptoend + 1));
	if (!n)return false;
	n[f->num_loop_jumptoend] = instruction_point;
	f->loop_jumptoends = n;
	f->num_loop_jumptoend++;
	return true;
}
inline bool compiler_add_loop_startjump(compiler_function* f, orcus_int instruction_point) {
	orcus_int* n = (orcus_int*)realloc(f->loop_jumptostarts, sizeof(orcus_int) * (f->num_loop_jumptostart + 1));
	if (!n)return false;
	n[f->num_loop_jumptostart] = instruction_point;
	f->loop_jumptostarts = n;
	f->num_loop_jumptostart++;
	return true;
}

inline orcus_int compiler_add_constant_string_from_token(compiler_function* fb, compiler_token* t) {
	compiler_function* f = fb->master;
	for (orcus_int n = 0; n < f->num_string_constants; n++) {
		compiler_token s = f->string_constant_tokens[n];
		if (s.num_chars != t->num_chars)continue;
		for (orcus_int c = 0; c < s.num_chars; c++) {
			if (s.chars[c] != t->chars[c])goto exit;
		}
		return n;
	exit:;
	}

	compiler_token* nptr = (compiler_token*)realloc(f->string_constant_tokens, sizeof(compiler_token) * (f->num_string_constants + 1));
	if (!nptr)return -1; //surely nobody will use 4294967295 / 18446744073709551615 constants in their code, right?
	f->string_constant_tokens = nptr;
	nptr[f->num_string_constants] = *t;
	f->num_string_constants++;
	return f->num_string_constants - 1;
}

inline bool compiler_add_goto_jump(compiler_function* f, compiler_token* t, orcus_int instruction_point) {
	//compiler_function* f = fb->master;
	void* nptr;
	for (orcus_int n = 0; n < f->num_gotos; n++) {
		compiler_token s = f->goto_alias[n];
		if (s.num_chars != t->num_chars)continue;
		for (orcus_int c = 0; c < t->num_chars; c++) {
			if (t->chars[c] != s.chars[c])goto next;
		}

		nptr = realloc(f->goto_positions[n], sizeof(orcus_int) * (f->goto_pos_count[n] + 1));
		if (!nptr)return false;
		f->goto_positions[n] = (orcus_int*)nptr;
		f->goto_positions[n][f->goto_pos_count[n]] = instruction_point;
		f->goto_pos_count[n]++;
		return true;

	next:;
	}

	nptr = realloc(f->goto_positions, sizeof(orcus_int*) * (f->num_gotos + 1));
	if (!nptr)return false;
	f->goto_positions = (orcus_int**)nptr;

	nptr = realloc(f->goto_pos_count, sizeof(orcus_int) * (f->num_gotos + 1));
	if (!nptr)return false;
	f->goto_pos_count = (orcus_int*)nptr;

	nptr = realloc(f->goto_defined, sizeof(bool) * (f->num_gotos + 1));
	if (!nptr)return false;
	f->goto_defined = (bool*)nptr;

	nptr = realloc(f->goto_alias, sizeof(compiler_token) * (f->num_gotos + 1));
	if (!nptr)return false;
	f->goto_alias = (compiler_token*)nptr;

	nptr = realloc(f->goto_labelpos, sizeof(orcus_int) * (f->num_gotos + 1));
	if (!nptr)return false;
	f->goto_labelpos = (orcus_int*)nptr;

	nptr = malloc(sizeof(orcus_int));
	if (!nptr)return false;
	f->goto_positions[f->num_gotos] = (orcus_int*)nptr;
	f->goto_positions[f->num_gotos][0] = instruction_point;
	f->goto_pos_count[f->num_gotos] = 1;
	f->goto_defined[f->num_gotos] = false;
	f->goto_alias[f->num_gotos] = *t;
	f->goto_labelpos[f->num_gotos] = 0;

	f->num_gotos++;
	return true;
}

inline bool compiler_add_goto_label(compiler_function* f, compiler_token* t, orcus_int instruction_point) {
	//compiler_function* f = fb->master;
	void* nptr;

	for (orcus_int n = 0; n < f->num_gotos; n++) {
		compiler_token s = f->goto_alias[n];
		if (s.num_chars != t->num_chars)continue;
		for (orcus_int c = 0; c < t->num_chars; c++) {
			if (t->chars[c] != s.chars[c])goto next;
		}

		f->goto_defined[n] = true;
		f->goto_labelpos[n] = instruction_point;
		return true;

	next:;
	}

	nptr = realloc(f->goto_positions, sizeof(orcus_int*) * (f->num_gotos + 1));
	if (!nptr)return false;
	f->goto_positions = (orcus_int**)nptr;

	nptr = realloc(f->goto_pos_count, sizeof(orcus_int) * (f->num_gotos + 1));
	if (!nptr)return false;
	f->goto_pos_count = (orcus_int*)nptr;

	nptr = realloc(f->goto_defined, sizeof(bool) * (f->num_gotos + 1));
	if (!nptr)return false;
	f->goto_defined = (bool*)nptr;

	nptr = realloc(f->goto_alias, sizeof(compiler_token) * (f->num_gotos + 1));
	if (!nptr)return false;
	f->goto_alias = (compiler_token*)nptr;

	nptr = realloc(f->goto_labelpos, sizeof(orcus_int) * (f->num_gotos + 1));
	if (!nptr)return false;
	f->goto_labelpos = (orcus_int*)nptr;

	f->goto_positions[f->num_gotos] = nullptr;
	f->goto_pos_count[f->num_gotos] = 0;
	f->goto_defined[f->num_gotos] = true;
	f->goto_alias[f->num_gotos] = *t;
	f->goto_labelpos[f->num_gotos] = instruction_point;

	f->num_gotos++;
	return true;
}

inline void propigate_compiler_info_down(compiler_function* parent, compiler_function* child) {
	child->master = parent->master;
	child->in_loop = parent->in_loop;
}

inline compiler_function* new_compiler_function(){
	compiler_function* out = (compiler_function*)malloc(sizeof(compiler_function));
	if(out){
		out->instructions=nullptr;
		out->number_instructions=0;
		out->instruction_tokens=nullptr;
		out->tokens_consumed = 0;
		out->token_error_num = 0;
		out->errorcode = 0;
	}
	return out;
}

inline void clear_compiler_function(compiler_function* f) {
	free(f->instructions);
	free(f->instruction_tokens);
	free(f->string_constant_tokens);
	free(f->loop_jumptoends);
	free(f->loop_jumptostarts);
	for (orcus_int i = 0; i < f->num_gotos;i++) {
		free(f->goto_positions[i]);
	}
	free(f->goto_alias);
	free(f->goto_defined);
	free(f->goto_labelpos);
	free(f->goto_pos_count);
	free(f->goto_positions);
	memset(f, 0, sizeof(compiler_function));
}

inline void delete_compiler_function(compiler_function* f){
	clear_compiler_function(f);
	free(f);
}




//appends the bytecode of fadd to fbase, and frees fadd.
inline bool merge_compiler_functions(compiler_function* fbase, compiler_function* fadd){
	if (fadd->num_string_constants) {
		compiler_token* tks=(compiler_token*)realloc(fbase->string_constant_tokens, sizeof(compiler_token)*(fadd->num_string_constants+fbase->num_string_constants) );
		if (!tks)return false;
		fbase->string_constant_tokens = tks;
		memcpy(fbase->string_constant_tokens + fbase->num_string_constants, fadd->string_constant_tokens, fadd->num_string_constants * sizeof(compiler_token));
		fbase->num_string_constants+=fadd->num_string_constants;
	}
	if (fadd->num_loop_jumptoend) {
		orcus_int* points = (orcus_int*)realloc(fbase->loop_jumptoends, sizeof(orcus_int)*(fbase->num_loop_jumptoend+ fadd->num_loop_jumptoend));
		if (!points)return false;
		fbase->loop_jumptoends = points;
		for (orcus_int i = 0; i < fadd->num_loop_jumptoend; i++) {
			fbase->loop_jumptoends[fbase->num_loop_jumptoend + i] = fadd->loop_jumptoends[i] + fbase->number_instructions; //can't memcpy, we need to adjust the break keyword points by the offset introduced by appending to the end of the instructions
		}
		fbase->num_loop_jumptoend += fadd->num_loop_jumptoend;
	}
	if (fadd->num_loop_jumptostart) {
		orcus_int* points = (orcus_int*)realloc(fbase->loop_jumptostarts, sizeof(orcus_int) * (fbase->num_loop_jumptostart + fadd->num_loop_jumptostart));
		if (!points)return false;
		fbase->loop_jumptostarts = points;
		for (orcus_int i = 0; i < fadd->num_loop_jumptostart; i++) {
			fbase->loop_jumptostarts[fbase->num_loop_jumptostart + i] = fadd->loop_jumptostarts[i] + fbase->number_instructions; //can't memcpy, we need to adjust the break keyword points by the offset introduced by appending to the end of the instructions
		}
		fbase->num_loop_jumptostart += fadd->num_loop_jumptostart;
	}
	if (fadd->num_gotos) {
		for (orcus_int i = 0; i < fadd->num_gotos; i++) {
			for (orcus_int n = 0; n < fadd->goto_pos_count[i]; n++) {
				compiler_add_goto_jump(fbase, fadd->goto_alias+i, fadd->goto_positions[i][n] + fbase->number_instructions); //i can't be fucked to write optimized code to do this
			}
			if (fadd->goto_defined[i])
				compiler_add_goto_label(fbase, fadd->goto_alias + i, fadd->goto_labelpos[i] + fbase->number_instructions); //ditto.
		}
	}
	if (fadd->number_instructions) {
		orcus_int nsiz = fbase->number_instructions + fadd->number_instructions;

		void* p = realloc(fbase->instructions, sizeof(orcus_opcode) * (nsiz));
		if (!p)return false;
		fbase->instructions = (orcus_opcode*)p;
		p = realloc(fbase->instruction_tokens, sizeof(orcus_int) * (nsiz));
		if (!p)return false;
		fbase->instruction_tokens = (orcus_int*)p;

		memcpy(fbase->instructions + fbase->number_instructions, fadd->instructions, sizeof(orcus_opcode) * fadd->number_instructions);
		memcpy(fbase->instruction_tokens + fbase->number_instructions, fadd->instruction_tokens, sizeof(orcus_int) * fadd->number_instructions);

		fbase->number_instructions = nsiz;
	}
	if (fadd->errorcode && !fbase->errorcode) {
		fbase->errorcode = fadd->errorcode;
		fbase->token_error_num = fadd->token_error_num;
	}
	delete_compiler_function(fadd);
	return true;
}

//like above, but does not free fadd
inline bool merge_compiler_functions_nofree(compiler_function* fbase, const compiler_function* fadd) {
	if (fadd->num_string_constants) {
		compiler_token* tks = (compiler_token*)realloc(fbase->string_constant_tokens, sizeof(compiler_token) * (fadd->num_string_constants + fbase->num_string_constants));
		if (!tks)return false;
		fbase->string_constant_tokens = tks;
		memcpy(fbase->string_constant_tokens + fbase->num_string_constants, fadd->string_constant_tokens, fadd->num_string_constants * sizeof(compiler_token));
		fbase->num_string_constants += fadd->num_string_constants;
	}
	if (fadd->num_loop_jumptoend) {
		orcus_int* points = (orcus_int*)realloc(fbase->loop_jumptoends, sizeof(orcus_int) * (fbase->num_loop_jumptoend + fadd->num_loop_jumptoend));
		if (!points)return false;
		fbase->loop_jumptoends = points;
		for (orcus_int i = 0; i < fadd->num_loop_jumptoend; i++) {
			fbase->loop_jumptoends[fbase->num_loop_jumptoend + i] = fadd->loop_jumptoends[i] + fbase->number_instructions; //can't memcpy, we need to adjust the break keyword points by the offset introduced by appending to the end of the instructions
		}
		fbase->num_loop_jumptoend += fadd->num_loop_jumptoend;
	}
	if (fadd->num_loop_jumptostart) {
		orcus_int* points = (orcus_int*)realloc(fbase->loop_jumptostarts, sizeof(orcus_int) * (fbase->num_loop_jumptostart + fadd->num_loop_jumptostart));
		if (!points)return false;
		fbase->loop_jumptostarts = points;
		for (orcus_int i = 0; i < fadd->num_loop_jumptostart; i++) {
			fbase->loop_jumptostarts[fbase->num_loop_jumptostart + i] = fadd->loop_jumptostarts[i] + fbase->number_instructions; //can't memcpy, we need to adjust the break keyword points by the offset introduced by appending to the end of the instructions
		}
		fbase->num_loop_jumptostart += fadd->num_loop_jumptostart;
	}
	if (fadd->num_gotos) {
		for (orcus_int i = 0; i < fadd->num_gotos; i++) {
			for (orcus_int n = 0; n < fadd->goto_pos_count[i]; n++) {
				compiler_add_goto_jump(fbase, fadd->goto_alias + i, fadd->goto_positions[i][n] + fbase->number_instructions); //i can't be fucked to write optimized code to do this
			}
			if (fadd->goto_defined[i])
				compiler_add_goto_label(fbase, fadd->goto_alias + i, fadd->goto_labelpos[i] + fbase->number_instructions); //ditto.
		}
	}
	if (fadd->number_instructions) {
		orcus_int nsiz = fbase->number_instructions + fadd->number_instructions;

		void* p = realloc(fbase->instructions, sizeof(orcus_opcode) * (nsiz));
		if (!p)return false;
		fbase->instructions = (orcus_opcode*)p;
		p = realloc(fbase->instruction_tokens, sizeof(orcus_int) * (nsiz));
		if (!p)return false;
		fbase->instruction_tokens = (orcus_int*)p;

		memcpy(fbase->instructions + fbase->number_instructions, fadd->instructions, sizeof(orcus_opcode) * fadd->number_instructions);
		memcpy(fbase->instruction_tokens + fbase->number_instructions, fadd->instruction_tokens, sizeof(orcus_int) * fadd->number_instructions);
	fbase->number_instructions = nsiz;
	}

	if (fadd->errorcode && !fbase->errorcode) {
		fbase->errorcode = fadd->errorcode;
		fbase->token_error_num = fadd->token_error_num;
	}
	return true;
}

//finishes up stuff like gotos and constants
inline bool compiler_function_finalize_addendums(compiler_function* f) {
	for (orcus_int i = 0; i < f->num_gotos; i++) {
		if (f->goto_defined[i]) {
			for (orcus_int n = 0; n < f->goto_pos_count[i]; n++) {
				orcus_int p = f->goto_positions[i][n];
				orcus_int* j = (orcus_int*)(f->instructions + p);
				*j = f->goto_labelpos[i] - p;
			}
		}
		free(f->goto_positions[i]);
	}
	free(f->goto_positions);
	free(f->goto_alias);
	free(f->goto_defined);
	free(f->goto_labelpos);
	free(f->goto_pos_count);
	f->goto_positions = nullptr;
	f->goto_alias = nullptr;
	f->goto_defined = nullptr;
	f->goto_labelpos = nullptr;
	f->goto_pos_count = nullptr;
	f->num_gotos = 0;

	orcus_int spaceneeded = 0; //track space needed so we can preallocate it in a big block
	for (orcus_int i = 0; i < f->num_string_constants; i++) {
		compiler_token s = f->string_constant_tokens[i];
		spaceneeded += (s.num_chars + sizeof(orcus_opcode) - 1) / sizeof(orcus_opcode);
		spaceneeded += 2;
		spaceneeded += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE * 2;
	}
	void* nptr = realloc(f->instructions, sizeof(orcus_opcode) * (spaceneeded + f->number_instructions));
	if (!nptr) {
		f->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
		return false;
	}
	f->instructions = (orcus_opcode*)nptr;
	memmove(f->instructions + spaceneeded, f->instructions, f->number_instructions * sizeof(orcus_opcode));

	nptr = realloc(f->instruction_tokens, sizeof(orcus_int) * (spaceneeded + f->number_instructions));
	if (!nptr) {
		f->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
		return false;
	}
	f->instruction_tokens = (orcus_int*)nptr;
	memmove(f->instruction_tokens + spaceneeded, f->instruction_tokens, f->number_instructions * sizeof(orcus_int));
	memset(f->instruction_tokens, 0, sizeof(orcus_int) * spaceneeded); //zero the tokens at the constant string assignment
	orcus_int offset = 0;
	for (orcus_int i = 0; i < f->num_string_constants; i++) {
		compiler_token s = f->string_constant_tokens[i];
		f->instructions[offset] = ORCUS_OPCODE_NSTR;
		offset++;
		*((orcus_int*)(f->instructions + offset)) = s.num_chars;
		offset += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
		memcpy(f->instructions + offset, s.chars, s.num_chars);
		offset += (s.num_chars + sizeof(orcus_opcode) - 1) / sizeof(orcus_opcode);
		f->instructions[offset] = ORCUS_OPCODE_SCON;
		offset++;
		*((orcus_int*)(f->instructions + offset)) = i;
		offset += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
	}
	f->number_instructions += spaceneeded;

	free(f->string_constant_tokens);
	f->string_constant_tokens = nullptr;
	f->num_string_constants = 0;
	return true;
}

inline bool add_instruction(compiler_function* f, orcus_opcode opcode, orcus_int token_num){
	void* p=realloc(f->instructions,sizeof(orcus_opcode)*(f->number_instructions+1) );
	if(!p)return false;
	f->instructions = (orcus_opcode*)p;
	p=realloc(f->instruction_tokens,sizeof(orcus_int)*(f->number_instructions+1) );
	if(!p)return false;
	f->instruction_tokens = (orcus_int*)p;
	
	f->instructions[f->number_instructions] = opcode;
	f->instruction_tokens[f->number_instructions] = token_num;
	
	f->number_instructions++;
	return true;
}

inline bool add_instruction_at_begining(compiler_function* f, orcus_opcode opcode, orcus_int token_num){
	void* p=realloc(f->instructions,sizeof(orcus_opcode)*(f->number_instructions+1) );
	if(!p)return false;
	f->instructions = (orcus_opcode*)p;
	p=realloc(f->instruction_tokens,sizeof(orcus_int)*(f->number_instructions+1) );
	if(!p)return false;
	f->instruction_tokens = (orcus_int*)p;
	
	memmove(f->instructions+1,f->instructions, sizeof(orcus_opcode)*f->number_instructions );
	memmove(f->instruction_tokens+1,f->instruction_tokens, sizeof(orcus_int)*f->number_instructions );
	
	f->instructions[0] = opcode;
	f->instruction_tokens[0] = token_num;
	
	f->number_instructions++;
	return true;
}

inline bool add_rawdata(compiler_function* f, uint32_t data,orcus_int token_num){
	const int sizereq = sizeof(uint32_t) / sizeof(orcus_opcode);
	void* p=realloc(f->instructions,sizeof(orcus_opcode)*(f->number_instructions+ sizereq) );
	if(!p)return false;
	f->instructions = (orcus_opcode*)p;
	p=realloc(f->instruction_tokens,sizeof(orcus_int)*(f->number_instructions+ sizereq) );
	if(!p)return false;
	f->instruction_tokens = (orcus_int*)p;
	
	*(uint32_t*)(f->instructions+f->number_instructions) = data;
	for (int n = 0; n < sizereq; n++) {
		f->instruction_tokens[f->number_instructions+n] = token_num;
	}
	
	f->number_instructions+= sizereq;
	return true;
}

#ifdef ORCUS_64BIT
inline bool add_rawdata_double(compiler_function* f, uint64_t data,orcus_int token_num){
	const int sizereq = sizeof(uint64_t) / sizeof(orcus_opcode);
	void* p=realloc(f->instructions,sizeof(orcus_opcode)*(f->number_instructions+ sizereq) );
	if(!p)return false;
	f->instructions = (orcus_opcode*)p;
	p=realloc(f->instruction_tokens,sizeof(orcus_int)*(f->number_instructions+ sizereq) );
	if(!p)return false;
	f->instruction_tokens = (orcus_int*)p;
	
	*(uint64_t*)(f->instructions+f->number_instructions) = data;
	for (int n = 0; n < sizereq; n++) {
		f->instruction_tokens[f->number_instructions + n] = token_num;
	}
	
	f->number_instructions+= sizereq;
	return true;
}
#endif


#ifdef ORCUS_64BIT
typedef uint64_t binarydata;
inline bool add_rawdata_bitwidth_size(compiler_function* f, uint64_t data,orcus_int token_num){
	return add_rawdata_double(f, data, token_num);
}
#else
typedef uint32_t binarydata;
inline bool add_rawdata_bitwidth_size(compiler_function* f, uint32_t data,orcus_int token_num){
	return add_rawdata(f, data, token_num);
}
#endif

inline compiler_token* m_compile_get_next_token(compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset){
	if( token_offset < num_tokens && token_offset>=0 ){ //OOB
		return tokens+token_offset;
	}
	return nullptr;
}

orcus_opcode m_compile_get_operator_opcode_from_token(compiler_token* t,int ttype=0){
	orcus_int nc = t->num_chars;
	if(nc==1){
		char c=t->chars[0];
		switch (c){
			case '+':
				return ORCUS_OPCODE_ADD;
			case '-':
				return ttype==1 ? ORCUS_OPCODE_UNM : ORCUS_OPCODE_SUB;
			case '*':
				return ORCUS_OPCODE_MUL;
			case '^':
				return ORCUS_OPCODE_POW;
			case '/':
				return ORCUS_OPCODE_DIV;
			case '\\':
				return ORCUS_OPCODE_IDIV;
			case '%':
				return ORCUS_OPCODE_MOD;
			case '!':
				return ORCUS_OPCODE_BNOT;
			case '&':
				return ORCUS_OPCODE_BAND;
			case '|':
				return ORCUS_OPCODE_BOR;
			case '~':
				return ORCUS_OPCODE_BXOR;
			case '#':
				return ORCUS_OPCODE_LEN;
			case '<':
				return ORCUS_OPCODE_LET;
			case '>':
				return ORCUS_OPCODE_GRT;
		}
	}else if(nc==2){
		if(t->chars[0]==t->chars[1]){
			switch(t->chars[0]){
				case '=':
					return ORCUS_OPCODE_EQL;
				case '.':
					return ORCUS_OPCODE_CNCT;	
				case '<':
					return ORCUS_OPCODE_BSHL;
				case '>':
					return ORCUS_OPCODE_BSHR;
				case '!':
					return ORCUS_OPCODE_LNOT;
				case '&':
					return ORCUS_OPCODE_LAND;
				case '|':
					return ORCUS_OPCODE_LOR;
				case '~':
					return ORCUS_OPCODE_LXOR;
				case '+':
					return ORCUS_OPCODE_INC;
				case '-':
					return ORCUS_OPCODE_DEC;
			}
		}else if(t->chars[1]=='='){
			switch(t->chars[0]){
				case '!':
					return ORCUS_OPCODE_NEQ;
				case '<':
					return ORCUS_OPCODE_LTE;
				case '>':
					return ORCUS_OPCODE_GTE;
				case '&':
					return ORCUS_OPCODE_BAND;
				case '|':
					return ORCUS_OPCODE_BOR;
				case '~':
					return ORCUS_OPCODE_BXOR;
				case '+':
					return ORCUS_OPCODE_ADD;
				case '-':
					return ORCUS_OPCODE_SUB;
				case '*':
					return ORCUS_OPCODE_MUL;
				case '^':
					return ORCUS_OPCODE_POW;
				case '\\':
					return ORCUS_OPCODE_IDIV;
				case '/':
					return ORCUS_OPCODE_DIV;
				case '%':
					return ORCUS_OPCODE_MOD;
			}
		}
	}else if(nc==3){
		if(token_matches_chars(t,"..=")){
			return ORCUS_OPCODE_CNCT;
		}else if(token_matches_chars(t,"<<=")){
			return ORCUS_OPCODE_BSHL;
		}else if(token_matches_chars(t,">>=")){
			return ORCUS_OPCODE_BSHR;
		}else if (token_matches_chars(t, "&&=")) {
			return ORCUS_OPCODE_LAND;
		}else if (token_matches_chars(t, "||=")) {
			return ORCUS_OPCODE_LOR;
		}else if (token_matches_chars(t, "~~=")) {
			return ORCUS_OPCODE_LXOR;
		}
	}
	return ORCUS_OPCODE_NOP;
}

inline bool add_raw_string_onto_stack(compiler_function* f, char* s, orcus_int len, orcus_int dbg_toknum) { //no additional opcode, just raw length and chars.
	orcus_int required_space = (len + sizeof(orcus_opcode) - 1) / sizeof(orcus_opcode);
	add_rawdata_bitwidth_size(f, len, dbg_toknum);
	void* nptr = realloc(f->instructions, sizeof(orcus_opcode) * (f->number_instructions + required_space));
	if (!nptr) {
		f->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
		f->token_error_num = dbg_toknum;
		return false;
	}
	f->instructions = (orcus_opcode*)nptr;
	nptr = realloc(f->instruction_tokens, sizeof(orcus_int) * (f->number_instructions + required_space));
	if (!nptr) {
		f->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
		f->token_error_num = dbg_toknum;
		return false;
	}
	f->instruction_tokens = (orcus_int*)nptr;

	memcpy(f->instructions + f->number_instructions, s, sizeof(char) * len);
	for (orcus_int i = 0; i < required_space; i++) {
		f->instruction_tokens[f->number_instructions + i] = dbg_toknum;
	}
	f->number_instructions += required_space;
	return true;
}


inline void add_string_onto_stack(compiler_function* f,char* s,orcus_int len,orcus_int dbg_toknum){
	orcus_int required_space = (len + sizeof(orcus_opcode)-1) / sizeof(orcus_opcode);
	add_instruction(f, ORCUS_OPCODE_NSTR,dbg_toknum);
	add_rawdata_bitwidth_size(f, len,dbg_toknum);
	void* nptr=realloc(f->instructions, sizeof(orcus_opcode)*(f->number_instructions+required_space) );
	if(!nptr){
		f->errorcode=ORCUS_COMPERR_MEMORY_ALLOCATION;
		f->token_error_num=dbg_toknum;
		return;
	}
	f->instructions=(orcus_opcode*)nptr;
	nptr=realloc(f->instruction_tokens, sizeof(orcus_int)*(f->number_instructions+required_space) );
	if(!nptr){
		f->errorcode=ORCUS_COMPERR_MEMORY_ALLOCATION;
		f->token_error_num=dbg_toknum;
		return;
	}
	f->instruction_tokens=(orcus_int*)nptr;
	
	memcpy( f->instructions+f->number_instructions, s ,sizeof(char)*len );
	for(orcus_int i=0;i<required_space;i++){
		f->instruction_tokens[f->number_instructions+i]=dbg_toknum;
	}
	f->number_instructions+=required_space;
}




//forward references for other complex functions
orcus_int m_compile_read_var_statment(compiler_function* f, compiler_token* tokens, orcus_int num_tokens, orcus_int token_offset, compiler_function* postfix_function = nullptr);
orcus_int orcus_compile_tokens_to_bytecode(compiler_token* tokens, orcus_int num_tokens, orcus_int token_offset, compiler_function* out);


orcus_int m_compile_read_variable(compiler_function* f, compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset){
	orcus_int initial_offset=token_offset;
	compiler_token* cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
	//if(!(cur_tok &&(cur_tok->token_flags & (TOKEN_VARIABLE | TOKEN_SCOPESPECIFIER) ))) {
	if (!cur_tok){
		return 0;
	}
	if(cur_tok->token_flags & TOKEN_STATIC_NIL){
		add_instruction(f, ORCUS_OPCODE_NNIL,token_offset);
		token_offset++;
	}else if(cur_tok->token_flags & TOKEN_STATIC_BOOLEAN){
		if(cur_tok->chars[0]=='t'){
			add_instruction(f, ORCUS_OPCODE_NTRU,token_offset);
		}else{
			add_instruction(f, ORCUS_OPCODE_NFAL,token_offset);
		}
		token_offset++;
	}else if(cur_tok->token_flags & TOKEN_STATIC_STRING){
		orcus_int ci=compiler_add_constant_string_from_token(f,cur_tok);
		if (ci==-1){
			f->errorcode=ORCUS_COMPERR_MEMORY_ALLOCATION;
			f->token_error_num=token_offset;
			return 0;
		}
		add_instruction(f,ORCUS_OPCODE_GCON, token_offset);
		add_rawdata_bitwidth_size(f,ci, token_offset);
		//add_string_onto_stack(f,cur_tok->chars,cur_tok->num_chars,token_offset);
		token_offset++;
	}else if(cur_tok->token_flags & TOKEN_STATIC_NUMBER){
		char* cstr=(char*)malloc(cur_tok->num_chars+1);
		if (!cstr) {
			f->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
			f->token_error_num = token_offset;
			return 0;
		}
		memcpy(cstr,cur_tok->chars,cur_tok->num_chars);
		cstr[cur_tok->num_chars]='\0';
		char* end;
		
		orcus_int i;
		if (cstr[0] == '-') {
			if (cur_tok->num_chars > 3 && cstr[2] == 'x') {
				i = strtoull(cstr + 3, &end, 16);
				i *= -1;
			}
			else if (cur_tok->num_chars > 3 && cstr[2] == 'b') {
				i = strtoull(cstr + 3, &end, 2);
				i *= -1;
			}
			else {
				i = strtoll(cstr, &end, 10);
			}
		}
		else {
			if (cur_tok->num_chars > 2 && cstr[1] == 'x') {
				i = strtoull(cstr + 2, &end, 16);
			}
			else if (cur_tok->num_chars > 2 && cstr[1] == 'b') {
				i = strtoull(cstr + 2, &end, 2);
			}
			else {
				i = strtoll(cstr, &end, 10);
			}
		}
		
		if (*end=='\0') {
			add_instruction(f, ORCUS_OPCODE_NINT,token_offset);
			add_rawdata_bitwidth_size(f, *((binarydata*)&i),token_offset);
		}else{
			orcus_float fa = strtod(cstr, &end);
			if (*end=='\0') {
				add_instruction(f, ORCUS_OPCODE_NFLO,token_offset);
				add_rawdata_bitwidth_size(f, *((binarydata*)&fa),token_offset);
			}else{
				f->errorcode=ORCUS_COMPERR_MALFORMED_NUMBER;
				f->token_error_num=token_offset;
				return 0;
			}
		}
		free(cstr);
		token_offset++;
	}else if( (cur_tok->token_flags & (TOKEN_OPERATOR | TOKEN_MISC_OP)) && cur_tok->num_chars==1 && (cur_tok->chars[0] == '[' || cur_tok->chars[0] == '{') ){
		
		bool is_array = cur_tok->chars[0]=='[';
		orcus_int implicit_position=0;
		orcus_int elements_set = 0;
			
		
		token_offset++;

		while(true){ //the ticky thing about tables and arrays is how many syntax options we have. [1,2,3], [1=1,2=2,3=3]
			cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
				
				
			if(!cur_tok){
				break;
			}
				
			//exit at closing symbol.
			if( (cur_tok->token_flags & TOKEN_OPERATOR) && cur_tok->num_chars==1 && cur_tok->chars[0]== (is_array ? ']' : '}') ){
				add_instruction(f, is_array ? ORCUS_OPCODE_NARR : ORCUS_OPCODE_NTAB, token_offset);
				if (elements_set) {
					add_instruction(f,ORCUS_OPCODE_MSET,token_offset);
					add_rawdata_bitwidth_size(f, elements_set, token_offset);
				}
				token_offset++;
				break;
			}
			elements_set++;
				
			//read a variable. this could be implicit index value or an index.
			compiler_function idxfunc;
			memset(&idxfunc, 0, sizeof(compiler_function));
			propigate_compiler_info_down(f, &idxfunc);
			
			idxfunc.num_args_out = 1;
			token_offset+= m_compile_read_var_statment(&idxfunc,tokens,num_tokens,token_offset);
			
			if(idxfunc.errorcode){
				f->errorcode = idxfunc.errorcode;
				f->token_error_num = idxfunc.token_error_num;
				return 0;
			}
				
			//read the next token to find out.
			cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
			if(!cur_tok){
				f->errorcode=ORCUS_COMPERR_NO_MORE_TOKENS;
				f->token_error_num=token_offset;
				return 0;
			}
				
			if(cur_tok->token_flags & TOKEN_OPERATOR && cur_tok->num_chars==1){
				if(cur_tok->chars[0]=='='){ //explicit index
					//add_instruction(f,ORCUS_OPCODE_CPYT,token_offset);
					
					

					
					token_offset++;
					orcus_int oao = f->num_args_out;
					f->num_args_out = 1;
					orcus_int o= m_compile_read_var_statment(f,tokens,num_tokens,token_offset);
					f->num_args_out = oao;
					
					//add_instruction(f, ORCUS_OPCODE_SWPT, token_offset);

					merge_compiler_functions_nofree(f, &idxfunc);
					clear_compiler_function(&idxfunc);

					if(f->errorcode){
						return 0;
					}else if(!o){
						f->errorcode=ORCUS_COMPERR_NO_VARIABLE;
						f->token_error_num=token_offset;
						return 0;
					}
						
					//add_instruction(f,ORCUS_OPCODE_SET,token_offset);
					token_offset += o;
					cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
						
					if(!cur_tok){
						f->errorcode=ORCUS_COMPERR_NO_MORE_TOKENS;
						f->token_error_num=token_offset;
						return 0;
					}
						
					if(!(cur_tok->token_flags & TOKEN_OPERATOR) && cur_tok->num_chars!=1 && !(cur_tok->chars[0]==',' || is_array ? ']' : '}')){
						f->errorcode=ORCUS_COMPERR_WRONG_SYMBOL;
						f->token_error_num=token_offset;
						return 0;
					}
					if(cur_tok->chars[0] == ',')token_offset++;
				}else if(cur_tok->chars[0]==',' || is_array ? ']' : '}'){ //implicit index
					//add_instruction(f,ORCUS_OPCODE_CPYT,token_offset);
					
					merge_compiler_functions_nofree(f, &idxfunc);
					clear_compiler_function(&idxfunc);

					//add_instruction(f, ORCUS_OPCODE_SWPT, token_offset);

					add_instruction(f,ORCUS_OPCODE_NINT,token_offset);
					add_rawdata_bitwidth_size(f,*(binarydata*)&implicit_position,token_offset);
					implicit_position++;

					//add_instruction(f,ORCUS_OPCODE_SET,token_offset);
					if(cur_tok->chars[0] == ',')token_offset++;
				}else{
					f->errorcode=ORCUS_COMPERR_WRONG_SYMBOL;
					f->token_error_num=token_offset;
					return 0;
				}
			}else{
				f->errorcode=ORCUS_COMPERR_WRONG_SYMBOL;
				f->token_error_num=token_offset;
				return 0;
			}
				
		};
		
	}else if(cur_tok->token_flags & (TOKEN_ENV_VAR | TOKEN_SCOPESPECIFIER) ){
		orcus_opcode g_op= ORCUS_OPCODE_GENV_KCS;
		if(cur_tok->token_flags & TOKEN_SCOPESPECIFIER){
			if(token_matches_chars(cur_tok,"local")){
				g_op = ORCUS_OPCODE_GETL_KCS;
			}else if(token_matches_chars(cur_tok,"global")){
				g_op = ORCUS_OPCODE_GETG_KCS;
			}
			token_offset++;
			cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
			
			if(!cur_tok){
				f->errorcode=ORCUS_COMPERR_NO_MORE_TOKENS;
				f->token_error_num=token_offset;
				return 0;
			}else if(!(cur_tok->token_flags & TOKEN_ENV_VAR)){
				f->errorcode = ORCUS_COMPERR_NO_NAMED_VARIABLE;
				f->token_error_num=token_offset;
				return 0;
			}
			
			
		}
		
		add_instruction(f, g_op, token_offset);
		if (!add_raw_string_onto_stack(f, cur_tok->chars, cur_tok->num_chars, token_offset)) {
			f->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
			f->token_error_num = token_offset;
			return 0;
		}
		
		token_offset++;
	}else if(cur_tok->token_flags & TOKEN_MISC_OP && cur_tok->num_chars==1 && cur_tok->chars[0]=='('){ //parenthesis statement reading. we read (b+c) as a variable because it makes things easier on the compiler, and helps further segment the code to be more clean. after all, a+(b+c) is functionally equivalent to computing (b+c) then doing a+.
		token_offset++;

		orcus_int oao = f->num_args_out;
		f->num_args_out = 1;
		token_offset+=m_compile_read_var_statment(f,tokens,num_tokens,token_offset);
		f->num_args_out = oao;
		if(f->errorcode){
			return 0;
		}
		
		cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
		if(!cur_tok){
			f->errorcode = ORCUS_COMPERR_NO_MORE_TOKENS;
			f->token_error_num=token_offset;
			return 0;
		}else if(!(cur_tok->token_flags & TOKEN_MISC_OP && cur_tok->num_chars==1 && cur_tok->chars[0]==')') ){
			f->errorcode = ORCUS_COMPERR_NESTEDSTATMENT_NOT_CLOSED;
			f->token_error_num=token_offset;
			return 0;
		}
		token_offset++;
		
	}else if(cur_tok->token_flags & TOKEN_KEYWORD && token_matches_chars(cur_tok,"function")){ //likewise, functions are a first class variable so there's no real reason to not include parsing it in the variable reading block.
		token_offset++;
		cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
		if(!cur_tok){
			f->errorcode=ORCUS_COMPERR_NO_MORE_TOKENS;
			f->token_error_num=token_offset;
			return 0;
		}else if( !(cur_tok->token_flags & TOKEN_MISC_OP && cur_tok->num_chars==1 && cur_tok->chars[0]=='(') ){
			f->errorcode=ORCUS_COMPERR_WRONG_SYMBOL;
			f->token_error_num=token_offset;
			return 0;
		}
		
		add_instruction(f,ORCUS_OPCODE_NFUN,token_offset);
		
		//binarydata* sizeoffunction=(binarydata*)(f->instructions+f->number_instructions);
		
		add_rawdata_bitwidth_size(f,0,token_offset);

		orcus_int initial_bytecode_size=f->number_instructions;
		
		token_offset++;
		
		compiler_function funcargreadf;
		memset(&funcargreadf, 0, sizeof(compiler_function));
		propigate_compiler_info_down(f, &funcargreadf);

		while(true){ //reading args
			cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
			
			if(!cur_tok){
				f->errorcode=ORCUS_COMPERR_NO_MORE_TOKENS;
				f->token_error_num=token_offset;
				return 0;
			}else if(cur_tok->token_flags & TOKEN_MISC_OP && cur_tok->num_chars==1 && cur_tok->chars[0]==')'){
				break;
			}
			
			if(cur_tok->token_flags & TOKEN_ENV_VAR){
				add_instruction(&funcargreadf, ORCUS_OPCODE_NNSE, token_offset);
				add_instruction(&funcargreadf, ORCUS_OPCODE_SETL_KCS, token_offset);
				add_raw_string_onto_stack(&funcargreadf, cur_tok->chars, cur_tok->num_chars, token_offset);
				//add_string_onto_stack(&funcargreadf,cur_tok->chars,cur_tok->num_chars,token_offset);
				//token_offset+=m_compile_read_variable(funcargreadf,tokens,num_tokens,token_offset, nullptr); //load the var name onto the stack
				//add_instruction(funcargreadf,ORCUS_OPCODE_SWPT,token_offset); // we need the stack to be key, value. currently it's value 1, value 2, ..., key
				//add_instruction(&funcargreadf,ORCUS_OPCODE_SETL,token_offset);
				token_offset++;
			}else{
				f->errorcode=ORCUS_COMPERR_NO_NAMED_VARIABLE;
				f->token_error_num=token_offset;
				return 0;
			}
			
			cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
			if(!cur_tok){
				f->errorcode=ORCUS_COMPERR_NO_MORE_TOKENS;
				f->token_error_num=token_offset;
				return 0;
			}else if( !(cur_tok->token_flags & TOKEN_MISC_OP && cur_tok->num_chars==1 && (cur_tok->chars[0]==')' || cur_tok->chars[0]==',')  ) ){
				f->errorcode=ORCUS_COMPERR_WRONG_SYMBOL;
				f->token_error_num=token_offset;
				return 0;
			}
			if(cur_tok->chars[0] == ',')token_offset++;

		}
		
		cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
		if(!cur_tok){
			f->errorcode=ORCUS_COMPERR_NO_MORE_TOKENS;
			f->token_error_num=token_offset;
			return 0;
		}else if( !(cur_tok->token_flags & TOKEN_MISC_OP && cur_tok->num_chars==1 && cur_tok->chars[0]==')') ){
			f->errorcode=ORCUS_COMPERR_WRONG_SYMBOL;
			f->token_error_num=token_offset;
			return 0;
		}
		token_offset++;

		add_instruction(&funcargreadf,ORCUS_OPCODE_CLS,0); //discards extra args in
		compiler_function ff;
		memset(&ff, 0, sizeof(ff));
		ff.master = &ff;
		//do not set master to us, needs to be isolated.
		orcus_int toks_consumed=orcus_compile_tokens_to_bytecode(tokens, num_tokens, token_offset, &ff);
		if(ff.errorcode){
			f->errorcode=ff.errorcode;
			f->token_error_num=ff.token_error_num;
			return 0;
		}
		add_instruction(&ff, ORCUS_OPCODE_EXIT, token_offset);
		compiler_function_finalize_addendums(&ff);

		token_offset += toks_consumed;
		merge_compiler_functions_nofree(f, &funcargreadf);
		clear_compiler_function(&funcargreadf);
		merge_compiler_functions_nofree(f,&ff);
		clear_compiler_function(&ff);
		//*sizeoffunction = f->number_instructions-initial_bytecode_size;
		*(binarydata*)(f->instructions + initial_bytecode_size - ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE) = f->number_instructions - initial_bytecode_size;
		token_offset++; //advance 1 token to step over the ending end keyword
	}

	return token_offset- initial_offset;
}



orcus_int m_compile_read_unary_op(compiler_function* f, compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset){
	orcus_int initial_offset=token_offset;
	while(true){
		compiler_token* cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
		if(!cur_tok){
			break;
		}
		if(cur_tok->token_flags & TOKEN_UNARY_OP && !(cur_tok->token_flags & TOKEN_SELFMODIFY_OP) ){
			add_instruction_at_begining(f, m_compile_get_operator_opcode_from_token(cur_tok,1), token_offset); // use _at_begining so that !#a is interpreted as not length a, instead of length not a.
			token_offset++;
		}else{
			break;
		}
	}
	return token_offset-initial_offset;
}

orcus_int m_compile_read_binary_op(compiler_function* f, compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset){
	orcus_int initial_offset=token_offset;
	compiler_token* cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
	if(!cur_tok){
		return 0;
	}else if(cur_tok->token_flags & TOKEN_BINARY_OP && !(cur_tok->token_flags & TOKEN_SELFMODIFY_OP) ){
		add_instruction(f, m_compile_get_operator_opcode_from_token(cur_tok), token_offset);
		token_offset++;
	}
	else if (cur_tok->token_flags & TOKEN_STATIC_NUMBER && cur_tok->chars[0] == '-') {
		add_instruction(f, ORCUS_OPCODE_ADD, token_offset); //do not increment the token count, as we need to read it again to know what number we are adding.
	}else{
		return 0;
	}
	
	return token_offset-initial_offset;
}


orcus_int m_compile_read_var_indexing(compiler_function* f, compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset){
	orcus_int initial_offset=token_offset;
	
	while(true){
		compiler_token* cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
		if(!cur_tok || !(cur_tok->token_flags & TOKEN_MISC_OP) || cur_tok->num_chars!=1){ //soft break, since reading indexes is optional.
			break;
		}
		
		if(cur_tok->chars[0]=='.'){ //namespaced indexing
			token_offset++;
			compiler_token* cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
			if(!cur_tok){
				f->errorcode=ORCUS_COMPERR_NO_MORE_TOKENS;
				f->token_error_num=token_offset;
				return 0;
			}else if(!(cur_tok->token_flags & TOKEN_ENV_VAR)){
				f->errorcode=ORCUS_COMPERR_NO_NAMED_VARIABLE;
				f->token_error_num=token_offset;
				return 0;
			}else{
				//token_offset+=m_compile_read_variable(f,tokens,num_tokens,token_offset,i);
				//f->number_instructions--; //truncate the GENV instruction that will be added.
				//add_instruction(f, ORCUS_OPCODE_GET, token_offset);
				add_instruction(f, ORCUS_OPCODE_GET_KCS, token_offset);
				add_raw_string_onto_stack(f, cur_tok->chars, cur_tok->num_chars, token_offset);
				token_offset++;
			}
		}else if(cur_tok->chars[0]=='['){ //specific indexing
			token_offset++;
			orcus_int oao = f->num_args_out;
			f->num_args_out = 1;
			orcus_int o=m_compile_read_var_statment(f,tokens,num_tokens,token_offset);
			f->num_args_out = oao;
			if(f->errorcode){
				return 0;
			}else if(!o){
				f->errorcode = ORCUS_COMPERR_NO_VARIABLE;
				f->token_error_num=token_offset;
				return 0;
			}
			token_offset+=o;
			
			compiler_token* cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
			if(!cur_tok){
				f->errorcode=ORCUS_COMPERR_NO_MORE_TOKENS;
				f->token_error_num=token_offset;
				return 0;
			}else if(!(cur_tok->token_flags&TOKEN_MISC_OP && cur_tok->num_chars==1 && cur_tok->chars[0]==']')){
				f->errorcode=ORCUS_COMPERR_WRONG_SYMBOL;
				f->token_error_num=token_offset;
				return 0;
			}
			
			add_instruction(f, ORCUS_OPCODE_GET, token_offset);
			token_offset++;
		}else{
			break;
		}
		
	}
	
	return token_offset-initial_offset;
}



orcus_int m_compile_read_call_func(compiler_function* f, compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset, orcus_int* args_in){
	orcus_int initial_offset=token_offset;

	compiler_token* cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
	if(!cur_tok)return 0;
	if( !(cur_tok->token_flags & TOKEN_MISC_OP && cur_tok->num_chars==1 && cur_tok->chars[0]=='(') ){
		return 0;
	}
	token_offset++;

	orcus_int orig_args_out = f->num_args_out;

	while(true){
		f->num_args_out = 1;
		cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
		if(!cur_tok){
			f->errorcode=ORCUS_COMPERR_NO_MORE_TOKENS;
			f->token_error_num=token_offset;
			return 0;
		}else if( (cur_tok->token_flags & TOKEN_MISC_OP && cur_tok->num_chars==1 && cur_tok->chars[0]==')') ){
			token_offset++;
			break;
		}
		
		orcus_int o=m_compile_read_var_statment(f,tokens,num_tokens, token_offset);
		if(f->errorcode){
			return 0;
		}else if(!o){
			f->errorcode=ORCUS_COMPERR_NO_VARIABLE;
			f->token_error_num=token_offset;
			return 0;
		}
		(*args_in)++;
		token_offset+=o;
		
		cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
		if(!cur_tok){
			f->errorcode=ORCUS_COMPERR_NO_MORE_TOKENS;
			f->token_error_num=token_offset;
			return 0;
		}else if( (cur_tok->token_flags & TOKEN_MISC_OP && cur_tok->num_chars==1 && (cur_tok->chars[0]==')' || cur_tok->chars[0]==',') ) ){
			if(cur_tok->chars[0]==',')token_offset++;
		}else{
			f->errorcode=ORCUS_COMPERR_WRONG_SYMBOL;
			f->token_error_num=token_offset;
			return 0;
		}
		
	}
	f->num_args_out = orig_args_out;


	return token_offset-initial_offset;
}

//a var statment is something like 5+5
orcus_int m_compile_read_var_statment(compiler_function* f, compiler_token* tokens, orcus_int num_tokens, orcus_int token_offset, compiler_function* postfix_function){
	orcus_int initial_offset=token_offset;
	/*bytecode op order:
	1. get variable from environment
	2. index variable
	3. call function
	4. unary operation
	5. binary operation
	
	written op order:
	1. unary operation
	2. get variable from environment
	3. index variable
	4. call function
	5. binary operation*/
	
	compiler_token* cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
	//token_offset++;
	if(!cur_tok){
		//f->errorcode=ORCUS_COMPERR_NO_MORE_TOKENS;
		//f->token_error_num=token_offset;
		return 0;
	}
	compiler_function func_var;
	memset(&func_var, 0, sizeof(compiler_function));
	propigate_compiler_info_down(f, &func_var);
	compiler_function func_index;
	compiler_function func_call;
	compiler_function func_unary;
	memset(&func_unary, 0, sizeof(compiler_function));
	propigate_compiler_info_down(f, &func_unary);
	compiler_function func_binary;
	memset(&func_binary, 0, sizeof(compiler_function));
	propigate_compiler_info_down(f, &func_binary);
	
	token_offset+=m_compile_read_unary_op(&func_unary,tokens,num_tokens,token_offset);
	if(func_unary.errorcode){
		clear_compiler_function(&func_unary);
		f->errorcode=func_unary.errorcode;
		f->token_error_num=func_unary.token_error_num;
		return 0;
	}
	
	token_offset+=m_compile_read_variable(&func_var,tokens,num_tokens,token_offset);
	if(func_var.errorcode){
		f->errorcode = func_var.errorcode;
		f->token_error_num = func_var.token_error_num;
		clear_compiler_function(&func_unary);
		clear_compiler_function(&func_var);
		return 0;
	}else if(!func_var.number_instructions){
		clear_compiler_function(&func_unary);
		clear_compiler_function(&func_var);
		f->errorcode=ORCUS_COMPERR_NO_VARIABLE;
		f->token_error_num=token_offset;
		return 0;
	}
	
	compiler_function func_complete;
	
	bool firstpass = true;
	while (true) {
		memset(&func_complete, 0, sizeof(compiler_function));
		propigate_compiler_info_down(f, &func_complete);
		memset(&func_index, 0, sizeof(compiler_function));
		propigate_compiler_info_down(f, &func_index);
		memset(&func_call, 0, sizeof(compiler_function));
		propigate_compiler_info_down(f, &func_call);

		orcus_int vitokc = m_compile_read_var_indexing(&func_index, tokens, num_tokens, token_offset);
		token_offset += vitokc;
		if (func_index.errorcode) {
			clear_compiler_function(&func_unary);
			clear_compiler_function(&func_var);
			clear_compiler_function(&func_call);
			clear_compiler_function(&func_index);
			f->errorcode = func_index.errorcode;
			f->token_error_num = func_index.token_error_num;
			return 0;
		}

		orcus_int f_args_in = 0;
		orcus_int fctokc = m_compile_read_call_func(&func_call, tokens, num_tokens, token_offset, &f_args_in);
		token_offset += fctokc;
		if (func_call.errorcode) {
			clear_compiler_function(&func_unary);
			clear_compiler_function(&func_var);
			clear_compiler_function(&func_call);
			clear_compiler_function(&func_index);
			f->errorcode = func_call.errorcode;
			f->token_error_num = func_call.token_error_num;
			return 0;
		}





		if (fctokc) {
			merge_compiler_functions_nofree(&func_complete, &func_call); clear_compiler_function(&func_call);

			merge_compiler_functions_nofree(&func_complete, &func_var); clear_compiler_function(&func_var);
			merge_compiler_functions_nofree(&func_complete, &func_index); clear_compiler_function(&func_index);

			add_instruction(&func_complete, ORCUS_OPCODE_CALL, token_offset - 1);
			add_rawdata_bitwidth_size(&func_complete, f_args_in, token_offset - 1);
			add_rawdata_bitwidth_size(&func_complete, f->num_args_out, token_offset - 1); //args out
			f->num_args_out =0;
		}
		else if(vitokc){
			clear_compiler_function(&func_call);
			f->num_args_out--;
			merge_compiler_functions_nofree(&func_complete, &func_var); clear_compiler_function(&func_var);
			merge_compiler_functions_nofree(&func_complete, &func_index); clear_compiler_function(&func_index);
		}
		else{
			clear_compiler_function(&func_call);
			clear_compiler_function(&func_index);
			clear_compiler_function(&func_complete);
			if(firstpass)f->num_args_out--;
		}
		firstpass = false;
		//after reading a function call, read ahead a bit more to check for a chained function call, or further indexing. do this here because function calls are looked at last, and the compiler will throw and error trying to read a call as its own statment. for example, a()() (call the result of calling a), or a()[1] (index the result of calling a)
		if (!(vitokc+fctokc)) { //if we didn't consume any tokens, then we can just exit
			func_complete = func_var;
			break;
		}
		else { //otherwise, go back around and do it agian.
			func_var = func_complete;
			continue;
		}
	}

	merge_compiler_functions_nofree(&func_complete, &func_unary); clear_compiler_function(&func_unary);


	if (postfix_function) { merge_compiler_functions_nofree(&func_complete, postfix_function); clear_compiler_function(postfix_function); }
	
	
	token_offset+=m_compile_read_binary_op(&func_binary,tokens,num_tokens,token_offset);
	if(func_binary.errorcode){
		clear_compiler_function(&func_complete);
		clear_compiler_function(&func_binary);
		f->errorcode=func_binary.errorcode;
		f->token_error_num=func_binary.token_error_num;
		return 0;
	}
	if(func_binary.number_instructions){ //if we registered a binary operator, then we need to read ahead to get the other statement, which means we get to do this whole thing again.
		func_complete.num_args_out = 1;
		token_offset+=m_compile_read_var_statment(&func_complete, tokens, num_tokens, token_offset, &func_binary);
	}else{
		clear_compiler_function(&func_binary);
	}

	merge_compiler_functions_nofree(f, &func_complete);
	clear_compiler_function(&func_complete);

	//try to read ternery statment
	cur_tok = m_compile_get_next_token(tokens, num_tokens, token_offset);
	if (cur_tok && cur_tok->num_chars == 1 && cur_tok->chars[0] == '?' && (cur_tok->token_flags & (TOKEN_OPERATOR | TOKEN_MISC_OP)) ) {
		orcus_int oci = f->num_args_out;

		add_instruction(f, ORCUS_OPCODE_JRNI, token_offset);
		orcus_int jumppos_sidefalse = f->number_instructions;
		add_rawdata_bitwidth_size(f, 0, token_offset);
		f->num_args_out = 1;
		token_offset++;
		token_offset+=m_compile_read_var_statment(f, tokens, num_tokens, token_offset);
		if (f->errorcode)return 0;

		cur_tok = m_compile_get_next_token(tokens, num_tokens, token_offset);
		if (!(cur_tok && cur_tok->num_chars == 1 && cur_tok->chars[0] == ':' && (cur_tok->token_flags & (TOKEN_OPERATOR | TOKEN_MISC_OP)))) {
			f->errorcode = ORCUS_COMPERR_NO_VARIABLE;
			f->token_error_num = token_offset;
			return 0;
		}
		add_instruction(f, ORCUS_OPCODE_JMPR, token_offset);
		orcus_int jumppos_sidetrue = f->number_instructions;
		add_rawdata_bitwidth_size(f, 0, token_offset);

		*((orcus_int*)(f->instructions + jumppos_sidefalse)) = f->number_instructions - jumppos_sidefalse; //set jump to false section

		f->num_args_out = 1;
		token_offset++;
		token_offset += m_compile_read_var_statment(f, tokens, num_tokens, token_offset);
		if (f->errorcode)return 0;
		*((orcus_int*)(f->instructions + jumppos_sidetrue)) = f->number_instructions - jumppos_sidetrue; //set true section to jump over false secation.

		f->num_args_out = oci;

	}
	
	return token_offset-initial_offset;
}


//a statement is something like a=5, a[1]=5, a(5), or a[1](5)
orcus_int m_compile_read_statment(compiler_function* f, compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset){
	orcus_int initial_offset=token_offset;
	
	compiler_token* cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
	if(!cur_tok){
		return 0;
	}

	if(cur_tok->token_flags & TOKEN_KEYWORD && token_matches_chars(cur_tok,"return") ){
		//this is tricky, since we have to discern return a=5 and return a. one of these is a statement, while the other is a variable. why even compile code after return? well, goto exists, for one, and the cursed code that this will inevitably cause will be more than worth it. but why even allow that, you may ask. well, the compiler's job is to make code run, not pass judgments about how shitty your code is, so pass judgments we will not.
		//well actually, having said that, i've used jump after return in this code so uh... you go get that 1% speed improvment, king!
		token_offset++;
		while(true){
			compiler_function n;
			memset(&n, 0, sizeof(compiler_function));
			propigate_compiler_info_down(f, &n);
			n.num_args_out = 1;
			orcus_int o=m_compile_read_var_statment(&n,tokens,num_tokens,token_offset);
			cur_tok = m_compile_get_next_token(tokens, num_tokens, token_offset + o);
			if(!cur_tok || (cur_tok->token_flags & TOKEN_MISC_OP && cur_tok->num_chars==1 && cur_tok->chars[0]=='=')){
				clear_compiler_function(&n);
				break;
			}
			if(n.errorcode){
				//f->errorcode=n->errorcode;
				//f->token_error_num=n->token_error_num;
				clear_compiler_function(&n);
				break; //we want to break instead, because reading an arg is optional, and m_compile_read_var_statment will error if there's no arg.
			}
			merge_compiler_functions_nofree(f,&n);
			clear_compiler_function(&n);
			token_offset+=o;
			if( !(cur_tok->token_flags & TOKEN_MISC_OP && cur_tok->num_chars==1 && cur_tok->chars[0]==',')){
				break;
			}
			token_offset++;
		}
		add_instruction(f, ORCUS_OPCODE_EXIT, token_offset);
		return token_offset-initial_offset;
	}else if(cur_tok->token_flags & (TOKEN_KEYWORD | TOKEN_LOOP_MODIFIER)){
		return 0;
	}else if (cur_tok->token_flags & TOKEN_ENV_VAR && num_tokens>token_offset+1) {//goto labels
		compiler_token* nt = tokens+token_offset + 1;
		if (nt->token_flags & TOKEN_MISC_OP && nt->num_chars == 1 && nt->chars[0] == ':') {
			if (!compiler_add_goto_label(f, cur_tok, f->number_instructions)) {
				f->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
				f->token_error_num = token_offset;
				return 0;
			}
			return 2;
		}
	}



	f->num_args_out = 0;
	orcus_opcode* set_instructs = nullptr;
	orcus_opcode get_instruction = ORCUS_OPCODE_GENV;
	orcus_opcode set_instruction = ORCUS_OPCODE_SENV;
	compiler_function* varfuncs = nullptr;
	compiler_function* vargetresidualfuncs = nullptr;
	compiler_function* varsetresidualfuncs = nullptr;
	while (true) {
		void* nptr=realloc(set_instructs, sizeof(orcus_opcode) * (f->num_args_out + 1));
		if (!nptr) {
			f->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
			f->token_error_num = token_offset;
			free(varfuncs);
			free(set_instructs);
			free(vargetresidualfuncs);
			free(varsetresidualfuncs);
			return 0;
		}
		set_instructs = (orcus_opcode*)nptr;

		nptr = realloc(varfuncs, sizeof(compiler_function) * (f->num_args_out + 1));
		if (!nptr) {
			f->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
			f->token_error_num = token_offset;
			free(varfuncs);
			free(set_instructs);
			free(vargetresidualfuncs);
			free(varsetresidualfuncs);
			return 0;
		}
		varfuncs = (compiler_function*)nptr;
		memset(varfuncs + f->num_args_out,0,sizeof(compiler_function));
		propigate_compiler_info_down(f, varfuncs + f->num_args_out);

		nptr = realloc(vargetresidualfuncs, sizeof(compiler_function) * (f->num_args_out + 1));
		if (!nptr) {
			f->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
			f->token_error_num = token_offset;
			free(varfuncs);
			free(set_instructs);
			free(vargetresidualfuncs);
			free(varsetresidualfuncs);
			return 0;
		}
		vargetresidualfuncs = (compiler_function*)nptr;
		memset(vargetresidualfuncs + f->num_args_out, 0, sizeof(compiler_function));
		propigate_compiler_info_down(f, vargetresidualfuncs + f->num_args_out);

		nptr = realloc(varsetresidualfuncs, sizeof(compiler_function) * (f->num_args_out + 1));
		if (!nptr) {
			f->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
			f->token_error_num = token_offset;
			free(varfuncs);
			free(set_instructs);
			free(vargetresidualfuncs);
			free(varsetresidualfuncs);
			return 0;
		}
		varsetresidualfuncs = (compiler_function*)nptr;
		memset(varsetresidualfuncs + f->num_args_out, 0, sizeof(compiler_function));
		propigate_compiler_info_down(f, varsetresidualfuncs + f->num_args_out);

		get_instruction = ORCUS_OPCODE_GENV_KCS;
		set_instruction = ORCUS_OPCODE_SENV_KCS;

		cur_tok = m_compile_get_next_token(tokens, num_tokens, token_offset);
		if (cur_tok->token_flags & TOKEN_SCOPESPECIFIER) { //find local/global keyword first

			if (token_matches_chars(cur_tok, "local")) {
				get_instruction = ORCUS_OPCODE_GETL_KCS;
				set_instruction = ORCUS_OPCODE_SETL_KCS;
			}
			else {
				get_instruction = ORCUS_OPCODE_GETG_KCS;
				set_instruction = ORCUS_OPCODE_SETG_KCS;
			}

			token_offset++;
			cur_tok = m_compile_get_next_token(tokens, num_tokens, token_offset);
			if (!cur_tok) {
				f->token_error_num = token_offset;
				f->errorcode = ORCUS_COMPERR_NO_MORE_TOKENS;
				for (orcus_int n = 0; n < f->num_args_out; n++) {
					clear_compiler_function(varfuncs + n);
					clear_compiler_function(vargetresidualfuncs + n);
					clear_compiler_function(varsetresidualfuncs + n);
				}
				free(varfuncs);
				free(vargetresidualfuncs);
				free(varsetresidualfuncs);
				free(set_instructs);
				return 0;
			}
		}

		//then find the variable we'll be interfacing with
		if (!(cur_tok->token_flags & TOKEN_ENV_VAR)) {
			f->token_error_num = token_offset;
			f->errorcode = ORCUS_COMPERR_NO_NAMED_VARIABLE;
			for (orcus_int n = 0; n < f->num_args_out; n++) {
				clear_compiler_function(varfuncs + n);
				clear_compiler_function(vargetresidualfuncs + n);
				clear_compiler_function(varsetresidualfuncs + n);
			}
			free(varfuncs);
			free(vargetresidualfuncs);
			free(varsetresidualfuncs);
			free(set_instructs);
			return 0;
		}


		compiler_function* funcvarblock = varfuncs + f->num_args_out;//new_compiler_function();
		compiler_function* funcvarget = vargetresidualfuncs + f->num_args_out;//new_compiler_function();
		compiler_function* funcvarset = varsetresidualfuncs + f->num_args_out;//new_compiler_function();

		add_instruction(funcvarget, get_instruction, token_offset);
		add_raw_string_onto_stack(funcvarget, cur_tok->chars, cur_tok->num_chars, token_offset);
		add_instruction(funcvarset, set_instruction, token_offset);
		add_raw_string_onto_stack(funcvarset, cur_tok->chars, cur_tok->num_chars, token_offset);
		token_offset++;
		//*/

		//check if we're indexing something about it.
		bool should_set = true;
		orcus_uint* function_output_args_point = nullptr;
		while (true) {
			cur_tok = m_compile_get_next_token(tokens, num_tokens, token_offset);
			if (!cur_tok) {
				if (!should_set)break;
				f->token_error_num = token_offset;
				f->errorcode = ORCUS_COMPERR_NO_MORE_TOKENS;
				for (orcus_int n = 0; n <= f->num_args_out; n++) {
					clear_compiler_function(varfuncs + n);
					clear_compiler_function(vargetresidualfuncs + n);
					clear_compiler_function(varsetresidualfuncs + n);
				}
				free(varfuncs);
				free(vargetresidualfuncs);
				free(varsetresidualfuncs);
				free(set_instructs);
				return 0;
			}
			else if (cur_tok->token_flags & TOKEN_MISC_OP && cur_tok->num_chars == 1 && (cur_tok->chars[0] == '[' || cur_tok->chars[0] == '.')) {
				if (function_output_args_point) {
					*function_output_args_point = 1;
					function_output_args_point = nullptr;
				}
				should_set = true;

				merge_compiler_functions_nofree(funcvarblock, funcvarget);
				clear_compiler_function(funcvarget);
				clear_compiler_function(funcvarset);
				

				
				token_offset++;
				if (cur_tok->chars[0] == '[') { //explicit index
					get_instruction = ORCUS_OPCODE_GET;
					set_instruction = ORCUS_OPCODE_SET;
					funcvarblock->num_args_out = 1;
					token_offset += m_compile_read_var_statment(funcvarblock, tokens, num_tokens, token_offset);
					cur_tok = m_compile_get_next_token(tokens, num_tokens, token_offset);
					if (!(cur_tok && cur_tok->token_flags & TOKEN_MISC_OP && cur_tok->num_chars == 1 && cur_tok->chars[0] == ']')) {
						f->token_error_num = token_offset;
						f->errorcode = ORCUS_COMPERR_WRONG_SYMBOL;
						for (orcus_int n = 0; n <= f->num_args_out; n++) {
							clear_compiler_function(varfuncs + n);
							clear_compiler_function(vargetresidualfuncs + n);
							clear_compiler_function(varsetresidualfuncs + n);
						}
						free(varfuncs);
						free(vargetresidualfuncs);
						free(varsetresidualfuncs);
						free(set_instructs);
						return 0;
					}
					add_instruction(funcvarget, get_instruction, token_offset);
					add_instruction(funcvarset, set_instruction, token_offset);
					token_offset++;
				}
				else { //implicit index
					get_instruction = ORCUS_OPCODE_GET_KCS;
					set_instruction = ORCUS_OPCODE_SET_KCS;
					cur_tok = m_compile_get_next_token(tokens, num_tokens, token_offset);
					if (!(cur_tok)) {
						f->token_error_num = token_offset;
						f->errorcode = ORCUS_COMPERR_NO_MORE_TOKENS;
						for (orcus_int n = 0; n <= f->num_args_out; n++) {
							clear_compiler_function(varfuncs + n);
							clear_compiler_function(vargetresidualfuncs + n);
							clear_compiler_function(varsetresidualfuncs + n);
						}
						free(varfuncs);
						free(vargetresidualfuncs);
						free(varsetresidualfuncs);
						free(set_instructs);
						return 0;
					}
					else if (!(cur_tok->token_flags & TOKEN_ENV_VAR)) {
						f->token_error_num = token_offset;
						f->errorcode = ORCUS_COMPERR_NO_NAMED_VARIABLE;
						for (orcus_int n = 0; n <= f->num_args_out; n++) {
							clear_compiler_function(varfuncs + n);
							clear_compiler_function(vargetresidualfuncs + n);
							clear_compiler_function(varsetresidualfuncs + n);
						}
						free(varfuncs);
						free(vargetresidualfuncs);
						free(varsetresidualfuncs);
						free(set_instructs);
						return 0;
					}
					//token_offset += m_compile_read_variable(getvarfunc, tokens, num_tokens, token_offset, i);
					//getvarfunc->number_instructions--; //remove the last instruction, which would be GENV. hacky, i know.
					add_instruction(funcvarget, ORCUS_OPCODE_GET_KCS,token_offset);
					add_raw_string_onto_stack(funcvarget, cur_tok->chars, cur_tok->num_chars, token_offset);
					add_instruction(funcvarset, ORCUS_OPCODE_SET_KCS, token_offset);
					add_raw_string_onto_stack(funcvarset, cur_tok->chars, cur_tok->num_chars, token_offset);
					/*
					orcus_int ci = compiler_info_add_constant_string_from_token(i, cur_tok);
					if (ci == -1) {
						f->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
						f->token_error_num = token_offset;
						return 0;
					}
					add_instruction(getvarfunc, ORCUS_OPCODE_GCON, token_offset);
					add_rawdata_bitwidth_size(getvarfunc, ci, token_offset);*/
					token_offset++;
				}

			}
			else if (token_matches_chars(cur_tok, "(")) { //function call
				if (function_output_args_point) {
					*function_output_args_point = 1;
					function_output_args_point = nullptr;
				}
				orcus_int ito = token_offset;
				orcus_int ain = 0;
				if (should_set) {
					merge_compiler_functions_nofree(funcvarblock, funcvarget);
					clear_compiler_function(funcvarget);
					clear_compiler_function(funcvarset);
				}
				else {
					clear_compiler_function(funcvarget);
					clear_compiler_function(funcvarset);
				}
				compiler_function prepend;
				memset(&prepend + f->num_args_out, 0, sizeof(compiler_function));
				propigate_compiler_info_down(f, &prepend);

				token_offset += m_compile_read_call_func(&prepend, tokens, num_tokens, token_offset, &ain); //append to f because function args come before the function.
				if (prepend.errorcode) {
					f->errorcode = prepend.errorcode;
					f->token_error_num = prepend.token_error_num;
					for (orcus_int n = 0; n <= f->num_args_out; n++) {
						clear_compiler_function(varfuncs + n);
						clear_compiler_function(vargetresidualfuncs + n);
						clear_compiler_function(varsetresidualfuncs + n);
					}
					free(varfuncs);
					free(vargetresidualfuncs);
					free(varsetresidualfuncs);
					free(set_instructs);
					clear_compiler_function(&prepend);
					return 0;
				}
				add_instruction(funcvarblock, ORCUS_OPCODE_CALL, ito);
				add_rawdata_bitwidth_size(funcvarblock, ain, ito);
				add_rawdata_bitwidth_size(funcvarblock, 0, ito);

				merge_compiler_functions_nofree(&prepend, funcvarblock);
				clear_compiler_function(funcvarblock);
				*funcvarblock = prepend;

				function_output_args_point = (orcus_uint*)((funcvarblock->instructions) + (funcvarblock->number_instructions) - sizeof(orcus_uint) / sizeof(orcus_opcode)); //put this here because the instructions pointer will move around during the earlier code.
				get_instruction = ORCUS_OPCODE_GET;
				set_instruction = ORCUS_OPCODE_SET;
				should_set = false;
			}
			else
			{
				break;
			}


		}

		//varfuncs[i->args_out] = getvarfunc;


		if (should_set) {
			set_instructs[f->num_args_out] = set_instruction;
			f->num_args_out++;
		}
		else if (f->num_args_out) {
			f->errorcode = ORCUS_COMPERR_REQUIRES_EXPLICIT_ASSIGNMENT;
			f->token_error_num = token_offset;
			return 0;
		}

		cur_tok = m_compile_get_next_token(tokens, num_tokens, token_offset);
		if (!cur_tok) {
			if (f->num_args_out) {
				f->token_error_num = token_offset;
				f->errorcode = ORCUS_COMPERR_NO_MORE_TOKENS;
				for (orcus_int n = 0; n < f->num_args_out; n++) {
					clear_compiler_function(varfuncs + n);
					clear_compiler_function(vargetresidualfuncs + n);
					clear_compiler_function(varsetresidualfuncs + n);
				}
				free(varfuncs);
				free(vargetresidualfuncs);
				free(varsetresidualfuncs);
				free(set_instructs);
				return 0;
			}
			else {
				break;
			}
		}
		else if (cur_tok->num_chars==1 && cur_tok->chars[0]==',' && (cur_tok->token_flags&TOKEN_MISC_OP) ) {
			token_offset++;
			continue;
		}
		else {
			break;
		}

	}


	//then check what we're doing with this. this is either a set operation, or a self-modifying set operation
	if(f->num_args_out){
		cur_tok = m_compile_get_next_token(tokens, num_tokens, token_offset);
		if (!cur_tok) {
			f->token_error_num = token_offset;
			f->errorcode = ORCUS_COMPERR_NO_MORE_TOKENS;
			for (orcus_int n = 0; n < f->num_args_out; n++) {
				clear_compiler_function(varfuncs + n);
				clear_compiler_function(vargetresidualfuncs + n);
				clear_compiler_function(varsetresidualfuncs + n);
			}
			free(varfuncs);
			free(vargetresidualfuncs);
			free(varsetresidualfuncs);
			free(set_instructs);
			return 0;
		}
		else if (!(cur_tok->token_flags & (TOKEN_MISC_OP | TOKEN_SELFMODIFY_OP))) {
			f->token_error_num = token_offset;
			f->errorcode = ORCUS_COMPERR_WRONG_SYMBOL;
			for (orcus_int n = 0; n < f->num_args_out; n++) {
				clear_compiler_function(varfuncs + n);
				clear_compiler_function(vargetresidualfuncs + n);
				clear_compiler_function(varsetresidualfuncs + n);
			}
			free(varfuncs);
			free(vargetresidualfuncs);
			free(varsetresidualfuncs);
			free(set_instructs);
			return 0;
		}

		orcus_int ito = token_offset;
		if (token_matches_chars(cur_tok, "=")) { //assignment
			token_offset++;
			//merge_compiler_functions(f, getvarfunc);
			

			compiler_function assfunc;
			memset(&assfunc, 0, sizeof(compiler_function));
			propigate_compiler_info_down(f, &assfunc);
			orcus_int total_args_out = f->num_args_out;
			while (f->num_args_out) {
				//i->args_out--; //decrement args out in the m_compile_read_var_statment function call.

				compiler_function argfunc;
				memset(&argfunc, 0, sizeof(compiler_function));
				propigate_compiler_info_down(f, &argfunc);
				argfunc.num_args_out = f->num_args_out;
				

				orcus_int starting_args_out = f->num_args_out;

				token_offset += m_compile_read_var_statment(&argfunc, tokens, num_tokens, token_offset);
				f->num_args_out = argfunc.num_args_out;
				if (argfunc.errorcode) {
					f->errorcode = argfunc.errorcode;
					f->token_error_num = argfunc.token_error_num;
					for (orcus_int n = 0; n < total_args_out; n++) {
						clear_compiler_function(varfuncs + n);
						clear_compiler_function(vargetresidualfuncs + n);
						clear_compiler_function(varsetresidualfuncs + n);
					}
					free(varfuncs);
					free(vargetresidualfuncs);
					free(varsetresidualfuncs);
					free(set_instructs);
					return 0;
				}

				

				orcus_int args_processed = starting_args_out - f->num_args_out;
				if (args_processed > 1) {
					
					for (orcus_int n = f->num_args_out; n < starting_args_out; n++) {
						merge_compiler_functions_nofree(&argfunc, varfuncs + total_args_out - n - 1);
						clear_compiler_function(varfuncs + total_args_out - n - 1);
						//add_instruction(&argfunc, set_instructs[n], ito);
						merge_compiler_functions_nofree(&argfunc, varsetresidualfuncs + total_args_out - n - 1);
						clear_compiler_function(varsetresidualfuncs + total_args_out - n - 1);

						clear_compiler_function(vargetresidualfuncs + total_args_out - n - 1);
					}
				}
				else {
					merge_compiler_functions_nofree(&argfunc, varfuncs + total_args_out - f->num_args_out -1);
					clear_compiler_function(varfuncs + total_args_out - f->num_args_out - 1);
					//add_instruction(&argfunc, set_instructs[i->args_out], ito);
					merge_compiler_functions_nofree(&argfunc, varsetresidualfuncs + total_args_out - f->num_args_out - 1);
					clear_compiler_function(varsetresidualfuncs + total_args_out - f->num_args_out - 1);

					clear_compiler_function(vargetresidualfuncs + total_args_out - f->num_args_out - 1);
				}

				cur_tok = m_compile_get_next_token(tokens, num_tokens, token_offset);
				if (f->num_args_out) { //if we need another arg...
					if (!cur_tok) { //we need tokens
						f->token_error_num = token_offset;
						f->errorcode = ORCUS_COMPERR_NO_MORE_TOKENS;
						return 0;
					}
					else if (cur_tok->num_chars == 1 && cur_tok->chars[0] == ',' && (cur_tok->token_flags & TOKEN_MISC_OP)) { //and need a comma seperator
						token_offset++;

						//merge_compiler_functions(assfunc, argfunc);
						merge_compiler_functions_nofree(&argfunc, &assfunc);
						clear_compiler_function(&assfunc);
						assfunc = argfunc;

						continue;
					}
					else {
						f->errorcode = ORCUS_COMPERR_REQUIRES_EXPLICIT_ASSIGNMENT;
						f->token_error_num = token_offset;
						for (orcus_int n = 0; n < total_args_out; n++) {
							clear_compiler_function(varfuncs + n);
							clear_compiler_function(vargetresidualfuncs + n);
							clear_compiler_function(varsetresidualfuncs + n);
						}
						free(varfuncs);
						free(vargetresidualfuncs);
						free(varsetresidualfuncs);
						free(set_instructs);
						return 0;
					}
				}

				//merge_compiler_functions(assfunc, argfunc);
				merge_compiler_functions_nofree(&argfunc, &assfunc);
				clear_compiler_function(&assfunc);
				assfunc = argfunc;
				

			}

			merge_compiler_functions_nofree(f, &assfunc);
			clear_compiler_function(&assfunc);

		}
		else if (cur_tok->token_flags & TOKEN_SELFMODIFY_OP) { //assignment & modification
			if (f->num_args_out >1) { //self-modification can only work on 1 var.
				f->errorcode = ORCUS_COMPERR_REQUIRES_EXPLICIT_ASSIGNMENT;
				f->token_error_num = token_offset;
				for (orcus_int n = 0; n < f->num_args_out; n++) {
					clear_compiler_function(varfuncs + n);
					clear_compiler_function(vargetresidualfuncs + n);
					clear_compiler_function(varsetresidualfuncs + n);
				}
				free(varfuncs);
				free(vargetresidualfuncs);
				free(varsetresidualfuncs);
				free(set_instructs);
				return 0;
			}

			merge_compiler_functions_nofree(f, varfuncs);
			clear_compiler_function(varfuncs);
			// stack: table, key
			// we of course need to do both get and set in one line, so we have to get a bit tricky about the stack variables, so...
			if (get_instruction == ORCUS_OPCODE_GET) {
				add_instruction(f, ORCUS_OPCODE_CPYX, ito);
				add_rawdata_bitwidth_size(f, 2, ito);
			}
			else if(get_instruction == ORCUS_OPCODE_GET_KCS || get_instruction == ORCUS_OPCODE_GENV) { //for enviromental vars, we only have the key on the stack, since the table is implicit. for static key gets, we only have the table on the stack and will copy that.
				add_instruction(f, ORCUS_OPCODE_CPYT, ito);
			}
			else { //other cases have nothing initially on the stack.

			}
			//stack: table, key, table, key
			merge_compiler_functions_nofree(f, vargetresidualfuncs);
			clear_compiler_function(vargetresidualfuncs);

			//stack: table, key, value
			//horray, we did it. well, actually no, we still need to read the rest of the statment.
			orcus_opcode insop = m_compile_get_operator_opcode_from_token(cur_tok);
			token_offset++;

			if (cur_tok->token_flags & TOKEN_BINARY_OP) {
				orcus_int oao= f->num_args_out;
				f->num_args_out = 1;
				token_offset += m_compile_read_var_statment(f, tokens, num_tokens, token_offset);
				f->num_args_out = oao;
				if (f->errorcode) {
					free(varfuncs);
					free(vargetresidualfuncs);
					free(varsetresidualfuncs);
					free(set_instructs);
					return 0;
				}
			}

			add_instruction(f, insop, ito);

			if (get_instruction == ORCUS_OPCODE_GET){ //this is a bit annoying, TBH. we need to go from 1,2,3 to 3,1,2 (table, key, value -> value,table,key)
				add_instruction(f, ORCUS_OPCODE_SWXY, ito);
				add_rawdata_bitwidth_size(f, 0, ito);
				add_rawdata_bitwidth_size(f, 2, ito);
				add_instruction(f, ORCUS_OPCODE_SWXY, ito);
				add_rawdata_bitwidth_size(f, 0, ito);
				add_rawdata_bitwidth_size(f, 1, ito);
			}
			else if(get_instruction==ORCUS_OPCODE_GENV || get_instruction==ORCUS_OPCODE_GET_KCS){ // key, value-> value, key
				add_instruction(f, ORCUS_OPCODE_SWPT, ito);
			}else{ //no stack manipulation needed for this.

			}

			//add_instruction(f, set_instruction, ito);
			merge_compiler_functions_nofree(f, varsetresidualfuncs);
			clear_compiler_function(varsetresidualfuncs);
		}
		else {
			//delete_compiler_function(getvarfunc);
			f->token_error_num = token_offset;
			f->errorcode = ORCUS_COMPERR_NO_OPERATION_FOUND;
			for (orcus_int n = 0; n < f->num_args_out; n++) {
				clear_compiler_function(varfuncs + n);
				clear_compiler_function(vargetresidualfuncs + n);
				clear_compiler_function(varsetresidualfuncs + n);
			}
			free(varfuncs);
			free(vargetresidualfuncs);
			free(varsetresidualfuncs);
			free(set_instructs);
			return 0;
		}
	}
	else { // !should_set
		merge_compiler_functions_nofree(f, varfuncs);
		clear_compiler_function(varfuncs);
	}

	free(varfuncs);
	free(vargetresidualfuncs);
	free(varsetresidualfuncs);
	free(set_instructs);
	
	return token_offset-initial_offset;
}



orcus_int m_compile_parse_while_loop(compiler_function* f, compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset){
	orcus_int initial_offset=token_offset;
	compiler_token* cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
	
	if( !(cur_tok&& cur_tok->token_flags & TOKEN_KEYWORD && token_matches_chars(cur_tok,"while")) ){
		return 0;
	}
	token_offset++;
	
	bool former_in_loop = f->in_loop;
	orcus_int former_num_loop_jumptostart = f->num_loop_jumptostart;
	orcus_int* former_loop_jumptostarts = f->loop_jumptostarts;
	orcus_int former_num_loop_jumptoend=f->num_loop_jumptoend;
	orcus_int* former_loop_jumptoends=f->loop_jumptoends;
	
	orcus_int loop_starting_position= f->number_instructions;

	f->in_loop = true;
	f->num_loop_jumptoend = 0;
	f->loop_jumptoends = nullptr;
	f->num_loop_jumptostart = 0;
	f->loop_jumptostarts = nullptr;

	orcus_int oao = f->num_args_out;
	f->num_args_out = 1;
	token_offset+=m_compile_read_var_statment(f,tokens,num_tokens,token_offset);
	f->num_args_out = oao;
	if(f->errorcode)return 0;
	
	add_instruction(f, ORCUS_OPCODE_JRNI, token_offset);
	compiler_add_loop_endjump(f,f->number_instructions);
	add_rawdata_bitwidth_size(f,0,token_offset);
	
	cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
	if(!cur_tok){
		f->errorcode = ORCUS_COMPERR_NO_MORE_TOKENS;
		f->token_error_num = token_offset;
		return 0;
	} else if( !(cur_tok->token_flags & TOKEN_KEYWORD && token_matches_chars(cur_tok,"do")) ){
		f->errorcode = ORCUS_COMPERR_WHILE_NEEDS_DO;
		f->token_error_num = token_offset;
		return 0;
	}
	token_offset++;
	
	while(true){
		orcus_int consumed=0;
		compiler_function bf;
		memset(&bf, 0, sizeof(bf));
		propigate_compiler_info_down(f, &bf);
		consumed=orcus_compile_tokens_to_bytecode(tokens, num_tokens, token_offset, &bf);
		if (bf.errorcode) {
			f->errorcode=bf.errorcode;
			f->token_error_num=bf.token_error_num;
			return 0;
		}
		
		merge_compiler_functions_nofree(f,&bf);
		clear_compiler_function(&bf);
		
		token_offset+=consumed;
		
		cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
		if(!cur_tok){
			f->errorcode = ORCUS_COMPERR_NO_MORE_TOKENS;
			f->token_error_num = token_offset;
			return 0;
		}else if(cur_tok->token_flags & TOKEN_KEYWORD && token_matches_chars(cur_tok,"end") ){
			token_offset++;
			break;
		}else{
			f->errorcode = ORCUS_COMPERR_WRONG_SYMBOL;
			f->token_error_num=token_offset;
			return 0;
		}
		
	}
	
	add_instruction(f, ORCUS_OPCODE_JMPR, token_offset-1);
	add_rawdata_bitwidth_size(f, loop_starting_position -f->number_instructions,token_offset-1);

	for(orcus_int i=0;i<f->num_loop_jumptoend;i++){
		*(orcus_int*)(f->instructions + f->loop_jumptoends[i]) = f->number_instructions - f->loop_jumptoends[i];
	}

	for (orcus_int i = 0; i < f->num_loop_jumptostart; i++) {
		*(orcus_int*)(f->instructions + f->loop_jumptostarts[i]) = loop_starting_position- f->loop_jumptostarts[i];
	}
	free(f->loop_jumptoends);
	free(f->loop_jumptostarts);
	
	f->in_loop = former_in_loop;
	f->num_loop_jumptoend = former_num_loop_jumptoend;
	f->loop_jumptoends = former_loop_jumptoends;
	f->num_loop_jumptostart = former_num_loop_jumptostart;
	f->loop_jumptostarts = former_loop_jumptostarts;

	return token_offset-initial_offset;
}

orcus_int m_compile_parse_if_statement(compiler_function* f, compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset){
	orcus_int initial_offset=token_offset;
	compiler_token* cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
	
	if( !(cur_tok&& cur_tok->token_flags & TOKEN_KEYWORD && token_matches_chars(cur_tok,"if")) ){
		return 0;
	}
	token_offset++;

	orcus_int oao = f->num_args_out;
	f->num_args_out = 1;
	orcus_int o=m_compile_read_var_statment(f,tokens,num_tokens,token_offset);
	f->num_args_out = oao;
	if(f->errorcode){
		return 0;
	}else if(!o){
		f->errorcode=ORCUS_COMPERR_NO_VARIABLE;
		f->token_error_num=token_offset;
		return 0;
	}
	token_offset+=o;
	
	cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
	if( !(cur_tok&& cur_tok->token_flags & TOKEN_KEYWORD && token_matches_chars(cur_tok,"then")) ){
		f->errorcode=ORCUS_COMPERR_IF_NEEDS_THEN;
		f->token_error_num=token_offset;
		return 0;
	}
	
	bool defaultcase = false;
	orcus_int init_jump_point;
	orcus_int num_jumptoends = 0;
	orcus_int* jump_to_end_points = nullptr;
	
	add_instruction(f, ORCUS_OPCODE_JRNI, token_offset);
	init_jump_point = f->number_instructions;
	add_rawdata_bitwidth_size(f,0,token_offset);
	token_offset++;


	while(true){
		orcus_int consumed=0;
		compiler_function bf;
		memset(&bf, 0, sizeof(bf));
		propigate_compiler_info_down(f, &bf);
		consumed=orcus_compile_tokens_to_bytecode(tokens, num_tokens, token_offset, &bf);
		if(bf.errorcode){
			f->errorcode=bf.errorcode;
			f->token_error_num=bf.token_error_num;
			return 0;
		}
		merge_compiler_functions_nofree(f,&bf);
		clear_compiler_function(&bf);
		token_offset+=consumed;
		
		cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
		if(!cur_tok){
			f->errorcode = ORCUS_COMPERR_NO_MORE_TOKENS;
			f->token_error_num = token_offset;
			return 0;
		}else if(cur_tok->token_flags & TOKEN_KEYWORD && token_matches_chars(cur_tok,"elseif") ){
			add_instruction(f, ORCUS_OPCODE_JMPR, token_offset);

			orcus_int* njtep=(orcus_int*)realloc(jump_to_end_points, sizeof(orcus_int) * (num_jumptoends + 1));
			if (!njtep) {
				f->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
				f->token_error_num = token_offset;
				return 0;
			}
			jump_to_end_points = njtep;
			jump_to_end_points[num_jumptoends] = f->number_instructions;
			num_jumptoends++;

			add_rawdata_bitwidth_size(f,0,token_offset);
			token_offset++;

			*(orcus_int*)(f->instructions + init_jump_point) = f->number_instructions - init_jump_point;
			
			orcus_int oao = f->num_args_out;
			f->num_args_out = 1;
			o=m_compile_read_var_statment(f,tokens,num_tokens,token_offset);
			f->num_args_out = oao;
			if(f->errorcode){
				return 0;
			}else if(!o){
				f->errorcode=ORCUS_COMPERR_NO_VARIABLE;
				f->token_error_num=token_offset;
				return 0;
			}
			token_offset+=o;
			
			add_instruction(f, ORCUS_OPCODE_JRNI, token_offset);
			init_jump_point =f->number_instructions;
			add_rawdata_bitwidth_size(f,0,token_offset);
			
			cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
			if( !(cur_tok&& cur_tok->token_flags & TOKEN_KEYWORD && token_matches_chars(cur_tok,"then")) ){
				f->errorcode=ORCUS_COMPERR_IF_NEEDS_THEN;
				f->token_error_num=token_offset;
				return 0;
			}
			token_offset++;
			
		}else if(cur_tok->token_flags & TOKEN_KEYWORD && token_matches_chars(cur_tok,"else") ){
			add_instruction(f, ORCUS_OPCODE_JMPR, token_offset);

			orcus_int* njtep = (orcus_int*)realloc(jump_to_end_points, sizeof(orcus_int) * (num_jumptoends + 1));
			if (!njtep) {
				f->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
				f->token_error_num = token_offset;
				return 0;
			}
			jump_to_end_points = njtep;
			jump_to_end_points[num_jumptoends] = f->number_instructions;
			num_jumptoends++;

			add_rawdata_bitwidth_size(f, 0, token_offset);

			token_offset++;
			*(orcus_int*)(f->instructions + init_jump_point) = f->number_instructions - init_jump_point;
			defaultcase = true;
		}else if(cur_tok->token_flags & TOKEN_KEYWORD && token_matches_chars(cur_tok,"end") ){
			token_offset++;
			
			if(!defaultcase){ //if we didn't fall through to else, we have to set the jump point of the last if check to here.
				*(orcus_int*)(f->instructions+ init_jump_point) = f->number_instructions- init_jump_point;
			}
			
			for(orcus_int i=0;i< num_jumptoends;i++){
				*(orcus_int*)(f->instructions+ jump_to_end_points[i]) = f->number_instructions - jump_to_end_points[i];
			}
			
			break;
		}else{
			f->errorcode = ORCUS_COMPERR_WRONG_SYMBOL;
			f->token_error_num=token_offset;
			return 0;
		}
		
	}
	
	free(jump_to_end_points);

	return token_offset-initial_offset;
}


orcus_int m_compile_read_keyword_block(compiler_function* f, compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset){
	orcus_int initial_offset=token_offset;
	
	compiler_token* cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
	if(!cur_tok){
		return 0;
	}else if(!(cur_tok->token_flags & TOKEN_KEYWORD) ){
		return 0;
	}
	
	if(token_matches_chars(cur_tok,"while")){
		token_offset+=m_compile_parse_while_loop(f,tokens,num_tokens,token_offset);
	}else if(token_matches_chars(cur_tok,"if")){
		token_offset+=m_compile_parse_if_statement(f,tokens,num_tokens,token_offset);
	}else if(token_matches_chars(cur_tok,"goto")){
		token_offset++;
		compiler_token* cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
		if(!cur_tok){
			f->errorcode=ORCUS_COMPERR_NO_MORE_TOKENS;
			f->token_error_num=token_offset;
			return 0;
		}else if(!(cur_tok->token_flags & TOKEN_ENV_VAR) ){
			f->errorcode=ORCUS_COMPERR_NO_NAMED_VARIABLE;
			f->token_error_num=token_offset;
			return 0;
		}
		add_instruction(f,ORCUS_OPCODE_JMPR, token_offset);
		if(!compiler_add_goto_jump(f,cur_tok,f->number_instructions)){
			f->errorcode=ORCUS_COMPERR_MEMORY_ALLOCATION;
			f->token_error_num=token_offset;
			return 0;
		}
		add_rawdata_bitwidth_size(f,0, token_offset);
		
		token_offset++;
	}else{
		return 0;
	}
	
	return token_offset-initial_offset;
}




orcus_int orcus_compile_tokens_to_bytecode(compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset, compiler_function* out){
	orcus_int initial_offset=token_offset;
	
	while(true){
		orcus_int o1=m_compile_read_statment(out,tokens,num_tokens,token_offset);
		if(out->errorcode){
			break;
		}
		token_offset+=o1;
		orcus_int o2=m_compile_read_keyword_block(out,tokens,num_tokens,token_offset);
		if(out->errorcode){
			break;
		}
		token_offset+=o2;
		
		orcus_int o3=0;
		compiler_token* cur_tok = m_compile_get_next_token(tokens,num_tokens,token_offset);
		if(cur_tok && cur_tok->token_flags & TOKEN_LOOP_MODIFIER){
			if(out->in_loop){
				if(token_matches_chars(cur_tok,"continue")){
					add_instruction(out, ORCUS_OPCODE_JMPR, token_offset);
					if (!compiler_add_loop_startjump(out, out->number_instructions)) {
						out->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
						out->token_error_num = token_offset;
						break;
					}
					add_rawdata_bitwidth_size(out, 0, token_offset);
				}else{ //break
					add_instruction(out, ORCUS_OPCODE_JMPR, token_offset);
					if(!compiler_add_loop_endjump(out,out->number_instructions)){
						out->errorcode=ORCUS_COMPERR_MEMORY_ALLOCATION;
						out->token_error_num=token_offset;
						break;
					}
					add_rawdata_bitwidth_size(out,0,token_offset);
				}
				token_offset++;
				o3=1;
			}else{
				out->errorcode=ORCUS_COMPERR_LOOPKEYWORD_WRONG_SPOT;
				out->token_error_num=token_offset;
				break;
			}
		}
		
		if(!o1 && !o2 && !o3){ //exit if no code was generated
			break;
		}

	}
	
	return token_offset - initial_offset;
}


void orcus_compile_mstring(orcus_string* str, orcus_variable* out, bool remove_debug_info){
	out->type = ORCUS_TYPE_NIL;
	out->data.i = 0;
	
	orcus_int num_tokens;
	compiler_token* tokens = orcus_compile_tokenize_mstring(str,&num_tokens);
	compiler_function func;
	memset(&func, 0, sizeof(func));
	func.master = &func;
	orcus_compile_tokens_to_bytecode(tokens, num_tokens, 0, &func);
	compiler_function_finalize_addendums(&func);

	if (func.errorcode) {
		int req_chars;
		char* buffer=nullptr;

		char pbuf[512];

		if (func.token_error_num >= num_tokens) {
#ifdef ORCUS_DEBUG
			printf("DEBUG/error|compiler: error registered at token %zi, but we only have %zi avalible.\n", func.token_error_num, num_tokens);
#endif
			func.token_error_num = num_tokens - 1;
		}
		else if (func.token_error_num < 0) {
#ifdef ORCUS_DEBUG
			printf("DEBUG/error|compiler: error registered at token %zi when the minimum is 0\n", func.token_error_num);
#endif
			func.token_error_num = 0;
		}

		compiler_token* etok = tokens+func.token_error_num;
		char* ts=(char*)malloc(etok->num_chars+1);
		if (!ts)return;
		memcpy(ts, etok->chars, etok->num_chars);
		ts[etok->num_chars]='\0';
#ifdef ORCUS_64BIT
		snprintf(pbuf, 510, "line %lli col %lli at \"%s\": ",etok->line_num,etok->line_col,ts);
#else
		snprintf(pbuf, 510, "line %i col %i at \"%s\": ", etok->line_num, etok->line_col,ts);
#endif
		const char* reason;

		switch (func.errorcode) {	
			case ORCUS_COMPERR_MEMORY_ALLOCATION:
				reason = "failed to allocate memory";
				break;
			case ORCUS_COMPERR_NO_MORE_TOKENS:
				reason = "no more tokens";
				break;
			case ORCUS_COMPERR_MALFORMED_NUMBER:
				reason = "malformed number";
				break;
			case ORCUS_COMPERR_WRONG_SYMBOL:
				reason = "wrong symbol";
				break;
			case ORCUS_COMPERR_NO_OPERATION_FOUND:
				reason = "no operation performed";
				break;
			case ORCUS_COMPERR_WHILE_NEEDS_DO:
				reason = "while requires 'do' keyword";
				break;
			case ORCUS_COMPERR_IF_NEEDS_THEN:
				reason = "if requires 'then' keyword";
				break;
			case ORCUS_COMPERR_LOOPKEYWORD_WRONG_SPOT:
				reason = "invalid location, must be in a loop";
				break;
			case ORCUS_COMPERR_ARRAY_NOT_CLOSED:
				reason = "array was not closed, expected ']'";
				break;
			case ORCUS_COMPERR_TABLE_NOT_CLOSED:
				reason = "table was not closed, expected '}'";
				break;
			case ORCUS_COMPERR_CALL_NOT_CLOSED:
				reason = "function was not closed, expected ')'";
				break;
			case ORCUS_COMPERR_NESTEDSTATMENT_NOT_CLOSED:
				reason = "parenthesis was not closed, expected ')'";
				break;
			case ORCUS_COMPERR_NO_VARIABLE:
				reason = "unable to find variable statment";
				break;
			case ORCUS_COMPERR_NO_NAMED_VARIABLE:
				reason = "unable to find named variable";
				break;
			case ORCUS_COMPERR_REQUIRES_EXPLICIT_ASSIGNMENT:
				reason = "explicit variable assignment required";
				break;
			default:
				reason = "unspecified error.";
		}

		req_chars = snprintf(nullptr, 0, "%s: %s.\n", pbuf, reason) + 1;
		buffer = (char*)malloc(req_chars);
		snprintf(buffer, req_chars, "%s%s", pbuf, reason);

		if (!buffer)return;

		out->type = ORCUS_TYPE_STRING;
		out->data.p = orcus_cstring_to_mstring(buffer,strlen(buffer));
	}
	else {
		orcus_function* nmf = (orcus_function*)malloc(sizeof(orcus_function));
		if (!nmf)return;


		orcus_debug_token* dts=nullptr;

		if (!remove_debug_info) {
			nmf->instruction_dbg_lookup = (orcus_uint*)malloc(sizeof(orcus_uint) * func.number_instructions);
			if (!(nmf->instruction_dbg_lookup))return;
			

			for (orcus_int i = 0; i < func.number_instructions; i++) {
				orcus_int toknum = func.instruction_tokens[i];

				if (toknum >= num_tokens) {
#ifdef ORCUS_DEBUG
					printf("DEBUG/error|compiler: instruction #%zi (%hi/%hx) was assigned by token %zi, but we only have %zi avalible.\n", i, func.instructions[i], func.instructions[i], toknum, num_tokens);
#endif
					toknum = num_tokens - 1;
				}
				nmf->instruction_dbg_lookup[i] = toknum;
			}
			free(func.instruction_tokens);

			nmf->dbg_tokens = (orcus_debug_token*)malloc(sizeof(orcus_debug_token) * num_tokens);
			for (orcus_int i = 0; i < num_tokens; i++) {
				compiler_token* t = tokens+i;

				if (t->token_type == TOKEN_TYPE_STRING) { //because the quote marks are lost in tokenization, we must add them back
					char* ntokcptr = (char*)realloc(t->chars,t->num_chars + 2);
					if (ntokcptr) {
						t->chars = ntokcptr;
						nmf->dbg_tokens[i].chars = t->chars;
						memmove(nmf->dbg_tokens[i].chars + 1, t->chars, t->num_chars);
						nmf->dbg_tokens[i].chars[0] = '\"';
						nmf->dbg_tokens[i].chars[t->num_chars+1] = '\"';
						nmf->dbg_tokens[i].num_chars = t->num_chars + 2;
					}
				}
				else {
					nmf->dbg_tokens[i].chars = t->chars;
					nmf->dbg_tokens[i].num_chars = t->num_chars;
				}	
				nmf->dbg_tokens[i].col = t->line_col;
				nmf->dbg_tokens[i].line = t->line_num;
			}
			nmf->num_dbg_tokens = num_tokens;

		}
		else {
			for (orcus_int i = 0; i < num_tokens; i++) {
				free(tokens[i].chars);
			}
			nmf->dbg_tokens = nullptr;
			nmf->num_dbg_tokens = 0;
			nmf->instruction_dbg_lookup = nullptr;
		}
		
		free(tokens);

		nmf->enviromental = false;
		nmf->references = 0;
		nmf->instructions = func.instructions;
		nmf->numberofinstructions = func.number_instructions;

		out->data.p = nmf;
		out->type = ORCUS_TYPE_FUNCTION;
	}
}
