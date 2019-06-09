#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl.H>
#include <FL/Fl_Menu_Bar.H>

#include <cstring>
#include <cerrno>


// forward declarations
//////

void find2_cb( Fl_Widget * w, void * v );
void set_title( Fl_Window * w);
int check_save();
void load_file( char * newfile, int ipos );
void saveas_cb( void );
void save_file( char * newfile );


int loading = 0;

class EditorWindow : public Fl_Double_Window
{
public:
    EditorWindow( int w, int h, const char * t);
    ~EditorWindow();
    
    Fl_Window          * replace_dlg;
    Fl_Input           * replace_find;
    Fl_Input           * replace_with;
    Fl_Button          * replace_all;
    Fl_Return_Button   * replace_next;
    Fl_Button          * replace_cancel;
    
    Fl_Text_Editor     * editor;
    char               search[256];
};

EditorWindow::EditorWindow( int w, int h, const char *t)
:  Fl_Double_Window( w, h, t)
 , replace_dlg(new Fl_Window(300,105,"Replace") )
 , replace_find(new Fl_Input(70,10,200,25,"Find:") )
 , replace_with(new Fl_Input(70,40,200,25,"Replace:") )
 , replace_all(new Fl_Button(10,70,120,25,"Replace All") )
 , replace_next(new Fl_Return_Button(105,70,120,25,"Replace Next") )
 , replace_cancel(new Fl_Button(230,70,60,25, "Cancel") )
{}

EditorWindow::~EditorWindow()
{
    delete replace_cancel;
    delete replace_next;
    delete replace_all;
    delete replace_with;
    delete replace_find;
    delete replace_dlg;
    delete editor;
}

int            changed = 0;
char           filename[256] = "";
Fl_Text_Buffer * textbuf;
char title[256];


// Callbacks
////////////////////////////////

void changed_cb( int, int nInserted, int nDeleted,int, const char*, void* v)
{
    if( (nInserted || nDeleted) && !loading) changed = 1;
    EditorWindow * w = (EditorWindow *) v;
    set_title(w);
    if( loading )  w->editor->show_insert_position();
}

void copy_cb( Fl_Widget *, void * v)
{
    EditorWindow* e = (EditorWindow *)v;
    Fl_Text_Editor::kf_cut(0, e->editor);
}

void cut_cb( Fl_Widget *, void* v)
{
    EditorWindow * e = (EditorWindow *)v;
    Fl_Text_Editor::kf_cut( 0, e->editor );
}

void delete_cb( Fl_Widget *, void * v)
{
    textbuf->remove_selection();
}

void find_cb( Fl_Widget * w, void * v)
{
    EditorWindow * e = (EditorWindow *) v;
    const char * val;
    val = fl_input("Search String:", e->search);
    if( val != nullptr ) {
        // User entered a string - go find it!
        std::strcpy(e->search, val);
        find2_cb( w, v );
    }
}

void find2_cb( Fl_Widget * w, void * v)
{
   EditorWindow * e = (EditorWindow *)v;
   if( e->search[0] == '\0' ) {
       // Search string is blank; get a new one...
       find_cb( w, v );
       return;
   }
   int pos = e->editor->insert_position();
   int found = textbuf->search_forward( pos, e->search, &pos);
   if( found ) {
       // Found a match; select and update the position...
       textbuf->select(pos, pos+strlen(e->search) );
       e->editor->insert_position( pos+strlen(e->search));
       e->editor->show_insert_position();
   } else {
       fl_alert("No occurences of \'%s\' found!", e->search);
   }
}

void new_cb( Fl_Widget *, void *)
{
    if( !check_save() ) return;
    
    filename[0] = '\0';
    textbuf->select( 0, textbuf->length() );
    textbuf->remove_selection();
    changed = 0;
    textbuf->call_modify_callbacks();
}

void open_cb( Fl_Widget *, void * )
{
    if( !check_save() ) return;
    
    char * newfile = fl_file_chooser( "Open File?", "*", filename );
    if( newfile != NULL ) load_file( newfile, -1 );
}

void paste_cb( Fl_Widget *, void * v)
{
    EditorWindow * e = (EditorWindow *)v;
    Fl_Text_Editor::kf_paste( 0, e->editor );
}

void quit_cb( Fl_Widget *, void * )
{
    if( changed && !check_save() )
        return;
        
    exit(0);
}

void replace_cb( Fl_Widget *, void* v )
{
    EditorWindow* e = (EditorWindow *)v;
    e->replace_dlg->show();
}

void replace2_cb( Fl_Widget *, void * v)
{
    EditorWindow * e = (EditorWindow *)v;
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
        textbuf->select( pos, pos + strlen(find) );
        textbuf->remove_selection();
        textbuf->insert( pos, replace );
        textbuf->select( pos, pos+strlen(replace) );
        e->editor->insert_position( pos+strlen(replace) );
        e->editor->show_insert_position();
    }
    else
    {
        fl_alert( "No occurrences of \'%s\' found!", find );
    }
}

void replall_cb( Fl_Widget *, void * v)
{
    EditorWindow * e = (EditorWindow *)v;
    const char * find = e->replace_find->value();
    const char * replace = e->replace_with->value();
    find = e->replace_find->value();
    if( find[0] == '\0' ) {
        // Search string is blank; get a new one...
        e->replace_dlg->show();
        return;
    }
    e->replace_dlg->hide();
    e->editor->insert_position(0);
    int times = 0;
    // Loop through the whole string
    for( int found = 1; found; )
    {
        int pos = e->editor->insert_position();
        found = textbuf->search_forward(pos, find, &pos);
        if( found )
        {
            // Found a match; update the position and replace text...
            textbuf->select(pos, pos+strlen(find));
            textbuf->remove_selection();
            textbuf->insert(pos, replace);
            e->editor->insert_position(pos+strlen(replace));
            e->editor->show_insert_position();
            times++;
        }
    }
    if( times )  fl_message("Replaced %d occurrences.", times);
    else  fl_alert("No occurrences of \'%s\' found!", find);
}    

