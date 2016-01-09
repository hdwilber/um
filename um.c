#include <gtk/gtk.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <memory.h>
#include <string.h>
#include <gio/gio.h>
#include <libuser/config.h>
#include <libuser/user.h>
#include <sys/types.h>
#include <sys/wait.h>


#define SHELLS_FILE_PATH "/etc/shells"
GList *users = NULL;
GList *shells = NULL;
GList *groups = NULL;
gint next_uid = 0;

enum {
  UID_COL,
  UNAME_COL,
  UFULLNAME_COL,
  USHELL_COL,
  UHOMEPATH_COL,
  UISNEW_COL,
  UN_COLS
};

enum {
  GID_COL,
  GNAME_COL,
  GINUSER_COL,
  GNOTNEW_COL,
  GN_COLS
};

typedef struct _UsersItem UsersItem;
struct _UsersItem {
  uid_t uid;
  gid_t gid;
  char *name;
  char *fullname;
  char *shell;
  char * home;
  gboolean is_new;
  gid_t *groups;
  int n_groups;
};

typedef struct _GroupsItem GroupsItem;
struct _GroupsItem {
  gid_t gid;
  gchar * name;
  struct group *grp;
  gboolean not_new;
  gboolean in_user;
};

static void user_item_to_string (UsersItem *u) {
  g_print ("Id: %i, Name: %s, Home: %s, Shell: %s\n", u->uid, u->name, u->home, u->shell);

}
_parse_gecos (UsersItem *u, char *data) {
  char delimiters[] = ",";
  char *running = NULL;
  char *token;

  running = strdup (data);
  token = strsep (&running, delimiters);
  
  u->fullname = token;
}

UsersItem * _find_by_id (uid_t id) {
  GList *o_users = users;
  UsersItem *u=NULL;
  if (id == 0) return NULL;
  do {
    u= users->data;
    if (u->uid == id) {
      break; 
    }
    users = g_list_next(users);
  }
  while (users != NULL);
  users = o_users;
  return u;
}

static gboolean
on_delete_event (GtkWidget *widget, GdkEvent  *event, gpointer   data) {
  g_print ("Going out...\n");
  gtk_main_quit();
  return TRUE;
}

void _populate_groups () {
  GroupsItem *g = NULL;
  struct group *grp =NULL;
  setgrent();
  grp = getgrent();
  while (grp != NULL) {
    g = NULL;
    g = (GroupsItem *)malloc (sizeof (GroupsItem));
    if (g != NULL) {
      g->grp = malloc (sizeof (struct group));
      memcpy(g->grp, grp, sizeof(struct group));
      g->not_new = TRUE;
      g->name = strdup(grp->gr_name);
      g->gid = grp->gr_gid;
      groups = g_list_append (groups, g);
      g->not_new = TRUE;
      g->in_user = FALSE;
    }
    grp=getgrent();
  }
  endgrent();

}


/**
 * Create a list with users insido GList *users variable
 * Using default password file.
 */
void _populate_users() {
  UsersItem *u;
  struct passwd *usr = NULL;
  setpwent();
  usr = getpwent();
  while (usr != NULL) {
    u = NULL;
    u = (UsersItem *)(malloc (sizeof(UsersItem)));
    if (u != NULL) {
      if (usr->pw_uid >= 1000 && usr->pw_uid < 60000) {
        u->uid = usr->pw_uid;
        u->gid = usr->pw_gid;
        u->name = strdup(usr->pw_name);
        /*u->fullname = strdup(usr->pw_gecos);*/
        _parse_gecos(u, usr->pw_gecos);
        u->home= strdup (usr->pw_dir);
        u->shell = strdup (usr->pw_shell);
        u->is_new = FALSE;

        users = g_list_append (users, u);
        u->n_groups = 1;
        u->groups = malloc (sizeof(gid_t));
        
        if (getgrouplist (u->name, u->gid, u->groups, &u->n_groups)) {
          g_free(u->groups);
          u->groups = malloc (u->n_groups * sizeof(gid_t));
          getgrouplist (u->name, u->gid, u->groups, &u->n_groups);
          g_print ("Group: %i\n", u->groups[1]);

        }
      }
    }
    usr = getpwent();
  }
  endpwent();

}

