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

struct compiler_info {
	orcus_int additional_instructions = 0;

	//for while loops
	bool in_loop = false;
	orcus_int loop_jumppoint_start = 0; //used to track where continue and loop end will bring you to
	orcus_int loop_endpoint_ref_count = 0; //tracks the number of below refs
	orcus_int* loop_endpoint_ref_pos = nullptr; //where each break point is. filled in once a loop is compiled so that break will skip to after the loop ends.

	//for if thens
	bool in_conditional = false;
	orcus_int conditional_jumppoint_init = 0; // for if statments, stores the position JRNI distance of the last conditional check, so that it skips to the start of the next elseif/else block.
	orcus_int conditional_jumptoend_count = 0; //these are placed at the end of every elseif/if block to jump to the end so that other blocks are not called when we already executed one.
	orcus_int* conditional_jumptoend_points = nullptr;
	bool conditional_defaultcase = false;

	//constant strings
	orcus_int num_constants = 0;
	orcus_string* constants = nullptr;

	//goto compiling. this is made difficult because we can have situations where we write goto before the label is defined. because the compiler is single pass, this is made a bit tricky, but not impossible. eg, goto a a:
	orcus_int num_gotos = 0; //how many different goto labels are being used. eg, 2
	orcus_int** goto_positions = nullptr; //where the goto keywords are in instruction space. eg, [1], [55,43,32]
	orcus_int* goto_pos_count = nullptr; // how many gotos are present. eg, 1,3
	bool* goto_defined = nullptr; //if the label is defined yet. eg, 0,1 
	orcus_string* goto_alias = nullptr; //the string of the goto label. eg, "stuff","cancel"
	orcus_int* goto_labelpos = nullptr; //where the label is defined. eg, ?,50

	//multiarg out support
	orcus_int args_out = 1;
};

inline void init_compiler_info(compiler_info* ci) {
	memset(ci, 0, sizeof(compiler_info));
}

inline compiler_info* new_compiler_info(){ //this exists to be used in m_compile_read_keyword_block and orcus_compile_tokens_to_bytecode, and whatnot to dictate keywords and control flow. doesn't need to be propagated to low-end functions like m_compile_read_variable or m_compile_read_statment.
//haha just kidding. that was the plan, but, well... programming. turns out constant statements need that propagation so... oh well. turns out a compiler with any amount of good features needs to do that, i guess. at least goto should be less compilated... right?
	compiler_info* out=(compiler_info*)malloc(sizeof(compiler_info));
	if(!out)return out;

	out->additional_instructions=0;
	
	out->in_loop=false;
	out->loop_jumppoint_start=0;
	out->loop_endpoint_ref_count=0;
	out->loop_endpoint_ref_pos=nullptr;

	out->in_conditional=false;
	out->conditional_jumppoint_init=0;
	out->conditional_jumptoend_count=0;
	out->conditional_jumptoend_points=nullptr;
	out->conditional_defaultcase=false;
	
	out->num_constants=0;
	out->constants=nullptr;
	
	out->num_gotos=0;
	
	out->goto_positions=nullptr;
	out->goto_pos_count=nullptr;
	out->goto_defined=nullptr;
	out->goto_alias=nullptr;
	out->goto_labelpos=nullptr;
	
	out->args_out = 1;

	return out;
}

inline void clear_compiler_info(compiler_info* i) {
	free(i->loop_endpoint_ref_pos);
	free(i->conditional_jumptoend_points);
	free(i->constants);
	for (orcus_int n = 0; n < i->num_gotos; n++) {
		free(i->goto_positions[n]);
	}
	free(i->goto_pos_count);
	free(i->goto_positions);
	free(i->goto_defined);
	free(i->goto_alias);
	free(i->goto_labelpos);
}

inline void delete_compiler_info(compiler_info* i){
	free(i->loop_endpoint_ref_pos);
	free(i->conditional_jumptoend_points);
	free(i->constants);
	for(orcus_int n=0;n<i->num_gotos;n++){
		free(i->goto_positions[n]);
	}
	free(i->goto_pos_count);
	free(i->goto_positions);
	free(i->goto_defined);
	free(i->goto_alias);
	free(i->goto_labelpos);	
	free(i);
}

inline void propogate_compiler_info(compiler_info* i_upstream, compiler_info* i_downstream) {
	i_upstream->constants = i_downstream->constants;
	i_upstream->num_constants = i_downstream->num_constants;
	i_upstream->num_gotos = i_downstream->num_gotos;
	i_upstream->goto_positions = i_downstream->goto_positions;
	i_upstream->goto_pos_count = i_downstream->goto_pos_count;
	i_upstream->goto_defined = i_downstream->goto_defined;
	i_upstream->goto_alias = i_downstream->goto_alias;
	i_upstream->goto_labelpos = i_downstream->goto_labelpos;
}

