/* geepro - Willem eprom programmer for linux
 * Copyright (C) 2011 Krzysztof Komarnicki
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

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "../src/cfp.h"
#include "be_stencil.h"
#include "index_stc.h"

static char gui_bineditor_stencil_new(GuiBineditor *be);
static char gui_bineditor_stencil_edit(GuiBineditor *be, const char *fname, const char *device, const char *path);


static char *gui_be_get_path(const char *fpath)
{
    char *tmp;
    int i;
    
    i =  strlen(fpath);
    if(!(tmp = (char *)malloc( i + 1 ))) return NULL;
    strcpy(tmp, fpath);
    i--;
    for( ; fpath[i] && fpath[i] != '/' && i != 0; i--);    
    if( i == 0 ) return NULL;
    tmp[i] = 0;    
    return tmp;
}

static inline char gui_be_stencil_check_ext(const char *name)
{
    char *x;

    if(!(x = strchr((char *)name, '.'))) return 0;
    return !strcmp(x, GUI_BINEDITOR_STENCIL_FILE_EXTENSION);
}

static inline void gui_be_stencil_add_position(GuiBineditor *be, const char *path, const char *f_name, FILE *f)
{    
    char *tree, *name, *desc, *fname;
    s_cfp *cfp;
    
    if(!(fname = (char *) malloc( strlen(path) + strlen(f_name) + 2))){
	ERROR(E_ERR, E_T_MALLOC);
	return;
    }
    sprintf(fname, "%s/%s", path, f_name);
    cfp = cfp_init();
    if(!cfp_load( cfp, fname)) return ;

    tree = cfp_get_val_string( cfp_get( cfp, "/device/tree"));
    desc = cfp_get_val_string( cfp_get( cfp, "/device/description"));
    name = cfp_get_val_string( cfp_get( cfp, "/device/name"));    
    cfp_free( cfp );
    if( !tree ){
	ERROR( E_WRN, "syntax error in file %s: missing key /device/tree ", fname);
	free( fname );
	return;
    }    

    if( !desc ){
	ERROR( E_WRN, "syntax error in file %s: missing key /device/description ", fname);
	free( tree );
	free( fname );
	return;
    }    
    if( !name ){
	ERROR( E_WRN, "syntax error in file %s: missing key /device/name ", fname);
	free( tree );
	free( desc );
	free( fname );
	return;
    }    
    fprintf(f, "%s:%s/$%s:\"%s\"\n", fname, tree, name, desc);    
    free( tree );
    free( desc );
    free( name );
    free( fname );
}

char gui_bineditor_stencil_generate_index_file(GuiBineditor *be, const char *fname)
{
    FILE *f;
    DIR  *d;
    struct dirent *dir;
    char *path;
        
    if(!(f = fopen(fname, "w"))){
	printf("ERROR:gui_bineditor_stencil_generate_index_file() -> creating index file error\n");
	return 0;
    }
    fprintf(f, "#Automaticaly generated - don't edit!\n#file path:menu path:\"description\"\n");
    
    if(!(path = gui_be_get_path( fname ))){
	path = (char *)".";
    }
    
    if(!(d = opendir(path))){
	printf("ERROR:gui_bineditor_stencil_generate_index_file() -> open directory %s\n", path);
	return 0;
    }
    while((dir = readdir(d))){
	if( gui_be_stencil_check_ext( dir->d_name) ) 
		gui_be_stencil_add_position(be, path, dir->d_name, f);
    }
    closedir( d );
    fclose(f);    
    free( path );    
    return 1;
}

extern void gui_bineditor_stencil_update(  GuiBineditor *be );	

static inline char gui_bineditor_stencil_update_all( GuiBineditor *be )
{    
    char x = !gui_bineditor_stencil_generate_index_file(be, "./stencils/stencil.idx"); // !!! path from config !!!!!
    if(!x) gui_bineditor_stencil_update( be );	
    return x;
}

static inline void gui_bineditor_stencil_add_file(GuiBineditor *be, char *fname)
{
    GtkWidget *dlg;
    char *tree, *name, *desc, ch;
    FILE *f1, *f2;
    s_cfp *cfp;
    index_stc_str *idx;
    char *bn;
    char path1[PATH_MAX + 1], path2[PATH_MAX + 1];
    if(!realpath("./stencils", path1)){ // from config !!
	ERROR(E_ERR, "real path 1");
	return;    
    }
    if(!realpath(fname, path2)){
	ERROR(E_ERR, "real path 2");
	return;    
    }
    bn = basename( fname );
    if( strcmp( path1, dirname( path2 )) ){ // if different location then copy to ./stencils
	char tmp[ strlen(path1) + strlen(bn) + 1 ];
        sprintf(tmp, "%s/%s", path1, bn);
	if( !access(tmp, R_OK) != 0 ){
	    char yes;
	    dlg = gtk_message_dialog_new( GTK_WINDOW(be->priv->wmain), 
		GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION,
		GTK_BUTTONS_YES_NO, 
		"File %s already exist in %s. Overwrite ?", bn, path1
	    ); 
	    yes = gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_YES;
	    gtk_widget_destroy(dlg);
	    if( !yes ) return;
	    if(unlink( tmp ) != 0){
		ERROR(E_ERR, "Cannot delete file %s", tmp);
		return;
	    }
	}
	if(!(f1 = fopen(fname, "r"))){
	    ERROR(E_ERR, "open source file error %s\n", fname);
	    return;
	}
	if(!(f2 = fopen(tmp, "w"))){
	    ERROR(E_ERR, "creating destination file error '%s'\n", tmp);
	    fclose(f1);
	    return;
	}
	printf("MSG(0) Copy file '%s' to '%s'\n", bn, path1);
	// slow method, but simple. Good for short files.
	while( !feof(f1) ){
	    ch = fgetc(f1);
	    if(!feof(f1)) fputc(ch, f2);
	}
	fclose( f1 );
	fclose( f2 );
    }        
// get path, name and description from stencil file
    cfp = cfp_init();
    if(!cfp_load( cfp, fname)) return ;
    tree = cfp_get_val_string( cfp_get( cfp, "/device/tree"));
    desc = cfp_get_val_string( cfp_get( cfp, "/device/description"));
    name = cfp_get_val_string( cfp_get( cfp, "/device/name"));    
    cfp_free( cfp );
    if( !tree ){
	ERROR( E_WRN, "syntax error in file %s: missing key /device/tree ", fname);
	return;
    }    

    if( !desc ){
	ERROR( E_WRN, "syntax error in file %s: missing key /device/description ", fname);
	free( tree );
	return;
    }    
    if( !name ){
	ERROR( E_WRN, "syntax error in file %s: missing key /device/name ", fname);
	free( tree );
	free( desc );
	return;
    }    
    // check index file 
    if(!(idx = index_stc_open("./stencils/stencil.idx"))) return; // from config !!!
    if(!index_stc_path_lookup( idx, tree, name)){
	index_stc_add( idx, tree, name, desc, fname );    
    } else {
	char yes;
	dlg = gtk_message_dialog_new( GTK_WINDOW(be->priv->wmain), 
	    GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION,
	    GTK_BUTTONS_YES_NO, 
	    "Path '%s/%s' already indexed. Reindex ?", tree, name
	); 
	yes = gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_YES;
	gtk_widget_destroy(dlg);
	if(yes){
	    if(index_stc_del(idx, tree, name)){
	       index_stc_add( idx, tree, name, desc, fname );
	    } else {
		ERROR(E_ERR, "delete path '%s/%s'", tree, name);
	    }
	}
    }
    index_stc_save( idx );
    index_stc_close( idx );
    free( tree );
    free( desc );
    free( name );
    gui_bineditor_stencil_update( be );	
}

static inline char gui_bineditor_stencil_add(GuiBineditor *be)
{
    GtkWidget *dlg;
    GtkFileFilter *ff;
        
    dlg = gtk_file_chooser_dialog_new("Geepro - add stencil", GTK_WINDOW(be->priv->wmain), GTK_FILE_CHOOSER_ACTION_OPEN, 
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
	    NULL
	  );
    // filter
    ff = gtk_file_filter_new();
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), ff);
    gtk_file_filter_set_name(ff, "*.stc");
    gtk_file_filter_add_pattern(ff, "*.stc");    
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dlg), ff);

    if( gtk_dialog_run( GTK_DIALOG(dlg) ) ){
	char *fname;
	fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
	gui_bineditor_stencil_add_file(be, fname);
	g_free( fname );
    }
    gtk_widget_destroy( dlg );
    return 0;
}

static inline char gui_bineditor_stencil_remove(GuiBineditor *be, const char *device, const char *path)
{
    GtkWidget *dlg;
    index_stc_str *idx;

    if( !be || !device || !path ) return 0;
    if(!(idx = index_stc_open("./stencils/stencil.idx"))) return 0; // from config !!!
    if(index_stc_path_lookup( idx, path, device)){
	if(index_stc_del(idx, path, device)){
	    index_stc_save( idx );
	    if( idx->fnm ){
		dlg = gtk_message_dialog_new( GTK_WINDOW(be->priv->wmain), 
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_YES_NO, 
			"Device removed from list.\nDelete also file '%s' from stencils directory ?", idx->fnm
		  ); // intl
		if(gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_YES){
		    if(unlink( idx->fnm ) != 0){
			ERROR(E_ERR, "Cannot delete file %s", idx->fnm);
		    }
		}
		gtk_widget_destroy(dlg);
	    }
	    index_stc_close( idx );    
	    gui_bineditor_stencil_update( be );	
	    return 0;
	}
    }
    index_stc_close( idx );    
    return 0;
}

char gui_bineditor_stencil_operation(GuiBineditor *be, const char *device, char *path, int operation, char has_child)
{
    switch( operation ){
	case GUI_BE_OPERATION_UPDATE_ALL : return gui_bineditor_stencil_update_all( be );
	case GUI_BE_OPERATION_ADD : return gui_bineditor_stencil_add( be );
	case GUI_BE_OPERATION_REMOVE : return gui_bineditor_stencil_remove( be, device, path );
	case GUI_BE_OPERATION_NEW: return gui_bineditor_stencil_new( be );
	case GUI_BE_OPERATION_EDIT: return gui_bineditor_stencil_edit( be, NULL, device, path );
    }
    return 0;
}

static char gui_bineditor_stencil_new(GuiBineditor *be)
{
    GtkWidget *dlg, *ctx, *tb;
    GtkWidget *w_dev;
    GtkWidget *w_pth;
    GtkWidget *w_fnm;
    const char *dev, *pth, *fnm;
    char acc;
    
    dlg = gtk_dialog_new_with_buttons("Geepro - new stencil", GTK_WINDOW(be->priv->wmain),
	GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
	GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
	NULL
    );
    ctx = gtk_dialog_get_content_area( GTK_DIALOG(dlg) );    
    tb = gtk_table_new(2, 3, FALSE);
    gtk_container_add( GTK_CONTAINER(ctx), tb );
    gtk_table_attach( GTK_TABLE(tb), gtk_label_new("Device name:"), 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach( GTK_TABLE(tb), gtk_label_new("Menu path  :"), 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach( GTK_TABLE(tb), gtk_label_new("File name  :"), 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);    
    w_dev = gtk_entry_new();
    w_pth = gtk_entry_new();
    w_fnm = gtk_entry_new();
    gtk_table_attach( GTK_TABLE(tb), w_dev, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach( GTK_TABLE(tb), w_pth, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach( GTK_TABLE(tb), w_fnm, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show_all( ctx );    
    do{
	acc = gtk_dialog_run( GTK_DIALOG(dlg) ) == GTK_RESPONSE_ACCEPT;
	dev = gtk_entry_get_text( GTK_ENTRY( w_dev ) );
	pth = gtk_entry_get_text( GTK_ENTRY( w_pth ) );
	fnm = gtk_entry_get_text( GTK_ENTRY( w_fnm ) );
    } while( acc && (!*dev || !*pth || !*fnm));
    gtk_widget_destroy( dlg );
    if( !acc ) return 0;
    gui_bineditor_stencil_edit( be, fnm, dev, pth);
    return 0;
}

/*****************************************************************************************************************************************************/

static char gui_bineditor_stencil_edit(GuiBineditor *be, const char *fname, const char *device, const char *path)
{
    GtkWidget *dlg, *ctx;
    char acc;
    
    dlg = gtk_dialog_new_with_buttons("Geepro - new stencil", GTK_WINDOW(be->priv->wmain),
	GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
	GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
	NULL
    );
    ctx = gtk_dialog_get_content_area( GTK_DIALOG(dlg) );        

    acc = gtk_dialog_run( GTK_DIALOG(dlg) ) == GTK_RESPONSE_ACCEPT;
    gtk_widget_destroy( dlg );
    return 0;
}


void gui_bineditor_stencil_sheet(GuiBineditor *be, const char *device, const char *fname)
{
    printf("stencil: %s - %s\n", fname, device);
    
}


