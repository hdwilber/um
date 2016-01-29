#include "groups.h"
#include "users.h"

typedef struct _Manager Manager;

struct _Manager {
  Users *us;
  Groups *gs;
  GtkBuilder *b;
  GtkWidget *main;
  GList *shells;
  GtkTreeModel *shells_model;
};
Manager *manager_new(char *p);