inline bool compiler_info_add_loop_endjump(compiler_info* i,orcus_int instruction_point){
	orcus_int* n=(orcus_int*)realloc(i->loop_endpoint_ref_pos,sizeof(orcus_int)*(i->loop_endpoint_ref_count+1) );
	if(!n)return false;
	n[i->loop_endpoint_ref_count] = instruction_point;
	i->loop_endpoint_ref_pos=n;
	i->loop_endpoint_ref_count++;
	return true;
}

inline bool compiler_info_add_conditional_endjump(compiler_info* i,orcus_int instruction_point){
	orcus_int* n=(orcus_int*)realloc(i->conditional_jumptoend_points,sizeof(orcus_int)*(i->conditional_jumptoend_count+1) );
	if(!n)return false;
	n[i->conditional_jumptoend_count] = instruction_point;
	i->conditional_jumptoend_points=n;
	i->conditional_jumptoend_count++;
	return true;
}

inline orcus_int compiler_info_add_constant_string_from_token(compiler_info* i,compiler_token* t){
	for(orcus_int n=0;n<i->num_constants;n++){
		orcus_string s=i->constants[n];
		if (s.size!=t->num_chars)continue;
		for(orcus_int c=0;c<s.size;c++){
			if(s.ptr[c]!=t->chars[c])goto exit;
		}
		return n;
		exit:;
	}
	
	orcus_string* nptr= (orcus_string*)realloc(i->constants,sizeof(orcus_string)*(i->num_constants+1) );
	if(!nptr)return -1; //surely nobody will use 4294967295 / 18446744073709551615 constants in their code, right?
	i->constants=nptr;
	orcus_string* nstr=i->constants+i->num_constants;
	nstr->size = t->num_chars;
	nstr->ptr = t->chars;
	i->num_constants++;
	return i->num_constants-1;
}

inline bool compiler_info_add_goto_jump(compiler_info* i,compiler_token* t,orcus_int instruction_point){
	void* nptr;
	
	for(orcus_int n=0;n<i->num_gotos;n++){
		orcus_string s=i->goto_alias[n];
		if(s.size!=t->num_chars)continue;
		for(orcus_int c=0;c<t->num_chars;c++){
			if(t->chars[c]!=s.ptr[c])goto next;
		}
		
		nptr=realloc(i->goto_positions[n],sizeof(orcus_int)*(i->goto_pos_count[n]+1) );
		if(!nptr)return false;
		i->goto_positions[n] = (orcus_int*)nptr;
		i->goto_positions[n][i->goto_pos_count[n]]=instruction_point;
		i->goto_pos_count[n]++;
		return true;
		
		next:;
	}
	
	nptr=realloc(i->goto_positions, sizeof(orcus_int*)*(i->num_gotos+1));
	if(!nptr)return false;
	i->goto_positions=(orcus_int**)nptr;

	nptr=realloc(i->goto_pos_count, sizeof(orcus_int)*(i->num_gotos+1));
	if(!nptr)return false;
	i->goto_pos_count=(orcus_int*)nptr;	
	
	nptr=realloc(i->goto_defined, sizeof(bool)*(i->num_gotos+1));
	if(!nptr)return false;
	i->goto_defined=(bool*)nptr;	
	
	nptr=realloc(i->goto_alias, sizeof(orcus_string)*(i->num_gotos+1));
	if(!nptr)return false;
	i->goto_alias=(orcus_string*)nptr;
	
	nptr=realloc(i->goto_labelpos, sizeof(orcus_int)*(i->num_gotos+1));
	if(!nptr)return false;
	i->goto_labelpos=(orcus_int*)nptr;	
	
	nptr=malloc(sizeof(orcus_int));
	if(!nptr)return false;
	i->goto_positions[i->num_gotos]=(orcus_int*)nptr;
	i->goto_positions[i->num_gotos][0] = instruction_point;
	i->goto_pos_count[i->num_gotos]=1;
	i->goto_defined[i->num_gotos]=false;
	orcus_string s;
	s.ptr=t->chars;
	s.size=t->num_chars;
	i->goto_alias[i->num_gotos]=s;
	i->goto_labelpos[i->num_gotos]=0;
	
	i->num_gotos++;
	return true;
}


