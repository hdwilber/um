#include <string.h>
#include <gio/gio.h>
#include <libuser/config.h>
#include <libuser/user.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <gtk/gtk.h>
#include "manager.h"


int main (int argc, char *argv[]) {
  Manager *man = NULL;

  gtk_init (&argc, &argv);

  man = manager_new("um.glade");

  gtk_main();

  return 0;
}
