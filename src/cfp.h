/* cfp - configuration file library
 *
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

#ifndef __CFP_H__
#define __CFP_H__

#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

// if set debug function active
//#define CFP_DEBUG

typedef struct s_cfp_tree_ s_cfp_tree;
typedef struct s_cfp_ s_cfp;
typedef struct s_cfp_heap_ s_cfp_heap;

struct s_cfp_heap_
{
    char  *lval;
    char  *rval;
    s_cfp_heap *next;
};

struct s_cfp_tree_
{
    int  line_nb;
    int  line_count;
    char *lval;
    char *rval;
    s_cfp_tree *branch;
    s_cfp_tree *prev;
    s_cfp_tree *next;
    s_cfp_tree *parent;
};

struct s_cfp_
{
    // tree
    s_cfp_tree *root;
    s_cfp_tree *current;
    s_cfp_heap *heap;
    // buffer
    char *buffer;
    int  allocated;
    int  collected;
    // parser
    int  start_line;
    char *tmp_lval;
    char *tmp_rval;
    int  tmp_rval_pos;
    char old_chr;
    char state;
    char type;
    int  bracket;
    char dqt:1;
    char sqt:1;    
    char skip:1;    
    char qd:1;
};


#ifdef CFP_DEBUG
#define cfp_debug_list( x )	cfp_dbg_list(x)
void cfp_dbg_list( s_cfp *);
#else
#define cfp_debug_list( x )
#endif

#define CMT_DIRECTIVE_TOKENS	{"unknown","include","define","undef","echo","if","else","endif","ifdef","ifndef","elsif"}
#ifndef TR
#define TR(x)	x
#endif

#define CMT_CALLBACK(x)		(cmt_cb)(x)

enum
{
    CMT_DIREC_UNKNOWN = 0,
    CMT_DIREC_INCLUDE,
    CMT_DIREC_DEFINE,
    CMT_DIREC_UNDEF,
    CMT_DIREC_ECHO,
    CMT_DIREC_IF,
    CMT_DIREC_ELSE,
    CMT_DIREC_ENDIF,
    CMT_DIREC_IFDEF,
    CMT_DIREC_IFNDEF,
    CMT_DIREC_ELSIF // not implemented
};

typedef struct s_cmt_ s_cmt;
typedef char (*cmt_cb)(s_cmt *, char chr, void *);
typedef int (*cmt_error_cb)(const char *fmt, ...);

typedef struct cmt_direc_defines cmt_ddf;

struct cmt_direc_defines
{
    char *name;
    char *value;
    cmt_ddf *next;
};

typedef struct cmt_iflist_ cmt_iflist;

struct cmt_iflist_
{
    char state:1;
    char else_occured:1;
    int  line;
    cmt_iflist *prev;
};

struct s_cmt_
{
// file
    const char *file_name;    
    cmt_cb	user_cb;
    void 	*ptr;
// directive 
    cmt_iflist	*iflist;
    cmt_ddf	*def_list, *def_list_prev;
    char	*collection;
    int		allocated;
    int		collected;
    int		dir_line;
    int		token_id;
    int		wcol;
    char	*arg;
    char	prev_chr;
    char        dircond:1;
    char	word_id:1;
// commentary 
    char	collector[2]; // buffer for collector
    int		mcmt;	// multiline nesting counter for commentary
    int		rn_brt;	// round bracket nesting counter
    int		sq_brt;	// square bracet nesting counter
    int		an_brt;
    int		cl_brt;	// block nesting counter
    int		line;	// line counter
    int		col;    // character count in line
    char	colflag:1; // if not set collector buffer empty
    char	scmt:1;	// single line comment flag
    char	skip:1; // ignore interpretation character flag
    char	qt:1;	// quotation flag
    char	ap:1;   // '..'
    char	an_f:1; // angle bracket flag
    char	rn_f:1; // parenthese bracket flag    
    char	sq_f:1; // square bracket flag
    char	cl_f:1; // curly bracket flag
    char	eline:1;// empty line internal flag
    char	crst;	// comment reset flag
    char	last;
    // setting flags
    char	clean;	// substitute all white characters ' ' except '\n' by ' '
    // flags identifiers
    char	cmt :1; 	// commentary
    char 	curly:1;	// curly brackets
    char	round:1;	// parentheses
    char	angle:1;	// angle brackets
    char	square:1;	// square brackets
    char	empty:1;	// repetitive empty character
    char	garbage:1;
    char	begin:1;	// line first character
    char	directive:1;
};

extern cmt_error_cb cmt_error; // handle to user error callback, default is printf()

// txthdr - text string attached at beginnig. Attached text is treated as #include. NULL if unused
//int cmt_fload(const char *fname, cmt_cb callback_function, void *user_pointer, const char *txthdr); // return 0 or errno otherwise
// tmp
//char cfp_parser(s_cfp *, s_cmt *, char, s_cfp_tree **, s_cfp_tree *);
// return pointer to element from path, return NULL on fail
//const s_cfp_tree *cfp_tree_get_element(s_cfp *cfp, const char *path);


/*
    -= Create cfp structure, Constructor =-
    Returned value:
	Newly created cfp structure.
*/
s_cfp *cfp_init();

