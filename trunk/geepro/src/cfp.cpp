/* cfp -> configuration library
 * Copyright (C) 2013 Krzysztof Komarnicki
 * Email: krzkomar@wp.pl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See the file COPYING. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include "cfp.h"

#define DPR_BUFFER_INCREMENT	16
#define MEMORY_ALLOC_INCREMENT	16

cmt_error_cb cmt_error = printf;

static const char *cmt_directive_token_table[] = CMT_DIRECTIVE_TOKENS;
static void cmt_direc_execute(s_cmt *cmt, int token, char *arg);
static void cmt_define_free(s_cmt *cmt);
static void cmt_direc_ifclean(s_cmt *cmt);
static char cmt_direc_cond(s_cmt *cmt, char *expr);
static void cmt_direc_subst(s_cmt *cmt, int start, char *name, char *val);
static char cmt_direc_allocate(s_cmt *cmt, int collected);
static void cmt_direc_process(s_cmt *cmt, int line);
static void cfp_heap_free(s_cfp *cfp);

char cmt_direc_ifstate(s_cmt *cmt)
{
    if( !cmt->iflist ) return 1;
    return cmt->iflist->state;
}

static char cmt_direc_put(s_cmt *cmt, char x, void *ptr)
{
    return cmt->user_cb(cmt, x, ptr );    
}

static char cmt_word_process(s_cmt *cmt, void *ptr)
{ 
    cmt_ddf *i;
    char *x;

    if(!cmt->collection ) return 0;

    for(i = cmt->def_list; i; i = i->next)
	if( !strcmp( cmt->collection, i->name)) break;

    x = cmt->collection;
    if( i ) x = i->value;

    for(;*x; x++) 
	if(cmt_direc_put(cmt, *x, ptr)) return 1;
	
    cmt->wcol = 0;
    return 0;
}

static char cmt_word_collect(s_cmt *cmt, char x)
{
    if(cmt_direc_allocate( cmt, cmt->wcol )) return -1;
    cmt->collection[cmt->wcol++ ] = x;
    return 0;
}

static char cmt_direc_txt_subst(s_cmt *cmt, char x, void *ptr)
{
    if(cmt->qt || cmt->ap){
	if(cmt->word_id){
	    cmt->word_id = 0;
	    if(cmt_word_collect(cmt, 0)) return -1;
	    if(cmt_word_process(cmt, ptr)) return -1;
	    if(cmt_direc_put(cmt, x, ptr)) return -1;
	    cmt->prev_chr = x;
	    return 0;    	    
	}    
	cmt->word_id = 0;
	if(cmt_direc_put(cmt, x, ptr)) return -1;
	cmt->prev_chr = x;
	return 0;	
    }
    if(!cmt->word_id){
	if( (( x >= 'a' && x <= 'z') || ( x >= 'A' && x <= 'Z')) && ( cmt->prev_chr <= ' ')){
	    cmt->word_id = 1;
	    cmt->wcol = 0;
	    if(cmt_word_collect(cmt, x)) return -1;
	} else
	    if(cmt_direc_put(cmt, x, ptr)) return -1;
	cmt->prev_chr = x;
	return 0;	
    }
    if( !((( x >= 'a' && x <= 'z') || ( x >= 'A' && x <= 'Z') || ((x >= '0') && (x <= '9')) || (x == '_'))) ){
	cmt->word_id = 0;
	if(cmt_word_collect(cmt, 0)) return -1;
	if(cmt_word_process(cmt, ptr)) return -1;
	if(cmt_direc_put(cmt, x, ptr)) return -1;
	cmt->prev_chr = x;
	return 0;    
    }
    if(cmt_word_collect(cmt, x)) return -1;    
    cmt->prev_chr = x;
    return 0;
}


static int cmt_directive_token_lookup(char *token)
{
    unsigned int i;
    for(i = 0; i < (sizeof( cmt_directive_token_table ) / sizeof( cmt_directive_token_table[0] )); i++)
								if(!strcmp(token, cmt_directive_token_table[i])) return i;    
    return 0;
}

static char cmt_direc_allocate(s_cmt *cmt, int collected)
{
    char *tmp;
    if((collected + 1) >= cmt->allocated ){
	if(!(tmp = (char *)realloc(cmt->collection, collected + DPR_BUFFER_INCREMENT))) return 1;
	cmt->collection = tmp;
	cmt->allocated += DPR_BUFFER_INCREMENT;
    }
    return 0;
}

static inline char cmt_direc_collector(s_cmt *cmt, char x)
{
    int tk;
    char *token, *arg, skip;
    if( cmt->garbage ) return 0;
    if( cmt->directive ){
	if(!cmt->dir_line) cmt->dir_line = cmt->line;
	if(cmt_direc_allocate( cmt, cmt->collected )) return -1;
	cmt->collection[ cmt->collected ] = x;
	cmt->collected++;    
        return 0;
    }
    if(!cmt->collected) return 0;
    if(cmt_direc_allocate( cmt, cmt->collected )) return -1;
    cmt->collection[ cmt->collected ] = 0;
    if( !cmt->collection ) return 0;
    token = cmt->collection;
    if( *token != '#' ) return -2;
    token++;
    for( ;*token && (*token <= ' '); token++); // begin of first word
    arg = token;
    for(skip = 0; *arg && ((*arg > ' ') || skip); arg++) 
					    skip = (*arg == '\\') ? 1 : 0;
    if( *arg && (*arg <= ' ')) *arg++ = 0;            
    if(!(tk = cmt_directive_token_lookup( token ))){
	cmt_error(TR("\n%s:%i: error: Unknown directive '%s'\n"), cmt->file_name, cmt->dir_line, token);
	cmt->collected = 0;
	return -3;
    }
    cmt_direc_execute(cmt, tk, arg);
    cmt->dir_line = 0;
    cmt->collected = 0;
    return 0;
}

void cmt_filter_init( s_cmt *c)
{
    memset( c, 0, sizeof( s_cmt ) );    
    c->line = 1;
}

void cmt_filter(s_cmt *cmt ,char *bf )
{
    char ep;

    cmt->cmt = cmt->scmt || cmt->mcmt;
    if( cmt->crst )
    	    cmt->crst--;
    if( cmt->cl_f ) 
	    cmt->cl_f = 0;
    if( cmt->sq_f ) 
	    cmt->sq_f = 0;
    if( cmt->rn_f ) 
	    cmt->rn_f = 0;
    if( cmt->an_f ) 
	    cmt->an_f = 0;
    if( *bf == '\n' ) cmt->line++;
    if( cmt->skip ){
	cmt->skip = 0;
	return;
    }
    switch( bf[0] ){
	case '\\': if(!cmt->cmt && (cmt->qt || cmt->ap || cmt->directive)) cmt->skip = 1; break;
	case 0:
	case '\n':  cmt->scmt = 0; 
		    cmt->directive = 0;
    		    break;	    	    
	case '/': if( (bf[1] == '/') && !(cmt->cmt || cmt->qt || cmt->ap)){
		    cmt->scmt = 1;
		    cmt->cmt = 1;
		    cmt->crst = 1;
		  }
		  if( bf[1] == '*' && !cmt->qt && !cmt->scmt && !cmt->ap){
		    cmt->mcmt++;
		    cmt->cmt = 1;
		    cmt->crst = 2;

		  }
		  break;
	case '*': if( bf[1] == '/' && cmt->mcmt && !cmt->qt && !cmt->ap){
		    cmt->mcmt--;
		    cmt->cmt = cmt->scmt || cmt->mcmt;
		    cmt->crst = 2;
		  }
		  break;
	case '#':if( !(cmt->cmt || cmt->qt || cmt->ap)){
		    cmt->directive = 1;
		    break;
		 }
    }        
    if(!cmt->cmt){
	if( *bf == '"' )
	    cmt->qt = !cmt->qt;
	if( *bf == '\'' )
	    cmt->ap = !cmt->ap;
	if( !cmt->qt && !cmt->ap ){
	    switch( *bf ){
		case '{': cmt->cl_brt++; break;
	        case '}': cmt->cl_brt--; cmt->cl_f = 1; break;
		case '[': cmt->sq_brt++; break;
		case ']': cmt->sq_brt--; cmt->sq_f = 1;break;
		case '(': cmt->rn_brt++; break;
		case ')': cmt->rn_brt--; cmt->rn_f = 1;break;
		case '<': cmt->an_brt++; break;
    		case '>': cmt->an_brt--; cmt->an_f = 1;break;
	    }
	    cmt->empty = 0;
	    if( (bf[0] <= ' ') && (bf[1] <= ' ') && (bf[0] != '\n')) cmt->empty = 1;
	    if( (*bf < ' ') && (*bf != '\n') && cmt->clean && (*bf != -1)) *bf = ' ';
	    if( !cmt->empty ){
	        if((cmt->last == '\n') && (*bf == '\n')) cmt->empty = 1;
	    }
	    cmt->last = *bf;
	}
    }
    cmt->cmt = cmt->scmt || cmt->crst || cmt->mcmt;
    cmt->curly = cmt->cl_brt || cmt->cl_f;
    cmt->square = cmt->sq_brt || cmt->sq_f;
    cmt->round = cmt->rn_brt || cmt->rn_f;
    cmt->angle = cmt->an_brt || cmt->an_f;
    if(!cmt->empty && !cmt->cmt){
	if( *bf > ' ') cmt->eline = 1;
	    if( *bf == '\n' ){
		cmt->empty |= !cmt->eline;
		cmt->eline = 0;	    
	    }
    }
    cmt->garbage = (cmt->cmt || cmt->empty);

    ep = 0;
    if(*bf == '\n'){
	if( cmt->col ) 
	    cmt->col = 0; 
	else 
	    ep = 1;
    } else 
	if(!cmt->garbage) 
	    cmt->col++;
    if( ep )
	cmt->begin = 1;
    else
	cmt->begin = (cmt->col == 1);
}

static char cmt_out(s_cmt *cmt, char x, cmt_cb cb, void *ptr)
{
    return cmt_direc_txt_subst(cmt, x, ptr) < 0;
}

char cmt_filter_collector(s_cmt *cmt, char chr, cmt_cb cb, void *ptr)
{
    cmt->collector[ 0 ] = cmt->collector[1];
    cmt->collector[ 1 ] = chr;
    if( (chr == 0) && !cmt->colflag ){ // first character in buffer == 0
	cmt->collector[0] = 0;
	cmt->colflag = 1;
	cb(cmt, 0, ptr);
	return 0;
    }
    if( !cmt->colflag ){ // wait for second character
	cmt->colflag = 1;
	return 0;
    }
    cmt_filter(cmt, cmt->collector);
    cmt_direc_collector(cmt, *cmt->collector);
    if(cmt->garbage || cmt->directive || cmt->dircond ) return 0;
    if(!cmt_direc_ifstate( cmt )) return 0;
    if(cmt_out(cmt, *cmt->collector, cb, ptr)) return 1;
    return 0;
}


static int cmt_fload_(s_cmt *cmt, const char *fname, cmt_cb cb, void *ptr, char flag, const char *incl)
{
    int err;
    FILE *f;

    err = 0;
    if(!cmt) return 0;
    if(!(f = fopen(fname, "r"))) return errno;
    if(!flag) cmt_filter_init( cmt );
    cmt->user_cb = cb;
    cmt->ptr = ptr;
    cmt->file_name = fname;
    cmt->clean = 1;

    if( incl ){
	for(;*incl && !err; incl++)
	    err = cmt_filter_collector( cmt, *incl, cb, ptr);
    }

    while( !feof( f ) && !err)
	err = cmt_filter_collector( cmt, fgetc(f), cb, ptr);

    if(!err) err = cmt_filter_collector( cmt, 0, cb, ptr); // terminate stream
    if(!err)
	if(cmt_out(cmt, 0, cb, ptr)) err = 1;
    fclose(f);
    if(cmt->collection) free(cmt->collection);
    cmt_define_free( cmt );
    cmt_direc_ifclean( cmt );
    return err;
}

int cmt_fload(const char *fname, cmt_cb cb, void *ptr, const char *incl)
{
    s_cmt cmt;
    return cmt_fload_(&cmt, fname, cb, ptr,  0, incl);
}

static char *cmt_cut_word(char *value)
{
    for(;*value && (*value > ' '); value++);
    if(*value)	*value++ = 0;
    return value;
}

static char cmt_is_word_chr(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '\\') || (c == '"') || (c == '\'');
}

static char cmt_ncmp(const char *start, const char *stop, const char *tmpl)
{
    int len;
    if( !(tmpl && start && stop) ) return 0;
    for(len = strlen( tmpl ); (start != stop) && (*start == *tmpl); start++, len--, tmpl++);    
    return len == 0;
}


static cmt_ddf *cmt_lookup_define(s_cmt *cmt, const char *key)
{
    cmt_ddf *i;
    
    for(i = cmt->def_list; i; i = i->next){
	if(i->name)
	    if(!strcmp( i->name, key)) return i;
	cmt->def_list_prev = i;
    }
    return NULL;
}

static void cmt_define_add(s_cmt *cmt, char *key, char *val)
{
    cmt_ddf *i;
    cmt_ddf *tmp;

    if(!cmt || !key) return;
    if((tmp = cmt_lookup_define(cmt, key))){ // already exist -> modify
	if(tmp->value) free(tmp->value);
	tmp->value = (char *)malloc( strlen(val) + 1);
	if( tmp->value ) strcpy( tmp->value, val);    
	return;    
    }

    if(!(tmp = (cmt_ddf*) malloc(sizeof(cmt_ddf)))){
	cmt_error(TR("\nerror: malloc() error\n"));
	return;
    }
    tmp->name = (char *)malloc( strlen(key) + 1);
    tmp->value = (char *)malloc( strlen(val) + 1);
    if( tmp->name ) strcpy( tmp->name, key);
    if( tmp->value ) strcpy( tmp->value, val);    
    tmp->next = NULL;
    
    if( !cmt->def_list ){
	cmt->def_list = tmp;
	return;
    }
    
    for( i = cmt->def_list; i->next; i = i->next);
    i->next = tmp;
}

static void cmt_define_free(s_cmt *cmt)
{
    cmt_ddf *i;
    while( cmt->def_list ){
	i = cmt->def_list->next;
	if(cmt->def_list->name) free(cmt->def_list->name);
	if(cmt->def_list->value) free(cmt->def_list->value);	
	free( cmt->def_list );
	cmt->def_list = i;
    }
}

static void cmt_define_delete( s_cmt *cmt, const char *key)
{
    cmt_ddf *tmp, *i;

    if(!cmt || !key) return;
    cmt->def_list_prev = NULL;
    if(!(tmp = cmt_lookup_define(cmt, key))) return; // not found
    i = tmp->next;
    if(tmp->name) free(tmp->name);
    if(tmp->value) free(tmp->value);	
    free( tmp );
    if( cmt->def_list == tmp) cmt->def_list = i;
    if(cmt->def_list_prev) cmt->def_list_prev->next = i;
}

static void cmt_direc_define(s_cmt *cmt, char *name)
{
    char *value;
    // looking for end of word
    value = cmt_cut_word( name );
    // clean empty characters
    for(;*value && (*value <= ' ');value++);
    cmt_define_add(cmt, name, value);
}

static void cmt_direc_undef(s_cmt *cmt, int idx)
{
    char *name;

    if(!(name = (char *)malloc( strlen( cmt->collection + idx) + 1))){
	cmt_error(TR("\nerror: malloc() error\n"));
	return;
    }
    strcpy( name, cmt->collection + idx);
    cmt_cut_word( name );
    cmt_define_delete(cmt, name);    
    free( name );
}

void cmt_direc_include(s_cmt *cmt, char *arg)
{
    char *tmp, skip;
    cmt_cut_word( arg );
    // check if argument is a string
    if(*arg == '"'){
	arg++;
	for(skip = 0, tmp = arg; *tmp && (*tmp != '"' || skip); tmp++) skip = *tmp == '\\';
	if( *tmp ){
	    s_cmt rcmt;
	    *tmp = 0;
	    cmt_filter_init(&rcmt);
	    rcmt.def_list = cmt->def_list;
	    if(cmt_fload_(&rcmt, arg, cmt->user_cb, cmt->ptr, 1, NULL)){
		cmt_error(TR("%s:%i: error: opening file '%s'\n"), cmt->file_name, cmt->dir_line, arg);
		return;
	    }
	    return;
	}
	cmt_error(TR("%s:%i: error: expected \" for terminate string '\"%s'\n"), cmt->file_name, cmt->dir_line, arg);
	return;
    }
    cmt_error(TR("%s:%i: error: expected \" for begin of file name: %s\n"), cmt->file_name, cmt->dir_line, arg);
}

static void cmt_direc_if(s_cmt *cmt, char cond)
{
    cmt_iflist *tmp;
    if(!(tmp = (cmt_iflist *)malloc( sizeof( cmt_iflist ) ))){
	cmt_error(TR("Error: Memory allocation\n"));
	return;
    };
    tmp->state = cond;
    tmp->else_occured = 0;
    tmp->prev = cmt->iflist;
    tmp->line = cmt->dir_line;
    cmt->iflist = tmp;
}

static void cmt_direc_else(s_cmt *cmt)
{
    if(cmt->iflist){
	if(cmt->iflist->else_occured){
	    cmt_error(TR("%s:%i: error: 'else' without 'if' directive\n"), cmt->file_name, cmt->dir_line);	
	    return;
	}
	cmt->iflist->else_occured = 1;
	cmt->iflist->state = !cmt->iflist->state;
	return;
    }
    cmt_error(TR("%s:%i: error: 'else' directive without 'if' directive\n"), cmt->file_name, cmt->dir_line);
}

static void cmt_direc_endif(s_cmt *cmt)
{
    cmt_iflist *tmp;    

    tmp = cmt->iflist;
    if(!tmp){
	cmt_error(TR("%s:%i: error: 'endif' directive without 'if' directive\n"), cmt->file_name, cmt->dir_line);
	return;
    }
    cmt->iflist = tmp->prev;
    free( tmp );
}

static void cmt_direc_ifclean(s_cmt *cmt)
{
    cmt_iflist *tmp;    
    while( cmt->iflist ){
	tmp = cmt->iflist;
	cmt_error(TR("%s:%i: error: missing 'endif' directive for 'if' directive in line %i\n"), cmt->file_name, cmt->dir_line, tmp->line);
	cmt->iflist = tmp->prev;
	free( tmp );
    }
}

static void cmt_direc_execute(s_cmt *cmt, int token, char *arg)
{
    int iarg;
    
    iarg = arg - cmt->collection;
    switch(token){
	case CMT_DIREC_UNKNOWN:break;
	case CMT_DIREC_INCLUDE:if(!cmt_direc_ifstate( cmt )) break; 
			       cmt_direc_process(cmt, iarg ); 
			       cmt_direc_include(cmt, cmt->collection + iarg); 
			       break;
	case CMT_DIREC_DEFINE: if(!cmt_direc_ifstate( cmt )) break; 
                               cmt_direc_undef(cmt, iarg); // undef previous definition if existed, allow to redeclare
			       cmt_direc_process(cmt, iarg ); 
			       cmt_direc_define(cmt, cmt->collection + iarg); 
			       break;
	case CMT_DIREC_UNDEF:  if(!cmt_direc_ifstate( cmt )) break; 
			       cmt_direc_undef(cmt, iarg); 
			       break;
	case CMT_DIREC_ECHO:   if(!cmt_direc_ifstate( cmt )) break; 
			       cmt_direc_process(cmt, iarg ); 
			       cmt_error( "%s", cmt->collection + iarg ); 
			       break;
	case CMT_DIREC_IF:     cmt_direc_process(cmt, iarg );
			       cmt_direc_if( cmt, cmt_direc_cond(cmt, cmt->collection + iarg) ); break;
	case CMT_DIREC_ELSE:   cmt_direc_else( cmt ); break;
	case CMT_DIREC_ENDIF:  cmt_direc_endif(cmt); break;
	case CMT_DIREC_IFDEF:  cmt_cut_word( arg );
			       cmt_direc_process(cmt, iarg );
			       cmt_direc_if( cmt, cmt_lookup_define(cmt, cmt->collection + iarg) ? 1:0); 
			       break;
	case CMT_DIREC_IFNDEF: cmt_cut_word( arg );
			       cmt_direc_process(cmt, iarg );
			       cmt_direc_if( cmt, !cmt_lookup_define(cmt, cmt->collection + iarg) ); 
			       break;
	case CMT_DIREC_ELSIF:  cmt_direc_process(cmt, iarg );	
			       // missing implementation
			       break;
    }
}

static inline void cmt_direc_cond_error(s_cmt *cmt, const char *txt)
{
    cmt_error(TR("%s:%i: error: syntax error. String '%s' not recognized as proper 'if' expression."), cmt->file_name, cmt->dir_line, txt);
}

static char cmt_direc_cond_char(char k)
{
    switch( k ){
	case '=': return 1;
	case '>': return 2;
	case '<': return 3;
	case '!': return 4;
    }
    return 0;
}

static char cmt_direc_cond(s_cmt *cmt, char *expr)
{
    char *tmp, op0, op1, *line;
    int arg0, arg1;

    line = expr;    
    if( !expr ) return 0;
    // get first argument
    tmp = NULL;
    arg0 = strtol( expr, &tmp, 0);
    if( expr == tmp ){
	cmt_direc_cond_error(cmt, line);
	return 0;        	        
    }   
    if( !cmt_direc_cond_char( *tmp ) && (*tmp > ' ')){
	cmt_direc_cond_error(cmt, expr);
	return 0;
    }
    for(expr = tmp; *expr && (*expr <= ' '); expr++);
    if( !*expr ) return arg0; // simple alone number
    // second argument - operator
    if( !(op0 = cmt_direc_cond_char(*expr)) ){
	cmt_direc_cond_error(cmt, expr);
	return 0;        
    }    
    expr++;
    op1 = (*expr == '=');
    if(((op0 == 4)||(op0 == 1)) && !op1){
	cmt_direc_cond_error(cmt, line);
	return 0;        	        
    }
    if( op1 ){
	expr++;
    } else {
	if( (*expr > ' ') && (( *expr < '0') || (*expr > '9'))){
	    cmt_direc_cond_error(cmt, expr);
	    return 0;        	    
	}
    }
    for(; *expr && (*expr <= ' '); expr++);
    tmp = NULL;
    arg1 = strtol( expr, &tmp, 0);    
    if( expr == tmp ){
	cmt_direc_cond_error(cmt, line);
	return 0;        	        
    }   
    if( *tmp > ' '){
	cmt_direc_cond_error(cmt, expr);
	return 0;        	        
    }   

    for(; *tmp && (*tmp <= ' '); tmp++); 
    if( *tmp > ' '){
	cmt_direc_cond_error(cmt, tmp);
	return 0;        	        
    }       

    switch( op0 ){
	case 1: return arg0 == arg1;
	case 2: return op1 ? (arg0 >= arg1) : (arg0 > arg1);
	case 3: return op1 ? (arg0 <= arg1) : (arg0 < arg1);
	case 4: return arg0 != arg1;
    }
    return 0;
}

static void cmt_direc_subst(s_cmt *cmt, int istart, char *name, char *val)
{
    int l, v, n;
    char *tmp;
    
    if( !cmt ) return;
    if( !val || !name || !cmt->collection) return;
    v = strlen( val );
    n = strlen( name );
    l = ((cmt->collected - n + v + 1) >= cmt->allocated) ? ( (cmt->allocated + v - n) + 16 ) : cmt->allocated;

    if(!(tmp = (char *)malloc( l ))){
	cmt_error(TR("error: cmt_direc_subst() -> allocating memory\n"));
	return;
    };
    memcpy( tmp, cmt->collection, cmt->collected );
    memset( tmp + istart, 0, cmt->collected - istart );
    sprintf(tmp + istart, "%s%s", val, cmt->collection + istart + n);
    free(cmt->collection);
    cmt->collection = tmp;
    cmt->collected = istart + n + strlen( cmt->collection + istart + n );
    cmt->allocated = l;
}

static void cmt_direc_process(s_cmt *cmt, int line) // preprocessor
{
    char *tmp, sqt, dqt, skip;
    int start, stop;
    cmt_ddf *i;

    for(stop = start = line; *(cmt->collection + start); start = stop + 1){
	tmp = cmt->collection + start;
	for(;*tmp && (*tmp <= ' '); tmp++, start++);
	stop = start;
	for( sqt = 0, dqt = 0, skip = 0; *tmp && (cmt_is_word_chr(*tmp) || sqt || dqt || skip); tmp++, stop++){
	    if( !skip ){
		if(*tmp == '\\' ) skip = 1;
		if(*tmp == '\'' && !dqt) sqt = !sqt;
		if(*tmp == '"' && !sqt) dqt = !dqt;
	    } else 
	    	skip = 0;
	}
	for(i = cmt->def_list; i; i = i->next){
	    if(i->name){
		if(cmt_ncmp(cmt->collection + start, cmt->collection + stop, i->name)){
		     cmt_direc_subst(cmt, start, i->name, i->value);
		     stop = start + strlen(i->value);
		     tmp = cmt->collection + stop;
		}
	    }
	}
	if(!*tmp) break;
    }    
}

static char cfp_allocate( s_cfp *cfp)
{
    if( cfp->allocated > (cfp->collected + 1) ) return 0;
    cfp->buffer = (char *)realloc( cfp->buffer, cfp->collected + MEMORY_ALLOC_INCREMENT);
    if( !cfp->buffer ){
	cfp->allocated = 0;
	return 1;
    }
    cfp->allocated = cfp->collected + MEMORY_ALLOC_INCREMENT;
    return 0;
}

static char cfp_add_buffer(s_cfp *cfp, s_cmt *cmt, char x)
{
    if( cfp_allocate( cfp ) ) return 1;
    cfp->buffer[cfp->collected++] = x;
    return 0;
}

/* return:
    0 - completed and parsed line
    1 - parsing in progress
    2 - syntax error
    3 - memory allocation error
*/
static char cfp_line_collect(s_cmt *cmt, char x, s_cfp *cfp)
{
    if( cfp->state == 0){ // looking for lvalue
	if( x <= ' ' ){
	    cfp->old_chr = x;
	    cfp->tmp_rval_pos = 0;
	    return 1;
	}
	if( (cfp->old_chr <= ' ') && (x >= 'a') ){
	    cfp->state = 1; 
	    cfp->collected = 0;
	    cfp->start_line = cmt->line;
	    if(cfp_add_buffer( cfp, cmt, x)) return 3;
	    cfp->old_chr = x;	    
	    return 1;
	}
	cmt_error(TR("\n%s:%i: error: syntax error, key name cannot begin from '%c'\n"), cmt->file_name, cmt->line, x);
	cfp->old_chr = x;
	return 2;    
    }

    if( cfp->state == 1){ // gather lvalue
	if(x >= 'a' || (x == '_') || (x >= '0' && x <= '9') || (x == '[') || (x == ']')){
	    if( x == '[' ){
		if( cfp->qd ) return 3;
		cfp->qd = 1;
	    }
	    if( x == ']' ){
		if( !cfp->qd ) return 3;
		cfp->qd = 0;
	    }	    
	    if(cfp_add_buffer( cfp, cmt, x)) return 3;
	    return 1;
	}
	if( cfp->qd ) return 1;
	cfp->state = 2;
	if(cfp_add_buffer( cfp, cmt, 0)) return 3; // terminate lvalue string
    }
    if( cfp->state == 2){ // looking for '='
	if( x <= ' ' ) return 1;
	if( cfp->qd ) return 3;
	if( x != '='){
	    cmt_error(TR("\n%s:%i: error: syntax error, expected '=' instead of '%c'.\n"), cmt->file_name, cmt->line, x);
	    return 2;
	}
	cfp->state = 3;
	cfp->old_chr = 0;
	return 1;
    }
    if( cfp->state == 3){ // looking for rvalue
	if( x <= ' ' ) return 1;	    
	if( (x >= ' ') && (x != ';')){
	    cfp->state = 4;
	    cfp->tmp_rval_pos = cfp->collected;
	    if(cfp_add_buffer( cfp, cmt, x)) return 3;
	    cfp->type = x;
	    switch( x ){
		case '{' : cfp->bracket++; break;
		case '"' : cfp->dqt = 1; break;
		case '\'': cfp->sqt = 1; break;
	    }
	    return 1;
	}
	cmt_error(TR("\n%s:%i: error: syntax error, illegal character '%c' for rvalue.\n"), cmt->file_name, cmt->line, x);
	return 2;	
    }
    if( cfp->state == 4){
	if( !cfp->skip ){
	    switch(x){
		case '{' : if( cfp->dqt || cfp->sqt ) break;
			   if(cfp->bracket) 
			      cfp->bracket++; 
		    	   else {
			      cmt_error(TR("\n%s:%i: error: syntax error, illegal character '{' .\n"), cmt->file_name, cmt->line);
			      return 2;		    		       
                           }
		           break;
		case '}' : if( cfp->dqt || cfp->sqt ) break;
			   cfp->bracket--; 
		           if(cfp->bracket == 0) cfp->state = 5;
				if( cfp->bracket < 0 ){
				cmt_error(TR("\n%s:%i: error: syntax error, '}' without '{'.\n"), cmt->file_name, cmt->line);
				return 2;		    
		    	   }
		           break;
		case '"' : if( cfp->sqt || cfp->bracket ) break;
			   if(cfp->dqt) 
				cfp->state = 5; 
			   else {
				cmt_error(TR("\n%s:%i: error: syntax error, illegal character '\"'.\n"), cmt->file_name, cmt->line);	    
				return 2;		
			   }    
		           break;
		case '\'': if( cfp->dqt  || cfp->bracket ) break;
			   if(cfp->sqt) 
				cfp->state = 5; 
			   else {
				cmt_error(TR("\n%s:%i: error: syntax error, illegal character '\''.\n"), cmt->file_name, cmt->line);	    
				return 2;		    
			   }
	                   break;
	        case '[':  if( cfp->dqt || cfp->sqt  || cfp->bracket ) break;
	    		   if(cfp->qd) return 3;
	    		   cfp->qd = 1;
	    		   break;
	    	case ']':  if( cfp->dqt || cfp->sqt  || cfp->bracket ) break;
	    		   if( !cfp->qd ) return 3;
	    		   cfp->qd = 0;
	    		   break;
	    }
	}
	if( (cfp->bracket == 0) && !cfp->sqt && !cfp->dqt && (x == ';') && !cfp->qd ) cfp->state = 5;
	if( x != ';' || (cfp->bracket != 0) || cfp->dqt || cfp->sqt ){
	    if(cfp_add_buffer( cfp, cmt, x) ) return 3;	
	}
	if( (x == '\\') && (cfp->sqt || cfp->dqt )){
	    cfp->skip = 1;
	    return 1;
	}
	cfp->skip = 0;
	if( (cfp->state != 5) || ( x != ';' )) return 1;
    }
    // state == 5
    if( x <= ' ' ) return 1;
    if( x != ';'){
	cmt_error(TR("\n%s:%i: error: expected ';' instead of '%c'.\n"), cmt->file_name, cmt->line, x);
	return 2;		        
    }    
    if(cfp_add_buffer( cfp, cmt, 0))  return 3;	// terminate string
    cfp->state = 0;
    cfp->old_chr = 0;
    cfp->bracket = 0;
    cfp->sqt = 0;
    cfp->dqt = 0;
    cfp->skip = 0;
    cfp->collected = 0;
    cfp->tmp_rval = cfp->buffer + cfp->tmp_rval_pos;
    cfp->tmp_lval = cfp->buffer;
    return 0;
}