static gboolean check_group(UsersItem *u, gid_t gid)
{
  int i = 0;
  for (i = 0; i < u->n_groups; i++) {
    if (u->groups[i] == gid) {
      return TRUE;
    }
  }
  return FALSE;
}

static void update_groups_model (GtkTreeModel *model, UsersItem *u) {
  GtkTreePath *path;
  GtkTreeIter iter;
  gtk_tree_model_get_iter_from_string (model, &iter, "0");
  path = gtk_tree_model_get_path (model, &iter);
  
  g_printf ("Current: %s\n", u->name);

  do 
  {
    gid_t grp_id;
    gtk_tree_model_get(model,&iter, GID_COL, &grp_id, -1); 
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, GINUSER_COL, FALSE, -1);

    if (u != NULL) {
      if (check_group (u, grp_id)) {
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, GINUSER_COL, TRUE, -1);
      }
      else {
        /*g_print ("error updating...");*/
      }
    }
  }  while (gtk_tree_model_iter_next(model,&iter));

  g_print ("Se ha cambiado lo gruspo \n");
  /*g_free(path);*/
  

}

static GtkTreeModel *create_groups_model () {
  GtkListStore *model;
  GtkTreeIter iter;

  model = gtk_list_store_new (GN_COLS,
      G_TYPE_UINT,
      G_TYPE_STRING,
      G_TYPE_BOOLEAN,
      G_TYPE_BOOLEAN
      );
  _populate_groups();
  GList *o_groups = groups;
  while(groups != NULL) {
    GroupsItem *grp = groups->data;
    gtk_list_store_append(model, &iter);
    gtk_list_store_set(model, &iter, 
        GID_COL, grp->gid,
        GNAME_COL, grp->name,
        GINUSER_COL, grp->in_user,
        GNOTNEW_COL, !grp->not_new,
        -1);
    /*tree_item_to_string(users->data);*/
    groups = g_list_next(groups);
  }
  groups= o_groups;
  return GTK_TREE_MODEL (model);
}

static GtkTreeModel *create_users_model () {
  GtkListStore *model;
  GtkTreeIter iter;

  model = gtk_list_store_new (UN_COLS,
      G_TYPE_UINT,
      G_TYPE_STRING,
      G_TYPE_STRING, 
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_BOOLEAN
      );
  _populate_users();
  GList *o_users = users;
  while(users != NULL) {
    UsersItem *usr = users->data;

    gtk_list_store_append(model, &iter);
    gtk_list_store_set(model, &iter, 
        UID_COL, usr->uid,
        UNAME_COL, usr->name,
        UFULLNAME_COL,usr->fullname,
        USHELL_COL, usr->shell,
        UHOMEPATH_COL, usr->home,
        UISNEW_COL, usr->is_new,
        -1);
    /*tree_item_to_string(users->data);*/
    users = g_list_next(users);
  }
  users = o_users;
  return GTK_TREE_MODEL (model);

}

static void cell_edited (GtkCellRendererText *cell, 
    const gchar *path_string, const gchar *new_text,
    gpointer data) {

  GtkTreeModel *model = (GtkTreeModel *)data;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  UsersItem *u;

  gint column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

  gtk_tree_model_get_iter (model, &iter, path);
  int i = gtk_tree_path_get_indices (path)[0];
  


  switch (column) {
    case UNAME_COL: {
                     char *old_text;
                     gtk_tree_model_get (model, &iter, column, &old_text, -1);
                     if (old_text != NULL)
                       g_free (old_text);
                     g_print (gtk_tree_path_to_string (path));
                     gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, new_text, -1);
                     g_print ("Changed to: %s\n", new_text);

                     break;
                   }
    case UFULLNAME_COL: {
                    char *old_text;
                     gtk_tree_model_get (model, &iter, column, &old_text, -1);
                     if (old_text != NULL)
                       g_free (old_text);
                     gtk_tree_path_get_indices (path)[0];
                     gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, new_text, -1);
                     g_print ("Changed to: %s\n", new_text);
                     break;
    }
    case UHOMEPATH_COL: {


                    char *old_text;
                     gtk_tree_model_get (model, &iter, column, &old_text, -1);
                     if (old_text != NULL)
                       g_free (old_text);
                     gtk_tree_path_get_indices (path)[0];
                     gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, new_text, -1);
                     g_print ("Changed to: %s\n", new_text);
                     break;
    }
    case USHELL_COL: {
                     char *old_text;
                     if (old_text !=NULL) 
                       gtk_tree_model_get (model, &iter, column, &old_text, -1);
                       g_free (old_text);
                     gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, new_text, -1);
                     g_print ("Changed to: %s\n", new_text);
                     }
  }



}