void replcan_cb( Fl_Widget *, void * v )
{
    EditorWindow * e = (EditorWindow *)v;
    e->replace_dlg->hide();
}

void save_cb( void )
{
    if( filename[0] == '\0')
    {
        // No filename - get one!
        saveas_cb();
        return;
    }
    else save_file( filename );
}

void saveas_cb( void )
{
    char * newfile;
    
    newfile = fl_file_chooser( "Save File As?", "*", filename );
    if( newfile != nullptr ) save_file( newfile );
}

int check_save()
{
    if( !changed ) return 1;
    
    int r = fl_choice( "The chrrent file has not been saved.\n"
                       "Would you like to sae it nwo?",
                       "Cancel", "Save", "Discard"
                     );
    
    if( r == 1 )
    {
        save_cb();  // Save the file...
        return !changed;
    }
    
    return r == 2 ? 1 : 0;
}

void load_file( char * newfile, int ipos )
{
    loading = 1;
    int insert = ( ipos != -1 );
    changed = insert;
    if( !insert ) std::strcpy( filename, "");
    int r;
    if( !insert ) r = textbuf->loadfile( newfile );
    else r = textbuf->insertfile( newfile, ipos );
    if( r ) {
        fl_alert( 
            "Error reading from file \'%s\':\n%s.", newfile
           , strerror(errno)
        );
     } else if( !insert ) std::strcpy(filename, newfile);
    loading = 0;
    textbuf->call_modify_callbacks();
}       

void save_file( char * newfile )
{
    if( textbuf->savefile( newfile ) )
        fl_alert( "Error writing to file \'%s\':\n%s.", newfile, strerror(errno));
    else
        std::strcpy( filename, newfile );
    changed = 0;
    textbuf->call_modify_callbacks();
}

void set_title( Fl_Window * w )
{
    if( filename[0] == '\0') std::strcpy( title, "Untitled" );
    else {
        char * slash;
        slash = strchr( filename, '/' );
    }
    
    if( changed ) strcat( title, " (modified)");
    w->label(title);
}

Fl_Menu_Item menuitems[] = {
  { "&File",              0, 0, 0, FL_SUBMENU },
    { "&New File",        0, (Fl_Callback *)new_cb },
    { "&Open File...",    FL_COMMAND + 'o', (Fl_Callback *)open_cb },
//    { "&Insert File...",  FL_COMMAND + 'i', (Fl_Callback *)insert_cb, 0, FL_MENU_DIVIDER },
    { "&Save File",       FL_COMMAND + 's', (Fl_Callback *)save_cb },
    { "Save File &As...", FL_COMMAND + FL_SHIFT + 's', (Fl_Callback *)saveas_cb, 0, FL_MENU_DIVIDER },
//    { "New &View", FL_ALT + 'v', (Fl_Callback *)view_cb, 0 },
//    { "&Close View", FL_COMMAND + 'w', (Fl_Callback *)close_cb, 0, FL_MENU_DIVIDER },
    { "E&xit", FL_COMMAND + 'q', (Fl_Callback *)quit_cb, 0 },
    { 0 },
  { "&Edit", 0, 0, 0, FL_SUBMENU },
//    { "&Undo",       FL_COMMAND + 'z', (Fl_Callback *)undo_cb, 0, FL_MENU_DIVIDER },
    { "Cu&t",        FL_COMMAND + 'x', (Fl_Callback *)cut_cb },
    { "&Copy",       FL_COMMAND + 'c', (Fl_Callback *)copy_cb },
    { "&Paste",      FL_COMMAND + 'v', (Fl_Callback *)paste_cb },
    { "&Delete",     0, (Fl_Callback *)delete_cb },
    { 0 },
  { "&Search", 0, 0, 0, FL_SUBMENU },
    { "&Find...",       FL_COMMAND + 'f', (Fl_Callback *)find_cb },
    { "F&ind Again",    FL_COMMAND + 'g', find2_cb },
    { "&Replace...",    FL_COMMAND + 'r', replace_cb },
    { "Re&place Again", FL_COMMAND + 't', replace2_cb },
    { 0 },
  { 0 }
};

Fl_Window * new_view()
{
    Fl_Window * window = new EditorWindow( 800, 600, "EditorWindow");
    Fl_Menu_Bar * m = new Fl_Menu_Bar(0,0,640,30);
    Fl_Widget * w = window->editor;
    window->end();
    return window;
}     

int main( int argc, char ** argv)
{
    textbuf = new Fl_Text_Buffer;
    Fl_Window * window = new_view();
    window->show( 1, argv );
    
    if( argc > 1 ) load_file( argv[1], -1 );
    
    return Fl::run();
    
    Fl_Menu_Bar * m = new Fl_Menu_Bar(0,0,640,30);
    m->copy(menuitems);
    
    EditorWindow * w = new EditorWindow( 600, 800, "Editor" );
    w->editor = new Fl_Text_Editor( 0, 30, 600, 770 );
    w->editor->buffer(textbuf);
    
    textbuf->add_modify_callback( changed_cb, w);
    textbuf->call_modify_callbacks();
    
    w->editor->textfont( FL_COURIER );  
}