static s_cfp_tree *cfp_add_tree(const char *lval, const char *rval, s_cfp_tree **tree, s_cfp_tree *parent)
{
    s_cfp_tree *tmp, *x;

    if(!(tmp = (s_cfp_tree *)malloc(sizeof(s_cfp_tree)))){
	cmt_error(TR("\nerror: cfp_add_tree() -> malloc().\n"));
	return NULL;	
    }
    memset(tmp, 0, sizeof(s_cfp_tree));
    if( lval ){
	if(!(tmp->lval = (char *)malloc( strlen(lval) + 1 ))){
	    cmt_error(TR("\nerror: cfp_add_tree() -> malloc().\n"));
	    free( tmp );
	    return NULL;	    
	}
	strcpy( tmp->lval, lval);
    }
    if( rval ){
	if(!(tmp->rval = (char *)malloc( strlen(rval) + 1 ))){
	    cmt_error(TR("\nerror: cfp_add_tree() -> malloc().\n"));
	    if(tmp->lval) free( tmp->lval );
	    free( tmp );
	    return NULL;	    
	}
	strcpy( tmp->rval, rval);
    }
    tmp->parent = parent;
    tmp->prev = *tree;
    tmp->branch = NULL;

    if(!*tree){ // first branch element
	*tree = tmp;
	return tmp;    
    }
    // go to last element in branch
    for(x = *tree; x->next; x = x->next);
    tmp->prev = x;
    x->next = tmp;
    return tmp;
}

