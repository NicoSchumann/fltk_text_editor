#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <FL/Fl.H>
#include <FL/x.H> // for fl_open_callback
#include <FL/Fl_Group.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/filename.H>

int                changed = 0;
char               filename[FL_PATH_MAX] = "";
char               title[FL_PATH_MAX];
Fl_Text_Buffer   *  textbuf = 0;
int TS = 10;

// Editor window functions and class...
void save_cb();
void saveas_cb();
void find2_cb( Fl_Widget *, void * );
void replall_cb( Fl_Widget *, void * );
void replace2_cb( Fl_Widget *, void * );
void replcan_cb( Fl_Widget *, void * );

class EditorWindow : public Fl_Double_Window
{
public:
    EditorWindow( int w, int h, const char * t );
    ~EditorWindow();

    Fl_Window     *     replace_dlg;
    Fl_Input      *     replace_find;
    Fl_Input      *     replace_with;
    Fl_Button     *     replace_all;
    Fl_Return_Button  * replace_next;
    Fl_Button     *     replace_cancel;

    int         wrap_mode;
    int         line_numbers;

    Fl_Text_Editor   *  editor;
    char               search[256];
};

EditorWindow::EditorWindow( int w, int h, const char * t ) : Fl_Double_Window( w, h, t )
{
    replace_dlg = new Fl_Window( 300, 105, "Replace" );
    replace_find = new Fl_Input( 80, 10, 210, 25, "Find:" );
    replace_find->align( FL_ALIGN_LEFT );

    replace_with = new Fl_Input( 80, 40, 210, 25, "Replace:" );
    replace_with->align( FL_ALIGN_LEFT );

    replace_all = new Fl_Button( 10, 70, 90, 25, "Replace All" );
    replace_all->callback( ( Fl_Callback * )replall_cb, this );

    replace_next = new Fl_Return_Button( 105, 70, 120, 25, "Replace Next" );
    replace_next->callback( ( Fl_Callback * )replace2_cb, this );

    replace_cancel = new Fl_Button( 230, 70, 60, 25, "Cancel" );
    replace_cancel->callback( ( Fl_Callback * )replcan_cb, this );
    replace_dlg->end();
    replace_dlg->set_non_modal();
    editor = 0;
    *search = ( char )0;
    wrap_mode = 0;
    line_numbers = 0;
}

EditorWindow::~EditorWindow()
{
    delete replace_dlg;
}


int check_save( void )
{
    if( !changed )
        return 1;

    int r = fl_choice( "The current file has not been saved.\n"
                       "Would you like to save it now?",
                       "Cancel", "Save", "Don't Save" );

    if( r == 1 )
    {
        save_cb(); // Save the file...
        return !changed;
    }

    return r == 2 ?   1 : 0;
}

int loading = 0;
void load_file( const char * newfile, int ipos )
{
    loading = 1;
    int insert = ( ipos != -1 );
    changed = insert;
    if( !insert )
        strcpy( filename, "" );
    int r;
    if( !insert )
         r = textbuf->loadfile( newfile );
    else
         r = textbuf->insertfile( newfile, ipos );
    changed = changed || textbuf->input_file_was_transcoded;
    if( r )
        fl_alert( "Error reading from file \'%s\':\n%s.", newfile, strerror( errno ) );
    else if( !insert )
        strcpy( filename, newfile );
    loading = 0;
    textbuf->call_modify_callbacks();
}

void save_file( const char * newfile )
{
    if( textbuf->savefile( newfile ) )
        fl_alert( "Error writing to file \'%s\':\n%s.", newfile, strerror( errno ) );
    else
        strcpy( filename, newfile );
    changed = 0;
    textbuf->call_modify_callbacks();
}

void copy_cb( Fl_Widget *, void * v )
{
    EditorWindow * e = ( EditorWindow * )v;
    Fl_Text_Editor::kf_copy( 0, e->editor );
}

void cut_cb( Fl_Widget *, void * v )
{
    EditorWindow * e = ( EditorWindow * )v;
    Fl_Text_Editor::kf_cut( 0, e->editor );
}

void delete_cb( Fl_Widget *, void * )
{
    textbuf->remove_selection();
}

void find_cb( Fl_Widget * w, void * v )
{
    EditorWindow * e = ( EditorWindow * )v;
    const char * val;

    val = fl_input( "Search String:", e->search );
    if( val != NULL )
    {
        // User entered a string - go find it!
        strcpy( e->search, val );
        find2_cb( w, v );
    }
}

