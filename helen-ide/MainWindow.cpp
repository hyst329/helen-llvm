#include "MainWindow.h"

MainWindow::MainWindow()
{
    set_default_size(600, 500);
    set_title("Helen IDE");
    box_main.set_homogeneous(0);
    box_editor.set_homogeneous(0);
    box_main.pack_end(menubar_main, 0, 0);
    box_main.pack_end(toolbar_main, 0, 0);
    box_main.pack_end(box_editor, 1, 1);
    box_main.pack_end(textview_output, 1, 1);
    box_editor.pack_end(treeview_project, 1, 1);
    box_editor.pack_end(sourceview_editor, 1, 1);
    box_main.pack_end(statusbar_main, 0, 0);
    add(box_main);
}