char cfp_parser(s_cfp *cfp, s_cmt *cmt, char x, s_cfp_tree **tree, s_cfp_tree *parent)
{
    char err;
    char *tmp;
    s_cfp *tmp_cfp;    
    s_cfp_tree *br;

    if((err = cfp_line_collect(cmt, x, cfp))){
	if( err < 2 ) return 0;
        return err; // return true if error occurs
    }
    if( !cfp->buffer ){
	cmt_error(TR("cfp_parser() empty buffer parameter\n"));
	return 1;
    }
    if( cfp->type == '{'){
	cfp->tmp_rval++;
	for( tmp = cfp->tmp_rval; *tmp; tmp++);
	for( ; tmp != (cfp->tmp_rval) && ( *tmp != '}'); tmp--);
	if( *tmp != '}'){
	    cmt_error(TR("\n%s:%i: error: syntax error, expected '}' to end section\n"), cmt->file_name, cfp->start_line);
	    return 1;
	}
	*tmp = 0;
	if(!(tmp_cfp = (s_cfp *) malloc( sizeof(s_cfp) ))){	    
	    cmt_error(TR("\nerror: cfp_add_tree() -> malloc().\n"));
	    return 1;
	}	
	memset( tmp_cfp, 0, sizeof( s_cfp ));
	if( !(br = cfp_add_tree( cfp->tmp_lval, NULL, tree, parent))){
	    cmt_error(TR("cfp_parser() add tree branch error\n"));
	    return 1; // add new branch
	}
	cmt->line--;	
	for( tmp = cfp->tmp_rval; *tmp; tmp++) 
	    if((err = cfp_parser(tmp_cfp, cmt, *tmp, &tmp_cfp->root, br))) break; // NULL temp
	br->branch = tmp_cfp->root; 
	if( tmp_cfp->buffer ) free( tmp_cfp->buffer );	
	free( tmp_cfp);
	if(err) return 1;
    } else {
	if( !cfp_add_tree( cfp->tmp_lval, cfp->tmp_rval, tree, parent)){
	    cmt_error(TR("cfp_parser() add tree item error: '%s' = '%s'\n"), cfp->tmp_lval, cfp->tmp_rval);
	    return 1;
	}
    }
    cfp->tmp_lval = NULL;
    cfp->tmp_rval = NULL;
    cfp->type = 0;
    return 0;
}

