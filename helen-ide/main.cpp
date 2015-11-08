#include "MainWindow.h"

int main(int argc, char* argv[])
{
    Glib::RefPtr<Gtk::Application> app =
    Gtk::Application::create(argc, argv,
      "org.helen.ide");

    MainWindow mw;
    mw.show_all();

    return app->run(mw);
}