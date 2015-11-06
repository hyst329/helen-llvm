#include <gtkmm.h>

int main(int argc, char* argv[])
{
    try {
        Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv, "org.helen.ide");
        Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_file("temp.glade");
        Gtk::Window* w;
        builder->get_widget("applicationwindow_ide", w);

        return app->run(*w);
    }
    catch(Glib::Error e) {
        printf("Error : %s\n", e.what().c_str());
        return 1;
    }
}