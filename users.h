#include <grp.h>
#include <gtk/gtk.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <memory.h>
#include <string.h>

#define SHELLS_FILE_PATH "/etc/shells"

typedef struct _Users Users;
struct _Users {
  GtkWidget *tv;
  GtkWidget *add;
  GtkWidget *del;
  GtkWidget *set;
  GtkTreeModel *model;
  GList *list;
  GList *shells;
  GtkTreeModel *shells_model;
};


typedef struct _UsersItem UsersItem; //Definición del tipo de dato
struct _UsersItem {     // Definición de la estructura
  uid_t uid;            //Identificador del usuario. Si es nuevo se establece a 0 
  gid_t gid;            //Identificador del grupo principal para el usuario
  char *name;           //Nombre de usuario  o login
  char *fullname;       //Nombre completo del usuario, primer parámetro de formato gecos
  char *shell;          //Shell
  char * home;          //Directorio de trabajo 
  gboolean is_new;      //Variable para poder diferenciar los nuevos usuarios creados 
  gid_t *groups;        //Lista de los id de grupos auxiliares a los que pertenece
  int n_groups;         //Cantidad de grupos de la lista de grupos auxiliares
};


gboolean check_group(UsersItem *u, gid_t gid);
Users *users_new (GtkBuilder *);
void users_insert(Users *, UsersItem *);
UsersItem * users_get_current (Users *);
void users_item_to_string (UsersItem *);
UsersItem * _find_by_id (Users *us, uid_t id);