void find2_cb( Fl_Widget * w, void * v )
{
    EditorWindow * e = ( EditorWindow * )v;
    if( e->search[0] == '\0' )
    {
        // Search string is blank; get a new one...
        find_cb( w, v );
        return;
    }

    int pos = e->editor->insert_position();
    int found = textbuf->search_forward( pos, e->search, &pos );
    if ( found )
    {
        // Found a match; select and update the position...
        textbuf->select( pos, pos+strlen( e->search ) );
        e->editor->insert_position( pos+strlen( e->search ) );
        e->editor->show_insert_position();
    }
    else
        fl_alert( "No occurrences of \'%s\' found!", e->search );
}

void set_title( Fl_Window * w )
{
    if( filename[0] == '\0' )
        strcpy( title, "Untitled" );
    else
    {
        char * slash;
        slash = strrchr( filename, '/' );
        if ( slash != NULL ) strcpy( title, slash + 1 );
        else strcpy( title, filename );
    }

    if( changed )
         strcat( title, " (modified)" );

    w->label( title );
}

void changed_cb( int, int nInserted, int nDeleted, int, const char *, void * v )
{
    if( ( nInserted || nDeleted ) && !loading )
        changed = 1;
    EditorWindow * w = ( EditorWindow * )v;
    set_title( w );
    if( loading )
        w->editor->show_insert_position();
}

void new_cb( Fl_Widget *, void * )
{
    if ( !check_save() )
        return;

    filename[0] = '\0';
    textbuf->select( 0, textbuf->length() );
    textbuf->remove_selection();
    changed = 0;
    textbuf->call_modify_callbacks();
}

void open_cb( Fl_Widget *, void * )
{
    if( !check_save() )
        return;
    Fl_Native_File_Chooser fnfc;
    fnfc.title( "Open file" );
    fnfc.type( Fl_Native_File_Chooser::BROWSE_FILE );
    if ( fnfc.show() )
        return;
    load_file( fnfc.filename(), -1 );

}

void insert_cb( Fl_Widget *, void * v )
{
    Fl_Native_File_Chooser fnfc;
    fnfc.title( "Insert file" );
    fnfc.type( Fl_Native_File_Chooser::BROWSE_FILE );
    if( fnfc.show() )
        return;
    EditorWindow * w = ( EditorWindow * )v;
    load_file( fnfc.filename(), w->editor->insert_position() );
}

void paste_cb( Fl_Widget *, void * v )
{
    EditorWindow * e = ( EditorWindow * )v;
    Fl_Text_Editor::kf_paste( 0, e->editor );
}

int num_windows = 0;

void close_cb( Fl_Widget *, void * v )
{
    EditorWindow * w = ( EditorWindow * )v;

    if( num_windows == 1 )
    {
        if( !check_save() )
            return;
    }

    w->hide();
    w->editor->buffer( 0 );
    textbuf->remove_modify_callback( changed_cb, w );
    Fl::delete_widget( w );

    num_windows--;
    if( !num_windows )
        exit( 0 );
}

void quit_cb( Fl_Widget *, void * )
{
    if( changed && !check_save() )
        return;

    exit( 0 );
}

void replace_cb( Fl_Widget *, void * v )
{
    EditorWindow * e = ( EditorWindow * )v;
    e->replace_dlg->show();
}

void replace2_cb( Fl_Widget *, void * v )
{
    EditorWindow * e = ( EditorWindow * )v;
    const char * find = e->replace_find->value();
    const char * replace = e->replace_with->value();

    if( find[0] == '\0' )
    {
        // Search string is blank; get a new one...
        e->replace_dlg->show();
        return;
    }

    e->replace_dlg->hide();

    int pos = e->editor->insert_position();
    int found = textbuf->search_forward( pos, find, &pos );

    if( found )
    {
        // Found a match; update the position and replace text...
        textbuf->select( pos, pos+strlen( find ) );
        textbuf->remove_selection();
        textbuf->insert( pos, replace );
        textbuf->select( pos, pos+strlen( replace ) );
        e->editor->insert_position( pos+strlen( replace ) );
        e->editor->show_insert_position();
    }
    else fl_alert( "No occurrences of \'%s\' found!", find );
}

