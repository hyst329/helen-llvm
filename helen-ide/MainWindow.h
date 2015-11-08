#include <gtkmm.h>
#include <gtksourceviewmm.h>

using namespace Gtk;

class MainWindow : public Window
{
public:
    MainWindow();
private:
    Box box_main = Box(ORIENTATION_VERTICAL), box_editor;
    MenuBar menubar_main;
    Toolbar toolbar_main;
    Statusbar statusbar_main;
    Gsv::View sourceview_editor;
    TreeView treeview_project;
    TextView textview_output;
};