static gboolean separator_row (GtkTreeModel *model,
                   GtkTreeIter  *iter,
                  gpointer      data)
{
  GtkTreePath *path;
  gint idx;

  path = gtk_tree_model_get_path (model, iter);
  idx = gtk_tree_path_get_indices (path)[0];

  gtk_tree_path_free (path);

  return idx == 5;
}


static void editing_started (GtkCellRendererText *cell,
    GtkCellEditable *editable, const gchar *path, gpointer data) {
  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (editable),
                                              separator_row, NULL, NULL);
}


enum {
  SHELL_PATH_COL,
  SHELL_NCOLS
  
};

static GtkTreeModel *create_shells_model () {
  GtkListStore *model =NULL;
  GtkTreeIter iter;

  model = gtk_list_store_new (SHELL_NCOLS, G_TYPE_STRING);
  _populate_shells();
  GList *o_shells = shells;
  while (shells != NULL) {
    gtk_list_store_append (model, &iter);
    gtk_list_store_set (model, &iter,
        SHELL_PATH_COL, shells->data,
        -1);
    shells = g_list_next(shells);
  }
  shells = o_shells;
  return GTK_TREE_MODEL (model);

}

static void fixed_toggled (GtkCellRendererToggle *cell,
                            gchar                 *path_str,
                            gpointer               data)
{
GtkTreeModel *model = (GtkTreeModel *)data;
GtkTreeIter  iter;
GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
gboolean fixed;

/* get toggled iter */
gtk_tree_model_get_iter (model, &iter, path);
gtk_tree_model_get (model, &iter, GINUSER_COL, &fixed, -1);

/* do something with the value */
fixed ^= 1;

/* set new value */
gtk_list_store_set (GTK_LIST_STORE (model), &iter, GINUSER_COL, fixed, -1);

/* clean up */
gtk_tree_path_free (path);
}


static void add_groups_columns(GtkTreeView *tv, GtkTreeModel *groups_model) {
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Id", renderer,
      "text", GID_COL, 
      NULL);
  gtk_tree_view_column_set_sort_column_id (column, GID_COL);
  gtk_tree_view_append_column (tv, column);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Name", renderer,
      "text", GNAME_COL,
      NULL);

  g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (GNAME_COL));
  g_object_set(renderer, "editable", TRUE, NULL);

  g_signal_connect (renderer, "edited", G_CALLBACK (cell_edited), groups_model);
  gtk_tree_view_column_set_sort_column_id (column,GNAME_COL);
  gtk_tree_view_append_column (tv, column);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes ("In", renderer,
      "active", GINUSER_COL,
      NULL);
  g_signal_connect (renderer, "toggled", G_CALLBACK (fixed_toggled), groups_model);

  gtk_tree_view_append_column (tv, column);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes ("New", renderer,
      "active", GNOTNEW_COL,
      NULL);
  gtk_tree_view_append_column (tv, column);

}