static void _cfp_free(s_cfp_tree *t)
{
    s_cfp_tree *next;
    while( t ){
	// free content
	if( t->branch )	_cfp_free( t->branch );
	if( t->lval ) free(t->lval);
	if( t->rval ) free(t->rval);
	// free link
	next = t->next;
	free( t );
	t = next;
    }
}

void cfp_free(s_cfp *cfp)
{
    // free tree
    cfp_heap_free( cfp );
    _cfp_free( cfp->root );
    free( cfp );    
}

static inline char cfp_tree_lval_cmp(s_cfp_tree *t, int len, const char *path)
{
    int k;
    if( !t->lval || !path || (len == 0) || !t ) return 0;
    k = strlen( t->lval ) - len;
    if( k != 0 ) return 0;
    return !strncmp(path, t->lval, len);
}

static const s_cfp_tree *_cfp_tree_get_element(s_cfp_tree *t, const char *path)
{
    const char *pe, *pk;
    int idx, len, index;

    if( !path || !t ) return NULL;
    if( *path != '/') return t; 
    path++;
    pk = pe = path;    
    for( ;*pe && (*pe != ':') && (*pe != '/'); pe++);
    for(pk = pe; *pk && (*pk != '/'); pk++);
    idx = 0;
    if( *pe == ':' ) idx = strtol(pe + 1, NULL, 10);    
    len = pe - path;
    if( len == 0 ) return NULL;
    index = 0;
    for(;t; t= t->next){
	if( cfp_tree_lval_cmp(t, len, path)){
	    if( index == idx){
		if( *pk == '/'){ // follow link
		     if( t->branch ) 
		        return _cfp_tree_get_element(t->branch, pk);
		     else
			return NULL;
		} else return t;
	    }
            index++;	
	}
	if( !t->next) break;
    }
    return NULL;
}


