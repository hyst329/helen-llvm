#include <gtkmm.h>
#include <gtksourceviewmm.h>

using namespace Gtk;

enum ToolButtons
{
    TB_NEW,
    TB_OPEN,
    TB_SAVE,
    TB_SAVE_AS,
    TB_BUILD,
    TB_BUILDRUN,
    TB_RUN,
    TB_STOP,
    TB_QUIT,
    // must be last entry
    TB_COUNT
};

class MainWindow : public Window
{
public:
    MainWindow();
protected:
    Box box_main, box_editor;
    MenuBar menubar_main;
    Toolbar toolbar_main;
    ToolButton toolbuttons[TB_COUNT];
    Statusbar statusbar_main;
    Gsv::View sourceview_editor;
    TreeView treeview_project;
    TextView textview_output;
};