static void add_columns(GtkTreeView *tv, GtkTreeModel *users_model,  GtkTreeModel *shells_model) {
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* Column for user id */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Id", renderer,
      "text", UID_COL, 
      NULL);
  gtk_tree_view_column_set_sort_column_id (column, UID_COL);
  gtk_tree_view_append_column (tv, column);

  /*Column for user name (login name) */

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Login", renderer,
      "text", UNAME_COL,
      NULL);

  g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (UNAME_COL));
  g_object_set(renderer, "editable", TRUE, NULL);

  g_signal_connect (renderer, "edited", G_CALLBACK (cell_edited), users_model);
  gtk_tree_view_column_set_sort_column_id (column,UNAME_COL);
  gtk_tree_view_append_column (tv, column);

  /* Column for Full Name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Name", renderer,
      "text", UFULLNAME_COL,
      NULL);

  g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (UFULLNAME_COL));
  g_object_set(renderer, "editable", TRUE, NULL);

  g_signal_connect (renderer, "edited", G_CALLBACK (cell_edited), users_model );
  gtk_tree_view_column_set_sort_column_id (column,UFULLNAME_COL);
  gtk_tree_view_append_column (tv, column);

  /*Column for shell */
  renderer = gtk_cell_renderer_combo_new();
  g_object_set (renderer, "model", shells_model,
    "text-column", SHELL_PATH_COL,
    "has-entry", FALSE,
    "editable", TRUE,
    NULL);

  column = gtk_tree_view_column_new_with_attributes ("Shell", renderer,
      "text", USHELL_COL,
      NULL);
  g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (USHELL_COL));
  g_signal_connect (renderer, "edited", G_CALLBACK (cell_edited), users_model);
  g_signal_connect (renderer, "editing-started", G_CALLBACK (editing_started), NULL);
  gtk_tree_view_column_set_sort_column_id (column,USHELL_COL);
  gtk_tree_view_append_column (tv, column);

  /*Home path column*/
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Home", renderer,
      "text", UHOMEPATH_COL,
      NULL);
  gtk_tree_view_column_set_sort_column_id (column,UHOMEPATH_COL);
  gtk_tree_view_append_column (tv, column);
  g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (UHOMEPATH_COL));
  g_object_set(renderer, "editable", TRUE, NULL);

  g_signal_connect (renderer, "edited", G_CALLBACK (cell_edited), users_model);


  /*Is new users column */
  renderer = gtk_cell_renderer_toggle_new ();
  /*g_signal_connect (renderer, "toggled", G_CALLBACK (fixed_toggled), model);*/
  column = gtk_tree_view_column_new_with_attributes ("New", renderer,
      "active", UISNEW_COL,
      NULL);
  gtk_tree_view_append_column (tv, column);

}

/* Populate shells GList using the default file path.
 * The first line is avoided
 */
_populate_shells () {
  GFile *shells_file = g_file_new_for_path(SHELLS_FILE_PATH);
  GError *err =NULL;
  GDataInputStream *data_stream = NULL;
  GFileInputStream *shells_stream = g_file_read (shells_file, NULL, &err);
  data_stream = g_data_input_stream_new (G_INPUT_STREAM(shells_stream));
  gsize length=0;
  char *mytext;
  // Avoid the first line
  mytext = g_data_input_stream_read_line (data_stream, &length, NULL, &err);
  mytext = g_data_input_stream_read_line (data_stream, &length, NULL, &err);

  while (mytext != NULL) {
    shells = g_list_append (shells, mytext);
    mytext = g_data_input_stream_read_line (data_stream, &length, NULL, &err);
  }
  g_input_stream_close (G_INPUT_STREAM(shells_stream), NULL, &err);
  g_object_unref (shells_file);
}



static void
add_new_user(GtkWidget *button, gpointer data)
{
  UsersItem user;
  GtkTreeIter iter;
  GtkTreeModel *model = (GtkTreeModel *)data;

  user.uid = 0;
  user.name = "";
  user.home = "";
  user.fullname = "";
  user.shell = "/usr/bash";
  user.is_new= TRUE;
  user.n_groups = 0;
  user.groups = NULL;

  users = g_list_append (users, &user);

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
        UID_COL, user.uid,
        UNAME_COL, user.name,
        UFULLNAME_COL,user.fullname,
        USHELL_COL, user.shell,
        UHOMEPATH_COL, user.home,
        UISNEW_COL, user.is_new,
        -1);
}

/*void _replace_char(char *data, char c, char *cr) {*/
  /*int length = strlen (data);*/
  /*int i =0;*/
  /*g_print ("%s converterd in:", data);*/
  /*for( i = 0; i< length; i++) {*/
    /*if (data[i] == c) {*/
      /*data[i] = cr;*/
    /*}*/
  /*}*/
  /*g_print ("%s\n", data);*/

/*}*/