const s_cfp_tree *cfp_tree_get_element(s_cfp *cfp, const char *path)
{
    if( !cfp ) return NULL;
    return _cfp_tree_get_element(cfp->root, path);
}

int cfp_tree_count_element(s_cfp *cfp, const char *path, const char *lvalue)
{
    s_cfp_tree *t;
    int i;
    t = (s_cfp_tree *)cfp_tree_get_element( cfp, path);
    for(i=0;t; t = t->next){
	if( !strcmp(t->lval, lvalue) ) i++;	
    }    
    return i;
}

s_cfp *cfp_init()
{
    s_cfp *cfp;
    
    if(!(cfp = (s_cfp *)malloc( sizeof( s_cfp )))){
    	cmt_error(TR("\nerror: malloc() error\n"));
	return NULL;
    }
    memset( cfp, 0, sizeof( s_cfp ));    
    return cfp;
}

static char cfp_body_cb(s_cmt *cmt, char x, s_cfp *cfp)
{
    return cfp_parser( cfp, cmt, x, &cfp->root, NULL); // NULL temp
}

char cfp_load(s_cfp *cfp, const char *fpath)
{
    char err;
    if( !cfp || !fpath ) return -1;
    err = cmt_fload(fpath, CMT_CALLBACK( cfp_body_cb ), cfp, NULL);
    if( cfp->buffer ) free( cfp->buffer );
    return err;    
}