inline bool compiler_info_add_goto_label(compiler_info* i,compiler_token* t,orcus_int instruction_point){
	void* nptr;

	for(orcus_int n=0;n<i->num_gotos;n++){
		orcus_string s=i->goto_alias[n];
		if(s.size!=t->num_chars)continue;
		for(orcus_int c=0;c<t->num_chars;c++){
			if(t->chars[c]!=s.ptr[c])goto next;
		}

		i->goto_defined[n]=true;
		i->goto_labelpos[n]=instruction_point;
		return true;
		
		next:;
	}
	
	nptr=realloc(i->goto_positions, sizeof(orcus_int*)*(i->num_gotos+1));
	if(!nptr)return false;
	i->goto_positions=(orcus_int**)nptr;

	nptr=realloc(i->goto_pos_count, sizeof(orcus_int)*(i->num_gotos+1));
	if(!nptr)return false;
	i->goto_pos_count=(orcus_int*)nptr;	
	
	nptr=realloc(i->goto_defined, sizeof(bool)*(i->num_gotos+1));
	if(!nptr)return false;
	i->goto_defined=(bool*)nptr;	
	
	nptr=realloc(i->goto_alias, sizeof(orcus_string)*(i->num_gotos+1));
	if(!nptr)return false;
	i->goto_alias=(orcus_string*)nptr;
	
	nptr=realloc(i->goto_labelpos, sizeof(orcus_int)*(i->num_gotos+1));
	if(!nptr)return false;
	i->goto_labelpos=(orcus_int*)nptr;
	
	i->goto_positions[i->num_gotos]=nullptr;
	i->goto_pos_count[i->num_gotos]=0;
	i->goto_defined[i->num_gotos]=true;
	orcus_string s;
	s.ptr=t->chars;
	s.size=t->num_chars;
	i->goto_alias[i->num_gotos]=s;
	i->goto_labelpos[i->num_gotos]=instruction_point;
	
	i->num_gotos++;
	return true;
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
	if (f->instructions)free(f->instructions);
	if (f->instruction_tokens)free(f->instruction_tokens);
	memset(f, 0, sizeof(compiler_function));
}

inline void delete_compiler_function(compiler_function* f){
	if(f->instructions)free(f->instructions);
	if(f->instruction_tokens)free(f->instruction_tokens);
	free(f);
}