/*char * _build_args (char *cmd,...) {*/
  /*va_list ars;*/
  /*char *aux = NULL;*/
  /*va_start (ars, cmd);*/
  /*aux = va_arg(ars, char *);*/
  /*char *ret = g_strdup(cmd);*/

  /*while (aux != NULL) {*/
    /*ret = g_strconcat (ret, aux, NULL);*/
    /*aux = va_arg(ars, char *);*/
  /*}*/
  /*va_end(ars);*/
  /*return ret;*/
/*}*/
void confirm_user (GtkButton *btn, gpointer data) {
  GtkTreeView *tv = data;
  GtkTreePath *path =NULL;
  GtkTreeIter iter;
  GtkTreeModel *model;

  model = g_object_get_data (G_OBJECT(data), "users");

  gtk_tree_view_get_cursor(tv, &path, NULL); 
  g_print (gtk_tree_path_to_string (path));
  gtk_tree_model_get_iter (model, &iter, path);

  gboolean is_new=FALSE;

  gtk_tree_model_get (model, &iter, UISNEW_COL, &is_new, -1);
  char *aux;
  char *name; 
  char *fullname;
  char *homepath;
  char *shell;
  int id; 

  gtk_tree_model_get (model, &iter, UNAME_COL, &name, -1);
  gtk_tree_model_get (model, &iter, UFULLNAME_COL, &fullname, -1);
  gtk_tree_model_get (model, &iter, UHOMEPATH_COL, &homepath, -1);
  gtk_tree_model_get (model, &iter, USHELL_COL, &shell, -1);
  gtk_tree_model_get (model, &iter, UID_COL, &id, -1);
  char *myoutput=NULL, *mystd=NULL;
  int myexit=0;
  GError *err ;
  char *fullcmd = NULL;
  char **myargs =NULL;

  if (is_new) {
    char cmd[] = "/usr/sbin/useradd";
    char *aux = "";
    /*fullcmd = g_strdup_printf( "%s|-m|-c|\"%s,,,\"|-d|%s|-s|%s|%s", cmd, fullname, homepath, shell, name);*/
    if (g_strcmp0(name, "") == 0) {
    }
    else {
      if (g_strcmp0(fullname, "") != 0) {
        aux = g_strdup_printf ("-c|\"%s,,,\"", fullname);
      }
      if (g_strcmp0(homepath, "") != 0) {
        aux = g_strdup_printf ("-m|-d|%s", homepath);
      }
      if (g_strcmp0(shell, "") != 0) {
        aux = g_strdup_printf("-s|%s", shell);
      }
      fullcmd = g_strdup_printf("%s|%s|%s", cmd, aux, name);
    }
  }
  else {
    UsersItem *u = _find_by_id (id);
    g_print ("Changing: %s to %s\n", u->name, name);
    if (u != NULL) {
      char cmd[] = "/usr/sbin/usermod";
      char *aux = "";
      if (g_strcmp0(u->name, name) != 0) {
        aux =  g_strdup_printf( "-l|%s",name);
      }
      if(g_strcmp0(u->home, homepath) != 0) {
        aux =  g_strdup_printf( "%s|-dm|%s",aux, homepath);
      }
      if (g_strcmp0(u->shell, shell) != 0) {
        aux = g_strdup_printf( "%s|-s|%s", aux, shell);
      }
      if (g_strcmp0(u->fullname, fullname) != 0) {
        aux = g_strdup_printf( "%s|-c|\"%s,,,\"", aux, fullname);
      }
      
      if (g_strcmp0(aux, "") != 0) {
        fullcmd = g_strdup_printf("%s|%s|%s",cmd, aux, u->name);
      }
    }
  }

  if (fullcmd != NULL) {
    myargs = g_strsplit (fullcmd, "|", -1);
    g_print ("This command to exec: %s\n", fullcmd);

    /*int i = 0;*/
    /*char *a = myargs[i];*/
    /*while(a != NULL){*/
      /*g_print ("Values: %s\t", a);*/
      /*i++;*/
      /*a = myargs[i];*/

    /*}*/

    /*g_spawn_sync (NULL, myargs, NULL, G_SPAWN_DEFAULT, NULL,NULL,*/
        /*&myoutput, &mystd, &myexit, NULL);*/

    /*g_print ("stdout: %s\n", myoutput);*/
    /*g_print ("stderr: %s\n", mystd);*/

    /*switch (myexit/256){*/
      /*case 0: g_print("Success\n");*/
              /*break;*/
      /*case 1: g_print ("No se puede actualizar el archivo passwd. Insuficientes permisos\n");*/
              /*break;*/
      /*case 9: g_print ("ya existe el usuario\n");break;*/
    /*}*/
  }
  else {
    g_print ("there are no commands to exec\n");
  }


}
void users_changed (GtkTreeView *tv, gpointer data) {
  GtkTreePath *path =NULL;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeModel *grp= NULL;
  model = g_object_get_data (G_OBJECT(data), "users");

  gtk_tree_view_get_cursor(tv, &path, NULL); 
  g_print (gtk_tree_path_to_string (path));
  gtk_tree_model_get_iter (model, &iter, path);

  int id=0;

  gtk_tree_model_get(model,&iter, UID_COL, &id, -1); 

  grp = g_object_get_data (G_OBJECT(data), "groups");

  if (id != 0) 
    update_groups_model (grp, _find_by_id(id));
}