// return 1 on fail  else 0
char cfp_get_expression(s_cfp *cfp, const char *path, const char **expr)
{
    const s_cfp_tree *e;
    
    *expr = NULL;
    if(!(e = cfp_tree_get_element(cfp, path))) return 1;
    *expr = e->rval;
    return 0;
}

// make copy of string that have to be freed
char cfp_get_string(s_cfp *cfp, const char *path, char **str)
{
    const char *expr = NULL;
    int r;
    char *tmp;
    
    *str = NULL;
    if( cfp_get_expression( cfp, path, &expr) ) return 1;
    if( (*expr != '"') && (*expr != '\'') ) return 2;
    r = strlen( expr );    
    if( expr[ r - 1] != expr[ 0 ] ) return 3;
    if(!(tmp = (char *)malloc( r ))){
    	cmt_error(TR("\nerror: malloc() error\n"));
	return 4;
    }
    strcpy( tmp, expr + 1);
    tmp[ r - 2] = 0;
    *str = tmp;
    return 0;
}

char cfp_get_long(s_cfp *cfp, const char *path, long *val)
{
    const char *expr = NULL;
    char *err = NULL;    

    *val = 0;    
    if( cfp_get_expression( cfp, path, &expr) ) return 1;
    *val = strtol( expr, &err, 0);
    if( err ){
	if( *err != 0 ) return 2;
    }
    return 0;
}