//appends the bytecode of fadd to fbase, and frees fadd.
inline bool merge_compiler_functions(compiler_function* fbase, compiler_function* fadd){
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
orcus_int m_compile_read_var_statment(compiler_function* f, compiler_token* tokens, orcus_int num_tokens, orcus_int token_offset, compiler_info* i, compiler_function* postfix_function = nullptr);
compiler_function* orcus_compile_tokens_to_bytecode(compiler_token* tokens, orcus_int num_tokens, orcus_int token_offset = 0, orcus_int* tokens_used = nullptr, compiler_info* ci = nullptr, compiler_function* in=nullptr);


orcus_int m_compile_read_variable(compiler_function* f, compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset,compiler_info* i){
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
		orcus_int ci=compiler_info_add_constant_string_from_token(i,cur_tok);
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
			
			orcus_int oao = i->args_out;
			i->args_out = 1;
			token_offset+= m_compile_read_var_statment(&idxfunc,tokens,num_tokens,token_offset,i);
			i->args_out = oao;
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
					orcus_int oao = i->args_out;
					i->args_out = 1;
					orcus_int o= m_compile_read_var_statment(f,tokens,num_tokens,token_offset,i);
					i->args_out = oao;
					
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
		/*
		orcus_int ci=compiler_info_add_constant_string_from_token(i,cur_tok);
		if (ci==-1){
			f->errorcode=ORCUS_COMPERR_MEMORY_ALLOCATION;
			f->token_error_num=token_offset;
			return 0;
		}
		add_instruction(f,ORCUS_OPCODE_GCON, token_offset);
		add_rawdata_bitwidth_size(f,ci, token_offset);
		//add_string_onto_stack(f,cur_tok->chars,cur_tok->num_chars,token_offset);
		add_instruction(f, g_op,token_offset);
		*/
		
		token_offset++;
	}else if(cur_tok->token_flags & TOKEN_MISC_OP && cur_tok->num_chars==1 && cur_tok->chars[0]=='('){ //parenthesis statement reading. we read (b+c) as a variable because it makes things easier on the compiler, and helps further segment the code to be more clean. after all, a+(b+c) is functionally equivalent to computing (b+c) then doing a+.
		token_offset++;

		orcus_int oao = i->args_out;
		i->args_out = 1;
		token_offset+=m_compile_read_var_statment(f,tokens,num_tokens,token_offset,i);
		i->args_out = oao;
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
		orcus_int toks_consumed=0;
		compiler_function ff;
		memset(&ff, 0, sizeof(ff));
		orcus_compile_tokens_to_bytecode(tokens, num_tokens, token_offset, &toks_consumed, nullptr, &ff);
		if(ff.errorcode){
			f->errorcode=ff.errorcode;
			f->token_error_num=ff.token_error_num;
			return 0;
		}
		token_offset += toks_consumed;
		merge_compiler_functions_nofree(f, &funcargreadf);
		clear_compiler_function(&funcargreadf);
		merge_compiler_functions_nofree(f,&ff);
		clear_compiler_function(&ff);
		add_instruction(f, ORCUS_OPCODE_EXIT, token_offset);
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


orcus_int m_compile_read_var_indexing(compiler_function* f, compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset,compiler_info* i){
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
			orcus_int oao = i->args_out;
			i->args_out = 1;
			orcus_int o=m_compile_read_var_statment(f,tokens,num_tokens,token_offset,i);
			i->args_out = oao;
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



orcus_int m_compile_read_call_func(compiler_function* f, compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset, orcus_int* args_in,compiler_info* i){
	orcus_int initial_offset=token_offset;

	compiler_token* cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
	if(!cur_tok)return 0;
	if( !(cur_tok->token_flags & TOKEN_MISC_OP && cur_tok->num_chars==1 && cur_tok->chars[0]=='(') ){
		return 0;
	}
	token_offset++;

	orcus_int orig_args_out = i->args_out;

	while(true){
		i->args_out = 1;
		cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
		if(!cur_tok){
			f->errorcode=ORCUS_COMPERR_NO_MORE_TOKENS;
			f->token_error_num=token_offset;
			return 0;
		}else if( (cur_tok->token_flags & TOKEN_MISC_OP && cur_tok->num_chars==1 && cur_tok->chars[0]==')') ){
			token_offset++;
			break;
		}
		
		orcus_int o=m_compile_read_var_statment(f,tokens,num_tokens, token_offset,i);
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
	i->args_out= orig_args_out;


	return token_offset-initial_offset;
}

//a var statment is something like 5+5
orcus_int m_compile_read_var_statment(compiler_function* f, compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset,compiler_info* i,compiler_function* postfix_function){
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
	compiler_function func_index;
	compiler_function func_call;
	compiler_function func_unary;
	memset(&func_unary, 0, sizeof(compiler_function));
	compiler_function func_binary;
	memset(&func_binary, 0, sizeof(compiler_function));
	
	token_offset+=m_compile_read_unary_op(&func_unary,tokens,num_tokens,token_offset);
	if(func_unary.errorcode){
		clear_compiler_function(&func_unary);
		f->errorcode=func_unary.errorcode;
		f->token_error_num=func_unary.token_error_num;
		return 0;
	}
	
	token_offset+=m_compile_read_variable(&func_var,tokens,num_tokens,token_offset,i);
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
		memset(&func_index, 0, sizeof(compiler_function));
		memset(&func_call, 0, sizeof(compiler_function));

		orcus_int vitokc = m_compile_read_var_indexing(&func_index, tokens, num_tokens, token_offset, i);
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
		orcus_int fctokc = m_compile_read_call_func(&func_call, tokens, num_tokens, token_offset, &f_args_in, i);
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
			add_rawdata_bitwidth_size(&func_complete, i->args_out, token_offset - 1); //args out
			i->args_out=0;
		}
		else if(vitokc){
			clear_compiler_function(&func_call);
			i->args_out--;
			merge_compiler_functions_nofree(&func_complete, &func_var); clear_compiler_function(&func_var);
			merge_compiler_functions_nofree(&func_complete, &func_index); clear_compiler_function(&func_index);
		}
		else{
			clear_compiler_function(&func_call);
			clear_compiler_function(&func_index);
			clear_compiler_function(&func_complete);
			if(firstpass)i->args_out--;
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
		orcus_int oao = i->args_out;
		i->args_out = 1;
		token_offset+=m_compile_read_var_statment(&func_complete,tokens,num_tokens,token_offset,i,&func_binary);
		i->args_out = oao;
	}else{
		clear_compiler_function(&func_binary);
	}

	merge_compiler_functions_nofree(f, &func_complete);
	clear_compiler_function(&func_complete);
	
	return token_offset-initial_offset;
}


//a statement is something like a=5, a[1]=5, a(5), or a[1](5)
orcus_int m_compile_read_statment(compiler_function* f, compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset,compiler_info* i){
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
			orcus_int oao = i->args_out;
			i->args_out = 1;
			orcus_int o=m_compile_read_var_statment(&n,tokens,num_tokens,token_offset,i);
			i->args_out = oao;
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
			if (!compiler_info_add_goto_label(i, cur_tok, f->number_instructions + i->additional_instructions)) {
				f->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
				f->token_error_num = token_offset;
				return 0;
			}
			return 2;
		}
	}



	i->args_out = 0;
	orcus_opcode* set_instructs = nullptr;
	orcus_opcode get_instruction = ORCUS_OPCODE_GENV;
	orcus_opcode set_instruction = ORCUS_OPCODE_SENV;
	compiler_function* varfuncs = nullptr;
	compiler_function* vargetresidualfuncs = nullptr;
	compiler_function* varsetresidualfuncs = nullptr;
	while (true) {
		void* nptr=realloc(set_instructs, sizeof(orcus_opcode) * (i->args_out + 1));
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

		nptr = realloc(varfuncs, sizeof(compiler_function) * (i->args_out + 1));
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
		memset(varfuncs + i->args_out,0,sizeof(compiler_function));

		nptr = realloc(vargetresidualfuncs, sizeof(compiler_function) * (i->args_out + 1));
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
		memset(vargetresidualfuncs + i->args_out, 0, sizeof(compiler_function));

		nptr = realloc(varsetresidualfuncs, sizeof(compiler_function) * (i->args_out + 1));
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
		memset(varsetresidualfuncs + i->args_out, 0, sizeof(compiler_function));

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
				for (orcus_int n = 0; n < i->args_out; n++) {
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
			for (orcus_int n = 0; n < i->args_out; n++) {
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


		compiler_function* funcvarblock = varfuncs + i->args_out;//new_compiler_function();
		compiler_function* funcvarget = vargetresidualfuncs + i->args_out;//new_compiler_function();
		compiler_function* funcvarset = varsetresidualfuncs + i->args_out;//new_compiler_function();

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
				for (orcus_int n = 0; n <= i->args_out; n++) {
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
					orcus_int oao = i->args_out;
					i->args_out = 1;
					token_offset += m_compile_read_var_statment(funcvarblock, tokens, num_tokens, token_offset, i);
					i->args_out = oao;
					cur_tok = m_compile_get_next_token(tokens, num_tokens, token_offset);
					if (!(cur_tok && cur_tok->token_flags & TOKEN_MISC_OP && cur_tok->num_chars == 1 && cur_tok->chars[0] == ']')) {
						f->token_error_num = token_offset;
						f->errorcode = ORCUS_COMPERR_WRONG_SYMBOL;
						for (orcus_int n = 0; n <= i->args_out; n++) {
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
						for (orcus_int n = 0; n <= i->args_out; n++) {
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
						for (orcus_int n = 0; n <= i->args_out; n++) {
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
				memset(&prepend + i->args_out, 0, sizeof(compiler_function));

				token_offset += m_compile_read_call_func(&prepend, tokens, num_tokens, token_offset, &ain, i); //append to f because function args come before the function.
				if (prepend.errorcode) {
					f->errorcode = prepend.errorcode;
					f->token_error_num = prepend.token_error_num;
					for (orcus_int n = 0; n <= i->args_out; n++) {
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
			set_instructs[i->args_out] = set_instruction;
			i->args_out++;
		}
		else if (i->args_out) {
			f->errorcode = ORCUS_COMPERR_REQUIRES_EXPLICIT_ASSIGNMENT;
			f->token_error_num = token_offset;
			return 0;
		}

		cur_tok = m_compile_get_next_token(tokens, num_tokens, token_offset);
		if (!cur_tok) {
			if (i->args_out) {
				f->token_error_num = token_offset;
				f->errorcode = ORCUS_COMPERR_NO_MORE_TOKENS;
				for (orcus_int n = 0; n < i->args_out; n++) {
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
	if(i->args_out){
		cur_tok = m_compile_get_next_token(tokens, num_tokens, token_offset);
		if (!cur_tok) {
			f->token_error_num = token_offset;
			f->errorcode = ORCUS_COMPERR_NO_MORE_TOKENS;
			for (orcus_int n = 0; n < i->args_out; n++) {
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
			for (orcus_int n = 0; n < i->args_out; n++) {
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
			orcus_int total_args_out = i->args_out;
			while (i->args_out) {
				//i->args_out--; //decrement args out in the m_compile_read_var_statment function call.

				compiler_function argfunc;
				memset(&argfunc, 0, sizeof(compiler_function));

				

				orcus_int starting_args_out = i->args_out;

				token_offset += m_compile_read_var_statment(&argfunc, tokens, num_tokens, token_offset, i);
				if (argfunc.errorcode) {
					f->errorcode = argfunc.errorcode;
					f->token_error_num = argfunc.token_error_num;
					for (orcus_int n = 0; n < i->args_out; n++) {
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

				

				orcus_int args_processed = starting_args_out - i->args_out;
				if (args_processed > 1) {
					
					for (orcus_int n = i->args_out; n < starting_args_out; n++) {
						merge_compiler_functions_nofree(&argfunc, varfuncs + total_args_out - n - 1);
						clear_compiler_function(varfuncs + total_args_out - n - 1);
						//add_instruction(&argfunc, set_instructs[n], ito);
						merge_compiler_functions_nofree(&argfunc, varsetresidualfuncs + total_args_out - n - 1);
						clear_compiler_function(varsetresidualfuncs + total_args_out - n - 1);

						clear_compiler_function(vargetresidualfuncs + total_args_out - n - 1);
					}
				}
				else {
					merge_compiler_functions_nofree(&argfunc, varfuncs + total_args_out - i->args_out - 1);
					clear_compiler_function(varfuncs + total_args_out - i->args_out - 1);
					//add_instruction(&argfunc, set_instructs[i->args_out], ito);
					merge_compiler_functions_nofree(&argfunc, varsetresidualfuncs + total_args_out - i->args_out - 1);
					clear_compiler_function(varsetresidualfuncs + total_args_out - i->args_out - 1);

					clear_compiler_function(vargetresidualfuncs + total_args_out - i->args_out - 1);
				}

				cur_tok = m_compile_get_next_token(tokens, num_tokens, token_offset);
				if (i->args_out) { //if we need another arg...
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
						for (orcus_int n = 0; n < i->args_out; n++) {
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
			if (i->args_out>1) { //self-modification can only work on 1 var.
				f->errorcode = ORCUS_COMPERR_REQUIRES_EXPLICIT_ASSIGNMENT;
				f->token_error_num = token_offset;
				for (orcus_int n = 0; n < i->args_out; n++) {
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
				orcus_int oao= i->args_out;
				i->args_out = 1;
				token_offset += m_compile_read_var_statment(f, tokens, num_tokens, token_offset, i);
				i->args_out = oao;
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
			for (orcus_int n = 0; n < i->args_out; n++) {
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



orcus_int m_compile_parse_while_loop(compiler_function* f, compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset,compiler_info* ci){
	orcus_int initial_offset=token_offset;
	compiler_token* cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
	
	compiler_info oci=*ci;
	
	
	if( !(cur_tok&& cur_tok->token_flags & TOKEN_KEYWORD && token_matches_chars(cur_tok,"while")) ){
		return 0;
	}
	token_offset++;
	
	
	ci->in_loop=true;
	ci->loop_jumppoint_start=f->number_instructions;
	ci->loop_endpoint_ref_count=0;
	ci->loop_endpoint_ref_pos=nullptr;
	
	orcus_int oao = ci->args_out;
	ci->args_out = 1;
	token_offset+=m_compile_read_var_statment(f,tokens,num_tokens,token_offset,ci);
	ci->args_out = oao;
	if(f->errorcode)return 0;
	
	add_instruction(f, ORCUS_OPCODE_JRNI, token_offset);
	compiler_info_add_loop_endjump(ci,f->number_instructions);
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
		ci->additional_instructions=oci.additional_instructions+f->number_instructions;
		compiler_function bf;
		memset(&bf, 0, sizeof(bf));
		orcus_compile_tokens_to_bytecode(tokens, num_tokens, token_offset, &consumed, ci, &bf);
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
	add_rawdata_bitwidth_size(f, ci->loop_jumppoint_start-f->number_instructions,token_offset-1);

	for(orcus_int i=0;i<ci->loop_endpoint_ref_count;i++){
		if (ci->loop_endpoint_ref_pos[i] > f->number_instructions) { //todo: actually fix whatever part of the compiler causes the jump to point out of bounds
#ifdef ORCUS_DEBUG
			printf("compiler: a loop requested a jump to point %zi, but there are only %zi instructions. skipping setting of jump position.\n", ci->loop_endpoint_ref_pos[i],f->number_instructions);
#endif
		}
		else {
			*(orcus_int*)(f->instructions + ci->loop_endpoint_ref_pos[i]) = f->number_instructions - ci->loop_endpoint_ref_pos[i];
		}
	}
	free(ci->loop_endpoint_ref_pos);
	

	oci.conditional_defaultcase = ci->conditional_defaultcase;
	oci.conditional_jumppoint_init = ci->conditional_jumppoint_init;
	oci.conditional_jumptoend_count = ci->conditional_jumptoend_count;
	oci.conditional_jumptoend_points= ci->conditional_jumptoend_points;
	propogate_compiler_info(&oci, ci);
	*ci=oci;
	
	return token_offset-initial_offset;
}

orcus_int m_compile_parse_if_statement(compiler_function* f, compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset,compiler_info* ci){
	orcus_int initial_offset=token_offset;
	compiler_token* cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
	
	compiler_info oci=*ci;
	
	
	if( !(cur_tok&& cur_tok->token_flags & TOKEN_KEYWORD && token_matches_chars(cur_tok,"if")) ){
		return 0;
	}
	token_offset++;
	
	ci->in_conditional=true;
	ci->conditional_jumppoint_init=f->number_instructions;
	ci->conditional_jumptoend_count=0;
	ci->conditional_jumptoend_points=nullptr;
	ci->conditional_defaultcase=false;
	
	orcus_int oao = ci->args_out;
	ci->args_out = 1;
	orcus_int o=m_compile_read_var_statment(f,tokens,num_tokens,token_offset,ci);
	ci->args_out = oao;
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
	
	
	add_instruction(f, ORCUS_OPCODE_JRNI, token_offset);
	ci->conditional_jumppoint_init=f->number_instructions;
	add_rawdata_bitwidth_size(f,0,token_offset);
	token_offset++;


	while(true){
		orcus_int consumed=0;
		ci->additional_instructions=oci.additional_instructions+f->number_instructions;
		compiler_function bf;
		memset(&bf, 0, sizeof(bf));
		orcus_compile_tokens_to_bytecode(tokens, num_tokens, token_offset, &consumed, ci,&bf);
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
			compiler_info_add_conditional_endjump(ci,f->number_instructions);
			add_rawdata_bitwidth_size(f,0,token_offset);
			token_offset++;

			*(orcus_int*)(f->instructions + ci->conditional_jumppoint_init) = f->number_instructions - ci->conditional_jumppoint_init;
			
			orcus_int oao = ci->args_out;
			ci->args_out = 1;
			o=m_compile_read_var_statment(f,tokens,num_tokens,token_offset,ci);
			ci->args_out = oao;
			if(f->errorcode){
				return 0;
			}else if(!o){
				f->errorcode=ORCUS_COMPERR_NO_VARIABLE;
				f->token_error_num=token_offset;
				return 0;
			}
			token_offset+=o;
			
			add_instruction(f, ORCUS_OPCODE_JRNI, token_offset);
			ci->conditional_jumppoint_init=f->number_instructions;
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
			compiler_info_add_conditional_endjump(ci, f->number_instructions);
			add_rawdata_bitwidth_size(f, 0, token_offset);

			token_offset++;
			*(orcus_int*)(f->instructions + ci->conditional_jumppoint_init) = f->number_instructions - ci->conditional_jumppoint_init;
			ci->conditional_defaultcase = true;
		}else if(cur_tok->token_flags & TOKEN_KEYWORD && token_matches_chars(cur_tok,"end") ){
			token_offset++;
			
			if(!ci->conditional_defaultcase){ //if we didn't fall through to else, we have to set the jump point of the last if check to here.
				*(orcus_int*)(f->instructions+ci->conditional_jumppoint_init) = f->number_instructions-ci->conditional_jumppoint_init;
			}
			
			for(orcus_int i=0;i<ci->conditional_jumptoend_count;i++){
				*(orcus_int*)(f->instructions+ci->conditional_jumptoend_points[i]) = f->number_instructions - ci->conditional_jumptoend_points[i];
			}
			
			break;
		}else{
			f->errorcode = ORCUS_COMPERR_WRONG_SYMBOL;
			f->token_error_num=token_offset;
			return 0;
		}
		
	}
	
	free(ci->conditional_jumptoend_points);

	//some data we need to propogate back upwards.
	propogate_compiler_info(&oci,ci);
	oci.loop_jumppoint_start = ci->loop_jumppoint_start;
	oci.loop_endpoint_ref_count = ci->loop_endpoint_ref_count;
	oci.loop_endpoint_ref_pos = ci->loop_endpoint_ref_pos;
	*ci=oci;

	return token_offset-initial_offset;
}


orcus_int m_compile_read_keyword_block(compiler_function* f, compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset,compiler_info* ci){
	orcus_int initial_offset=token_offset;
	
	compiler_token* cur_tok=m_compile_get_next_token(tokens,num_tokens,token_offset);
	if(!cur_tok){
		return 0;
	}else if(!(cur_tok->token_flags & TOKEN_KEYWORD) ){
		return 0;
	}
	
	if(token_matches_chars(cur_tok,"while")){
		token_offset+=m_compile_parse_while_loop(f,tokens,num_tokens,token_offset,ci);
	}else if(token_matches_chars(cur_tok,"if")){
		token_offset+=m_compile_parse_if_statement(f,tokens,num_tokens,token_offset,ci);
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
		if(!compiler_info_add_goto_jump(ci,cur_tok,f->number_instructions+ci->additional_instructions)){
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




compiler_function* orcus_compile_tokens_to_bytecode(compiler_token* tokens,orcus_int num_tokens,orcus_int token_offset, orcus_int* tokens_used, compiler_info* ci, compiler_function* in){
	compiler_function* out;
	if (in) {
		out = in;
	}
	else {
		out = new_compiler_function();
	}
	if(!out)return nullptr;
	orcus_int initial_offset=token_offset;
	bool ci_was_defined=false;
	if(!ci){
		ci_was_defined=true;
		ci=new_compiler_info();
	}
	
	while(true){
		orcus_int o1=m_compile_read_statment(out,tokens,num_tokens,token_offset,ci);
		if(out->errorcode){
			break;
		}
		token_offset+=o1;
		orcus_int o2=m_compile_read_keyword_block(out,tokens,num_tokens,token_offset,ci);
		if(out->errorcode){
			break;
		}
		token_offset+=o2;
		
		orcus_int o3=0;
		compiler_token* cur_tok = m_compile_get_next_token(tokens,num_tokens,token_offset);
		if(cur_tok && cur_tok->token_flags & TOKEN_LOOP_MODIFIER){
			if(ci->in_loop){
				if(token_matches_chars(cur_tok,"continue")){
					add_instruction(out, ORCUS_OPCODE_JMPR, token_offset);
					add_rawdata_bitwidth_size(out, ci->loop_jumppoint_start-(ci->additional_instructions+out->number_instructions)  ,token_offset);
				}else{ //break
					add_instruction(out, ORCUS_OPCODE_JMPR, token_offset);
					if(!compiler_info_add_loop_endjump(ci,out->number_instructions+ci->additional_instructions)){
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
	
	if(tokens_used)*tokens_used=token_offset-initial_offset;
	if(ci_was_defined){
		for(orcus_int i=0;i<ci->num_gotos;i++){
			if(ci->goto_defined[i]){
				for(orcus_int n=0;n<ci->goto_pos_count[i];n++){
					orcus_int p = ci->goto_positions[i][n];
					orcus_int* j = (orcus_int*)(out->instructions + p);
					*j=ci->goto_labelpos[i]-p;
				}
			}
		}
		
		orcus_int spaceneeded = 0; //track space needed so we can preallocate it in a big block
		for (orcus_int i = 0; i < ci->num_constants; i++) {
			orcus_string s = ci->constants[i];
			spaceneeded += (s.size + sizeof(orcus_opcode) - 1) / sizeof(orcus_opcode);
			spaceneeded += 2;
			spaceneeded += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE*2;
		}
		void* nptr=realloc(out->instructions, sizeof(orcus_opcode) * (spaceneeded + out->number_instructions));
		if (!nptr) {
			out->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
			return out;
		}
		out->instructions = (orcus_opcode*)nptr;
		memmove(out->instructions + spaceneeded, out->instructions, out->number_instructions * sizeof(orcus_opcode));
		
		nptr = realloc(out->instruction_tokens, sizeof(orcus_int) * (spaceneeded + out->number_instructions));
		if (!nptr) {
			out->errorcode = ORCUS_COMPERR_MEMORY_ALLOCATION;
			return out;
		}
		out->instruction_tokens = (orcus_int*)nptr;
		memmove(out->instruction_tokens + spaceneeded, out->instruction_tokens, out->number_instructions * sizeof(orcus_int));
		memset(out->instruction_tokens,0,sizeof(orcus_int)*spaceneeded); //zero the tokens at the constant string assignment
		orcus_int offset = 0;
		for (orcus_int i = 0; i < ci->num_constants; i++) {
			orcus_string s = ci->constants[i];
			out->instructions[offset] = ORCUS_OPCODE_NSTR;
			offset++;
			*((orcus_int*)(out->instructions + offset)) = s.size;
			offset += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
			memcpy(out->instructions + offset, s.ptr, s.size);
			offset += (s.size + sizeof(orcus_opcode) - 1) / sizeof(orcus_opcode);
			out->instructions[offset] = ORCUS_OPCODE_SCON;
			offset++;
			*((orcus_int*)(out->instructions + offset)) = i;
			offset += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
		}
		out->number_instructions += spaceneeded;
		
		delete_compiler_info(ci);
	}
	return out;
}


void orcus_compile_mstring(orcus_string* str, orcus_variable* out, bool remove_debug_info){
	out->type = ORCUS_TYPE_NIL;
	out->data.i = 0;
	
	orcus_int num_tokens;
	compiler_token* tokens = orcus_compile_tokenize_mstring(str,&num_tokens);
	compiler_function func;
	memset(&func, 0, sizeof(func));
	orcus_compile_tokens_to_bytecode(tokens, num_tokens, 0, nullptr, nullptr, &func);

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