void replall_cb( Fl_Widget *, void * v )
{
    EditorWindow * e = ( EditorWindow * )v;
    const char * find = e->replace_find->value();
    const char * replace = e->replace_with->value();

    find = e->replace_find->value();
    if( find[0] == '\0' )
    {
        // Search string is blank; get a new one...
        e->replace_dlg->show();
        return;
    }

    e->replace_dlg->hide();

    e->editor->insert_position( 0 );
    int times = 0;

    // Loop through the whole string
    for( int found = 1; found; )
    {
        int pos = e->editor->insert_position();
        found = textbuf->search_forward( pos, find, &pos );

        if( found )
        {
            // Found a match; update the position and replace text...
            textbuf->select( pos, pos+strlen( find ) );
            textbuf->remove_selection();
            textbuf->insert( pos, replace );
            e->editor->insert_position( pos+strlen( replace ) );
            e->editor->show_insert_position();
            times++;
        }
    }

    if( times )
        fl_message( "Replaced %d occurrences.", times );
    else
        fl_alert( "No occurrences of \'%s\' found!", find );
}

void replcan_cb( Fl_Widget *, void * v )
{
    EditorWindow * e = ( EditorWindow * )v;
    e->replace_dlg->hide();
}

void save_cb()
{
    if( filename[0] == '\0' )
    {
        // No filename - get one!
        saveas_cb();
        return;
    }
    else
        save_file( filename );
}

void saveas_cb()
{
    Fl_Native_File_Chooser fnfc;
    fnfc.title( "Save File As?" );
    fnfc.type( Fl_Native_File_Chooser::BROWSE_SAVE_FILE );
    if( fnfc.show() )
        return;
    save_file( fnfc.filename() );
}

Fl_Window * new_view();

void view_cb( Fl_Widget *, void * )
{
    Fl_Window * w = new_view();
    w->show();
}

Fl_Menu_Item menuitems[] =
{
    { "&File",              0, 0, 0, FL_SUBMENU },
    { "&New File",        0, ( Fl_Callback * )new_cb },
    { "&Open File...",    FL_COMMAND + 'o', ( Fl_Callback * )open_cb },
    { "&Insert File...",  FL_COMMAND + 'i', ( Fl_Callback * )insert_cb, 0, FL_MENU_DIVIDER },
    { "&Save File",       FL_COMMAND + 's', ( Fl_Callback * )save_cb },
    { "Save File &As...", FL_COMMAND + FL_SHIFT + 's', ( Fl_Callback * )saveas_cb, 0, FL_MENU_DIVIDER },
    {
        "New &View",        FL_ALT
        + 'v', ( Fl_Callback * )view_cb, 0
    },
    { "&Close View",      FL_COMMAND + 'w', ( Fl_Callback * )close_cb, 0, FL_MENU_DIVIDER },
    { "E&xit",            FL_COMMAND + 'q', ( Fl_Callback * )quit_cb, 0 },
    { 0 },

    { "&Edit", 0, 0, 0, FL_SUBMENU },
    { "Cu&t",             FL_COMMAND + 'x', ( Fl_Callback * )cut_cb },
    { "&Copy",            FL_COMMAND + 'c', ( Fl_Callback * )copy_cb },
    { "&Paste",           FL_COMMAND + 'v', ( Fl_Callback * )paste_cb },
    { "&Delete",          0, ( Fl_Callback * )delete_cb },
    { 0 },

    { "&Search", 0, 0, 0, FL_SUBMENU },
    { "&Find...",         FL_COMMAND + 'f', ( Fl_Callback * )find_cb },
    { "F&ind Again",      FL_COMMAND + 'g', find2_cb },
    { "&Replace...",      FL_COMMAND + 'r', replace_cb },
    { "Re&place Again",   FL_COMMAND + 't', replace2_cb },
    { 0 },

    { 0 }
};

Fl_Window * new_view()
{
    EditorWindow * w = new EditorWindow( 660, 400, title );

    w->begin();
    Fl_Menu_Bar * m = new Fl_Menu_Bar( 0, 0, 660, 30 );
    m->copy( menuitems, w );
    w->editor = new Fl_Text_Editor( 0, 30, 660, 370 );
    w->editor->textfont( FL_COURIER );
    w->editor->textsize( TS );
    w->editor->buffer( textbuf );


    w->end();
    w->resizable( w->editor );
    w->size_range( 300, 200 );
    w->callback( ( Fl_Callback * )close_cb, w );

    textbuf->add_modify_callback( changed_cb, w );
    textbuf->call_modify_callbacks();
    num_windows++;
    return w;
}

void cb( const char * fname )
{
    load_file( fname, -1 );
}

int main( int argc, char ** argv )
{
    textbuf = new Fl_Text_Buffer;
    fl_open_callback( cb );
    Fl_Window * window = new_view();
    window->show( 1, argv );
    if( argc > 1 )
        load_file( argv[1], -1 );
    return Fl::run();
}
