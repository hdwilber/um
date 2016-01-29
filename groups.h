#include <grp.h>
#include <gtk/gtk.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <memory.h>
#include <string.h>


#define GRP_TV "treeview_groups"
#define GRP_ADD "button_add_group"
#define GRP_DEL "button_del_group"
#define GRP_CON "button_set_group"

typedef struct _Groups Groups;

struct _Groups {
  GtkWidget *tv;
  GtkWidget *add;
  GtkWidget *del;
  GtkWidget *set;
  GtkTreeModel *model;
  GList *list;
};

typedef struct _GroupsItem GroupsItem;
struct _GroupsItem {    
  gid_t gid;            //Identificador de grupo
  gchar * name;         //Nombre del grupo
  struct group *grp;    //LInk a la estructura propia del grupo
  gboolean is_new;     //Auxiliar para diferenciar si el grupo es nuevo
  gboolean in_user;     // Auxiliar para establecer si el usuario seleccionado pertenece a este grupo.
};

Groups *groups_new(GtkBuilder *);
void groups_insert(Groups *, GroupsItem *);
void groups_item_to_string(GroupsItem *g);
