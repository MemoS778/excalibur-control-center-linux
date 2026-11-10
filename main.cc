#include "components/keyboard/keyboard.hh"
#include "components/ui/ui.hh"
#include <cstdlib>
#include <gtk/gtk.h>

int main(int argc, char **argv) {
  KeyboardController kbd;
  if (!std::getenv("ECC_PREVIEW") && !kbd.init())
    return 1;
  GtkApplication *app = gtk_application_new("io.github.MemoS778.ExcaliburRGB",
                                            G_APPLICATION_DEFAULT_FLAGS);
  ExcaliburGUI gui(app, kbd);
  g_signal_connect(app, "activate",
                   G_CALLBACK(+[](GtkApplication *, gpointer data) {
                     static_cast<ExcaliburGUI *>(data)->build();
                   }),
                   &gui);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}
