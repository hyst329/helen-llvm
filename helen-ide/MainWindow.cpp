#include "MainWindow.h"

MainWindow::MainWindow()
{
    set_default_size(600, 500);
    set_title("Helen IDE");
    box_main.set_homogeneous(0);
    box_main.set_orientation(ORIENTATION_VERTICAL);
    box_editor.set_homogeneous(0);
    // Adding buttons to toolbar
    toolbuttons[TB_NEW].set_icon_name("document-new");
    toolbuttons[TB_OPEN].set_icon_name("document-open");
    toolbuttons[TB_SAVE].set_icon_name("document-save");
    toolbuttons[TB_SAVE_AS].set_icon_name("document-save-as");
    toolbuttons[TB_BUILD].set_icon_name("system-run");
    toolbuttons[TB_BUILDRUN].set_icon_name("media-seek-forward");
    toolbuttons[TB_RUN].set_icon_name("media-playback-start");
    toolbuttons[TB_STOP].set_icon_name("media-playback-stop");
    toolbuttons[TB_QUIT].set_icon_name("application-exit");
    toolbuttons[TB_NEW].set_tooltip_text("New");
    toolbuttons[TB_OPEN].set_tooltip_text("Open");
    toolbuttons[TB_SAVE].set_tooltip_text("Save");
    toolbuttons[TB_SAVE_AS].set_tooltip_text("Save as...");
    toolbuttons[TB_BUILD].set_tooltip_text("Build");
    toolbuttons[TB_BUILDRUN].set_tooltip_text("Build and run");
    toolbuttons[TB_RUN].set_tooltip_text("Run");
    toolbuttons[TB_STOP].set_tooltip_text("Stop");
    toolbuttons[TB_QUIT].set_tooltip_text("Quit");
    for(ToolButton& b : toolbuttons) toolbar_main.add(b);
    // Packing widgets
    box_editor.pack_end(treeview_project, 1, 1);
    box_editor.pack_end(sourceview_editor, 1, 1);
    box_main.pack_end(menubar_main, 0, 0);
    box_main.pack_end(toolbar_main, 0, 0);
    box_main.pack_end(box_editor, 0, 0);
    box_main.pack_end(textview_output, 1, 1);
    box_main.pack_end(statusbar_main, 0, 0);
    add(box_main);
}