char cfp_get_double(s_cfp *cfp, const char *path, double *val)
{
    const char *expr = NULL;
    char *err = NULL;    

    *val = 0;    
    if( cfp_get_expression( cfp, path, &expr) ) return 1;
    *val = strtod( expr, &err);
    if( err ){
	if( *err != 0 ) return 2;
    }
    return 0;
}

char cfp_get_bool(s_cfp *cfp, const char *path, char *b)
{
    static const char *BTable[] = { 
                             "1","true", "_t_",".t.","yes", "on"
                             "0","false","_f_",".f.","no", "off" 
                           };

    const char *expr = NULL;
    unsigned int i, len, j;

    *b = -1;  
    if( cfp_get_expression( cfp, path, &expr) ) return 1;
    for(i = 0; expr[i] && (expr[i] > ' '); i++);
    len = i;
    for(;expr[i] && (expr[i] <= ' '); i++);
    if( expr[i] ) return 2;

    for(i = 0; i < (sizeof(BTable) / sizeof(BTable[0])); i++){
	j = strlen( BTable[i] );
	if( j != len ) continue;
	for( j = 0; (j < len) && ( BTable[i][j] == tolower( expr[j]) ); j++);
	if( j == len ){
	    *b = i < ((sizeof(BTable) / sizeof(BTable[0])) / 2);
	    return 0;
	}
    }
    return 2;
}

#ifdef CFP_DEBUG
void _cfp_dbg_list(s_cfp_tree *t, int deep)
{
    int x;
    for(;t; t= t->next){
	for(x = deep * 3; x; x--) putchar(' ');
	if( t->branch ){
	    if(t->lval){
		 printf("%s = {\n", t->lval);
		 _cfp_dbg_list( t->branch, deep + 1 );
		 for(x = deep * 3; x; x--) putchar(' ');
		 printf("};\n");
	    }
	} else
	    printf("%s = %s;\n", t->lval, t->rval);
	if( !t->next) break;
    }
}

void cfp_dbg_list(s_cfp *cfp)
{
    _cfp_dbg_list( cfp->root, 0 );
}
#endif

char *cfp_path_selector(const char *path_list, int access_how)
{
    char *tmp, *t, *homedir;
    const char *pos;
    int hlen;
    
    if( !path_list ) return NULL;
    homedir = getenv("HOME");    
    if( homedir ) hlen = strlen( homedir ); else hlen = 0;
    if(!(tmp = (char *)malloc( strlen(path_list) + hlen + 1 ))){ // + reserve space for expansion '~' + '\0'
    	cmt_error(TR("\nerror: malloc() error\n"));
	return NULL;
    }
    for(pos = path_list; *pos; pos++){    
	t = tmp;
	if( *pos == '~' && hlen ){ // expand tilda to home directory path
	    strcpy( tmp, homedir);
	    t += hlen;
	    pos++; // skip '~'	
	}
	for( ; *pos && (*pos != ':'); *t++ = *pos++ );
	*t = 0;
	// check file path
	if(!access( tmp, access_how))  return tmp;
    }    
    free( tmp );
    return NULL;
}