void delete_user (GtkWidget *btn, gpointer data)
{
  GtkTreePath *path =NULL;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeModel *grp= NULL;
  model = g_object_get_data (G_OBJECT(data), "users");

  gtk_tree_view_get_cursor(GTK_TREE_VIEW(data), &path, NULL); 
  g_print (gtk_tree_path_to_string (path));
  gtk_tree_model_get_iter (model, &iter, path);

  char *name = NULL;
  int is_new = NULL;
  gtk_tree_model_get(model,&iter, UNAME_COL, &name, UISNEW_COL, &is_new, -1); 
  if (name != NULL) {
    char *myoutput=NULL, *mystd=NULL;
    int myexit=0;
    GError *err ;
    char *fullcmd = NULL;
    char **myargs =NULL;

    if (!is_new) {
      char cmd[] = "/usr/sbin/userdel";
      fullcmd =g_strdup_printf ("%s|%s", cmd, name);
      myargs = g_strsplit (fullcmd, "|", -1);
      g_print ("This command to exec: %s\n", fullcmd);
    }
  }

}


int main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *button;
  GtkWidget *users_tv, *groups_tv;
  GtkWidget *users_add_btn;
  GtkWidget *users_confirm_btn;
  GtkWidget *users_delete_btn;
  GtkBuilder *builder;
  GError *err;
  GtkTreeModel *umodel, *smodel, *gmodel;

  gtk_init (&argc, &argv);

  builder = gtk_builder_new_from_file("um.glade");
  window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));

  g_signal_connect (window, "delete-event", G_CALLBACK (on_delete_event), NULL);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  users_tv = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_users"));
  groups_tv = GTK_WIDGET(gtk_builder_get_object(builder, "treeview_groups"));
  users_add_btn = GTK_WIDGET(gtk_builder_get_object(builder, "button_add_user"));
  users_confirm_btn = GTK_WIDGET(gtk_builder_get_object(builder, "button_confirm_user"));
  users_delete_btn = GTK_WIDGET(gtk_builder_get_object(builder, "button_delete_user"));

  umodel = create_users_model ();
  smodel = create_shells_model();
  gmodel = create_groups_model ();

  gtk_tree_view_set_model (GTK_TREE_VIEW(users_tv), umodel);
  gtk_tree_view_set_model (GTK_TREE_VIEW(groups_tv), gmodel);

  add_columns(GTK_TREE_VIEW(users_tv), umodel, smodel);
  add_groups_columns(GTK_TREE_VIEW(groups_tv), gmodel);

  g_object_set_data (G_OBJECT (users_tv), "groups", (gpointer)(gmodel));
  g_object_set_data (G_OBJECT (users_tv), "users", (gpointer)(umodel));

  g_signal_connect(users_add_btn, "clicked", G_CALLBACK(add_new_user), umodel);
  g_signal_connect(users_confirm_btn, "clicked", G_CALLBACK(confirm_user), users_tv);
  g_signal_connect(users_delete_btn, "clicked", G_CALLBACK(delete_user), users_tv);
  g_signal_connect(users_tv, "cursor-changed", G_CALLBACK(users_changed), users_tv);
  gtk_widget_show(window);
  gtk_main();

  return 0;
}