/*
    -= Load configuration file to tree structure =-
    cfp   - cfp structure
    fpath - file path    
    Returned value:
    0 - success, otherwise error (message on cmt_error_cb() )

*/
char cfp_load(s_cfp *cfp, const char *fpath);

/*
    -= Get right value for given path =-
    expr - pointer to not allocated string, holds rvalue after success, Returned value is only for read and cannot be changed.
    path - tree path:
    format: /node0/node1/...../noden/left_value
    node - node_name:occurance where ':occurance' is optional, default is ':0'
    example:
	/foo/bar/colors/potato
	or
	/foo:0/bar/colors:1/potato:2
    Return:
    0 - success, otherwise error (message on cmt_error_cb() )

*/
char cfp_get_expression(s_cfp *cfp, const char *path, const char **expr);

/*
    return count lvalue occurance in branch pointed by path
*/
int cfp_tree_count_element(s_cfp *cfp, const char *path, const char *lvalue);

/*
    -= Get string value from tree =-
    path - as above
    str - pointer to not allocated string, return newly allocated memory with copy of string, otherwise NULL
    Return:
    0 - on success            
*/
char cfp_get_string(s_cfp *cfp, const char *path, char **str);

/*
    -= Return integer value from tree =-
    path and returned value as above
*/
char cfp_get_long(s_cfp *cfp, const char *path, long *val);

/*
    -= Return floating point value from tree =-
    path and returned value as above
*/

char cfp_get_double(s_cfp *cfp, const char *path, double *val);
/*
    -= Return boolean value for keyname =-
    Accepted values returned in 'b':
    0 <- 0, no, _f_, .f., false, off
    1 <- 1, yes, _t_, .t., true, on  
    -1 <- error
    
    Returned values:
    1 - path not found or missing keyname
    2 - keyname value error
*/
char cfp_get_bool(s_cfp *cfp, const char *path, char *b);

/*
    -= Clean memory, destructor =-
*/
void cfp_free( s_cfp *);

/*
    ========================================================================================================
    === MISCELLANEOUS FUNCTION
*/

/*
    -= Return first valid full path to file from colon separated list 'path_list' =-
    path_list - comma separated list of paths eg. "/etc/foo/bar.cfg,~/.bar.cfg,/usr/local/etc/bar.cfg"
    access_how - The access_how argument either can be the bitwise OR of the flags R_OK, W_OK, X_OK, or the existence test F_OK. 
    Returned value:
    newly allocated first matched path string or NULL, have to be freed if not used
*/
char *cfp_path_selector(const char *path_list, int access_how);

/*
    ========================================================================================================
    === HEAP FUNCTION
*/

/*
    -= Throw variable on heap =-
    If variable exist value is substituded.
*/
char cfp_heap_set(s_cfp *cfp, const char *var_name, const char *value);

/*
    -= Unfold variables =-
    $  - config file variable (previously set by cfp_heap_set() )
    $$ - enviroment variable eg $$HOME    
*/
void cfp_heap_unfold( s_cfp *cfp );

/*

*/
const char *cfp_heap_get(s_cfp *cfp, const char *var_name);

#ifdef __cplusplus
}
#endif

#endif // __CFP_H__

/*
    -= CFP FILE FORMAT =-
    
    1 Preprocessor:
    - Commentary the same as in 'C++'
	Commentary can be nested.
    - included file:
	#include "valid_file_path"
    - definition - only simple substitutions (no arguments )
	#define foo	bar 
	#define a	aa bb
	#define b	aa bb\
	cc\
	dd
	#undef foo
    - output text to cmt_error_cb()
	#echo bla bla bla
    - conditionary 
	#ifdef foo
	...
	#endif
	
	#ifndef foo
	...
	#endif
	
	#if a arg b
	...
	#endif
        Accept #else for all #if directives

    2 Parser:
    Each line has following format:
	left_value = right_value;
	    where ';' terminate line and is mandatory.
    'left_value' string is case sensitive. 'right_value' for boolean are case insensitive (eg. 'Yes' is the same as 'yes', 'Yes' or 'YES').
    right values in quotations ".." or '..' are strings. String can be only one in line. Strings are interpreted as the whole word in preprocessor.
    right_value begin from '{' is begin of section. Each section must be terminated by '};'. Sections can contain other sections.
    There is allowed to have multi left_values with the same name. In that case proper path to 'right_value' is determined by occurance argument in cfp_get functions.
    If occurance argument is ommited, 0 is default so the first matched path will be choosed.     
    
*/