static char cfp_heap_add(s_cfp *cfp, const char *vname, const char *val)
{
    s_cfp_heap *tmp, *i;

    if( !vname || !val || !cfp ) return 1;
    if(!(tmp = (s_cfp_heap *)malloc( sizeof( s_cfp_heap )))){
	cmt_error(TR("\nerror: malloc() error\n"));
	return 2;    
    }
    if(!(tmp->lval = (char *)malloc( strlen(vname) + 1))){
	cmt_error(TR("\nerror: malloc() error\n"));
	free( tmp );
	return 2;
    }
    if(!(tmp->rval = (char *)malloc( strlen(val) + 1))){
	cmt_error(TR("\nerror: malloc() error\n"));
	free( tmp->lval );
	free( tmp );
	return 2;
    }
    tmp->next = NULL;
    strcpy(tmp->lval, vname);
    strcpy(tmp->rval, val);    

    if( !cfp->heap ){
	cfp->heap = tmp;
	return 0;    
    }

    for(i = cfp->heap; i->next; i = i->next);
    i->next = tmp;
        
    return 0;
}

static s_cfp_heap *cfp_heap_looking(s_cfp *cfp, const char *vname, s_cfp_heap *stop)
{
    s_cfp_heap *i;
    
    if( !cfp || !vname ) return NULL;
    for(i = cfp->heap; i && (i != stop); i = i->next){
	if( !i->lval ) continue;
	if( strcmp(i->lval, vname) ) continue;
	return i;
    }
    return NULL;
}

static void cfp_heap_free(s_cfp *cfp)
{
    s_cfp_heap *t;
    
    if( !cfp ) return;
    while( cfp->heap ){
	t = cfp->heap->next;
	if( cfp->heap->lval ) free(cfp->heap->lval);
	if( cfp->heap->rval ) free(cfp->heap->rval);
	free( cfp->heap );
	cfp->heap = t;
    }    
}

static const char *cfp_heap_get_lo(s_cfp *cfp, const char *vname, s_cfp_heap *stop)
{
    s_cfp_heap *t;    
    if(!(t = cfp_heap_looking(cfp, vname, stop))) return NULL;
    return t->rval;
}

const char *cfp_heap_get(s_cfp *cfp, const char *vname)
{
    return cfp_heap_get_lo(cfp, vname, NULL);
}

char cfp_heap_set(s_cfp *cfp, const char *vname, const char *val)
{
    s_cfp_heap *t;    

    if(!cfp || !vname || !val ) return 1;
    t = cfp_heap_looking(cfp, vname, NULL);
    if( !t ) // new variable
	return cfp_heap_add( cfp, vname, val);
    // modify variable
    if( t->rval ) free( t->rval );       
    if(!(t->rval = (char *)malloc( strlen(val) + 1))){
	cmt_error(TR("\nerror: malloc() error\n"));
	return 2;
    }
    strcpy(t->rval, val);
    return 0;
}

static inline char *cfp_heap_resolve(s_cfp *cfp, s_cfp_heap *it)
{
    char *i, sqt, skip, *tmp, *var=NULL, *na, *nc;
    int k, len, pos, varaloc, last_pos, r, taloc;
    
    if( !it || !cfp ) return NULL;
    tmp = NULL; sqt = 0; skip = 0; pos = 0; varaloc = 0; var = NULL; last_pos = 0; taloc = 0; len = 0;
    for( k = 0, i = it->rval; *i; i++, k++){
	if( pos ){
	    if( !skip && !((*i >= '0' && *i <= '9') || (*i >= 'a' && *i <= 'z') || (*i >= 'A' && *i <= 'Z') || (*i == '_') || (*i == '$')) ){
		if( len == 0 ) continue;
		// acquire variable name
		if( varaloc <= len ){
		    if(!(na = (char *)realloc( var, len + 1))){	
			cmt_error(TR("\nerror: malloc() error\n"));
			if( tmp ) free( tmp );
			if( var ) free( var );
			return NULL;
		    }
		    var = na;
		    varaloc = len + 1;
		}
		strncpy( var, it->rval + pos, len);
		var[ len ] = 0;
		// get variable value
		if( var[0] == '$' )
		    na = getenv( var + 1 );
		else
		    na = (char *)cfp_heap_get_lo( cfp, var, it);
		if( !na ) continue;
		// make substitution
		r = 0;
		if( tmp )
		    for(r = 0; tmp[r]; r++);
		r += pos - last_pos; // character count between variables
		r += strlen( na );
		// allocate memory for substitution
		if( taloc <= r ){
		    if(!(nc = (char *)realloc( tmp, r + 1))){	
			cmt_error(TR("\nerror: malloc() error\n"));
			if( tmp ) free( tmp );
			if( var ) free( var );
			return NULL;
		    }
		    if( !tmp ) *nc = 0;
		    tmp = nc;
		    taloc = r + 1;
		}
		strncat(tmp, it->rval + last_pos, pos - last_pos - 1); // -1 because of '$'
		strcat(tmp, na);
		last_pos = k;
		pos = 0;
		len = 0;
		continue;
	    }
	    len++;
	}
	if( skip ){
	    skip = 0;
	    continue;
	}
	if( *i == '\\' ){
	    skip = 1;
	    continue;	
	}
	if( *i == '\'') sqt = !sqt;
	if( (*i == '$') && !pos && !sqt){
	     pos = k + 1;
	     len = 0;
	}
    }
    if( var ) free( var );
    if( tmp ){ // add rest of string
	for(k = 0; tmp[k]; k++);
	r = k + strlen( it->rval + last_pos );
	if( taloc <= r ){
	    if(!(nc = (char *)realloc( tmp, r + 1))){	
		cmt_error(TR("\nerror: malloc() error\n"));
		if( tmp ) free( tmp );
		return NULL;
	    }
	    if( !tmp ) *nc = 0; // string init
	    tmp = nc;
	    taloc = r + 1;
	}	
        strcat(tmp, it->rval + last_pos);

    }
    return tmp;
}

void cfp_heap_unfold( s_cfp *cfp )
{
    s_cfp_heap *i;
    char *x, repeat;
    if( !cfp ) return;
    do {
	repeat = 0;
	for( i = cfp->heap; i; i = i->next ){
	    x = cfp_heap_resolve(cfp, i);
    	    if( x ){
    		if(i->rval) free( i->rval );
    		i->rval = x;
    		repeat = 1;
    	    }	
	}   
    } while( repeat );     
}

