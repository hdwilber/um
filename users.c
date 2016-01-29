#include "users.h"
void u_cell_edited (GtkCellRendererText *cell, 
    const gchar *path_string, const gchar *new_text,
    gpointer data);

void users_add_columns(Users *);
void users_create_shells_model (Users *);
void users_create_model(Users *);

void users_item_to_string (UsersItem *u) {
  g_print ("Id: %i, Name: %s, Home: %s, Shell: %s\n", u->uid, u->name, u->home, u->shell);
}

enum {
  /*Identificación par la columna de ID*/
  UID_COL,
  /*Identificación para la columna de nombre o login*/
  UNAME_COL,
  /*Identificación para el nombre completo\*/
  UFULLNAME_COL,
  /*Columna de shell*/
  USHELL_COL,
  /*Columna de entorno de trabajo $home*/
  UHOMEPATH_COL,
  /*Columna de nuevo usuario*/
  UISNEW_COL,
  UN_COLS
};

enum {
  SHELL_PATH_COL,
  SHELL_NCOLS
};

Users *users_new(GtkBuilder *b) {
  Users *us = NULL;
  us = (Users*)malloc(sizeof(Users));
  if (us != NULL) {
    us->tv = GTK_WIDGET(gtk_builder_get_object(b, "treeview_users"));
    us->add = GTK_WIDGET(gtk_builder_get_object(b, "button_add_user"));
    us->del = GTK_WIDGET(gtk_builder_get_object(b, "button_del_user"));
    us->set = GTK_WIDGET(gtk_builder_get_object(b, "button_set_user"));

    users_create_shells_model (us);
    users_create_model (us);
    gtk_tree_view_set_model(GTK_TREE_VIEW(us->tv), us->model);
    users_add_columns(us);
    return us;

  }
}

void users_insert(Users *us, UsersItem *u) {
  GtkTreeIter iter;
  us->list = g_list_append (us->list, u);

  gtk_list_store_append (GTK_LIST_STORE (us->model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (us->model), &iter,
        UID_COL, u->uid,
        UNAME_COL, u->name,
        UFULLNAME_COL,u->fullname,
        USHELL_COL, u->shell,
        UHOMEPATH_COL, u->home,
        UISNEW_COL, u->is_new,
        -1);
}

UsersItem * users_get_current (Users *us) {
  GtkTreePath *path =NULL;
  GtkTreeIter iter;
  UsersItem *u = (UsersItem *) malloc(sizeof(UsersItem));

  gtk_tree_view_get_cursor(GTK_TREE_VIEW(us->tv), &path, NULL); 
  g_print (gtk_tree_path_to_string (path));
  gtk_tree_model_get_iter (us->model, &iter, path);
  memset(u, 0, sizeof(UsersItem));

  gtk_tree_model_get (GTK_TREE_MODEL(us->model), &iter, UISNEW_COL, &u->is_new, -1);

  gtk_tree_model_get (us->model, &iter, UNAME_COL, &u->name, -1);
  gtk_tree_model_get (us->model, &iter, UFULLNAME_COL, &u->fullname, -1);
  gtk_tree_model_get (us->model, &iter, UHOMEPATH_COL, &u->home, -1);
  gtk_tree_model_get (us->model, &iter, USHELL_COL, &u->shell, -1);
  gtk_tree_model_get (us->model, &iter, UID_COL, &u->uid, -1);

  return u; 
}


gboolean u_separator_row (GtkTreeModel *model,
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

void u_editing_started (GtkCellRendererText *cell,
    GtkCellEditable *editable, const gchar *path, gpointer data) {
  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (editable),
                                              u_separator_row, NULL, NULL);
}

/* Realiza la verificación de un usuario dentro de un grupo con gid
 * Se debe cambiar a verificar dentro la lista del usuario
 */
gboolean check_group(UsersItem *u, gid_t gid)
{
  int i = 0;
  for (i = 0; i < u->n_groups; i++) {
    if (u->groups[i] == gid) {
      return TRUE;
    }
  }
  return FALSE;
}

_parse_gecos (UsersItem *u, char *data) {
  char delimiters[] = ",";
  char *running = NULL;
  char *token;

  running = strdup (data);
  token = strsep (&running, delimiters);
  
  u->fullname = token;
}

/* 
 * Realiza una búsqueda del usuario dentro las listas de usuarios con el identificador como parámetro
 */
UsersItem * _find_by_id (Users *us, uid_t id) {
  GList *o_users = us->list;
  UsersItem *u=NULL;
  if (id == 0) return NULL;
  do {
    u= us->list->data;
    if (u->uid == id) {
      break; 
    }
    us->list = g_list_next(us->list);
  }
  while (us->list != NULL);
  us->list = o_users;
  return u;
}

void _populate_users(Users *us) {
  UsersItem *u;
  struct passwd *usr = NULL;
  us->list = NULL;
  /*Abre el archivo por defecto para la lectura de los usuarios.*/
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

        us->list = g_list_append (us->list, u);
        u->n_groups = 1;
        u->groups = malloc (sizeof(gid_t));
        
        /*Obtiene la lista de grupos auxiliares al que pertenec el usuario actual a ser leido
         * La primera vez, getgrouplist devuelve en el puntero de entero de entrada
         * en este caso n->groups la cantidad de grupos que realmente tiene el usuario
         * Para lo cual, una vez obtenido la cantidad total de grupos, se realiza una
         * segunda llamada con la memoria reservada para la cantidad total.
         */
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

void users_create_model(Users *us) {
  GtkListStore *model;    // Modelo de listas simples 
  GtkTreeIter iter;

  model = gtk_list_store_new (UN_COLS,
      G_TYPE_UINT,
      G_TYPE_STRING,
      G_TYPE_STRING, 
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_BOOLEAN
      );
  _populate_users(us);
  GList *o_users = us->list;
  while(us->list != NULL) {
    UsersItem *usr = us->list->data;

    /* Se inserta en el modelo de datos, la información con las columnas
     * definidas anteriormente
     */
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
    us->list = g_list_next(us->list);
  }
  /* Se preserva el origen de la lista de usuarios */
  us->list = o_users;
  us->model = GTK_TREE_MODEL(model);
}

void users_add_columns(Users *us) {
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* Column for user id */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Id", renderer,
      "text", UID_COL, 
      NULL);
  gtk_tree_view_column_set_sort_column_id (column, UID_COL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(us->tv), column);

  /*Column for user name (login name) */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Login", renderer,
      "text", UNAME_COL,
      NULL);

  g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (UNAME_COL));
  g_object_set(renderer, "editable", TRUE, NULL);

  g_signal_connect (renderer, "edited", G_CALLBACK (u_cell_edited), us->model);
  gtk_tree_view_column_set_sort_column_id (column,UNAME_COL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(us->tv), column);

  /* Column for Full Name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Name", renderer,
      "text", UFULLNAME_COL,
      NULL);

  g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (UFULLNAME_COL));
  g_object_set(renderer, "editable", TRUE, NULL);

  g_signal_connect (renderer, "edited", G_CALLBACK (u_cell_edited), us->model);
  gtk_tree_view_column_set_sort_column_id (column,UFULLNAME_COL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(us->tv), column);

  /*Column for shell */
  renderer = gtk_cell_renderer_combo_new();
  g_object_set (renderer, "model", us->shells_model,
    "text-column", SHELL_PATH_COL,
    "has-entry", FALSE,
    "editable", TRUE,
    NULL);

  column = gtk_tree_view_column_new_with_attributes ("Shell", renderer,
      "text", USHELL_COL,
      NULL);
  g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (USHELL_COL));
  g_signal_connect (renderer, "edited", G_CALLBACK (u_cell_edited), us->model);
  g_signal_connect (renderer, "editing-started", G_CALLBACK (u_editing_started), NULL);
  gtk_tree_view_column_set_sort_column_id (column,USHELL_COL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(us->tv), column);

  /*Home path column*/
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Home", renderer,
      "text", UHOMEPATH_COL,
      NULL);
  gtk_tree_view_column_set_sort_column_id (column,UHOMEPATH_COL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(us->tv), column);
  g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (UHOMEPATH_COL));
  g_object_set(renderer, "editable", TRUE, NULL);

  g_signal_connect (renderer, "edited", G_CALLBACK (u_cell_edited), us->model);


  /*Is new users column */
  renderer = gtk_cell_renderer_toggle_new ();
  /*g_signal_connect (renderer, "toggled", G_CALLBACK (fixed_toggled), model);*/
  column = gtk_tree_view_column_new_with_attributes ("New", renderer,
      "active", UISNEW_COL,
      NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(us->tv), column);

}

void users_create_shells_model (Users *us) {
  GtkListStore *model =NULL;
  GtkTreeIter iter;

  model = gtk_list_store_new (SHELL_NCOLS, G_TYPE_STRING);
  _populate_shells(us);
  GList *o_shells = us->shells;
  while (us->shells != NULL) {
    gtk_list_store_append (GTK_LIST_STORE(model), &iter);
    gtk_list_store_set (GTK_LIST_STORE(model), &iter,
        SHELL_PATH_COL, us->shells->data,
        -1);
    us->shells = g_list_next(us->shells);
  }
  us->shells = o_shells;
  us->shells_model = GTK_TREE_MODEL(model);

}

/* Populate shells GList using the default file path.
 * The first line is avoided
 */
_populate_shells (Users *us) {
  GFile *shells_file = g_file_new_for_path(SHELLS_FILE_PATH);
  GError *err =NULL;
  GDataInputStream *data_stream = NULL;
  GFileInputStream *shells_stream = g_file_read (shells_file, NULL, &err);
  data_stream = g_data_input_stream_new (G_INPUT_STREAM(shells_stream));
  us->shells = NULL;
  g_print ("HOLA\n");
  gsize length=0;
  char *mytext;
  // Avoid the first line
  mytext = g_data_input_stream_read_line (data_stream, &length, NULL, &err);
  mytext = g_data_input_stream_read_line (data_stream, &length, NULL, &err);

  while (mytext != NULL) {
    us->shells = g_list_append (us->shells, mytext);
    mytext = g_data_input_stream_read_line (data_stream, &length, NULL, &err);
    g_print ("%s\n", mytext);
  }
  g_input_stream_close (G_INPUT_STREAM(shells_stream), NULL, &err);
  g_object_unref (shells_file);

}


/*void users_changed (GtkTreeView *tv, gpointer data) {*/
  /*GtkTreePath *path =NULL;*/
  /*GtkWidget *grp_tv = NULL;*/
  /*GtkTreeIter iter;*/
  /*GtkTreeModel *model;*/
  /*GtkTreeModel *grp= NULL;*/
  /*model = g_object_get_data (G_OBJECT(data), "users");*/

  /*gtk_tree_view_get_cursor(tv, &path, NULL); */
  /*g_print (gtk_tree_path_to_string (path));*/
  /*gtk_tree_model_get_iter (model, &iter, path);*/

  /*int id=0;*/

  /*gtk_tree_model_get(model,&iter, UID_COL, &id, -1); */

  /*grp = g_object_get_data (G_OBJECT(data), "groups");*/
  /*grp_tv = g_object_get_data (G_OBJECT(data), "groups-tv");*/

  /*if (id != 0) {*/
    /*update_groups_model (grp, _find_by_id(id));*/
      /*gtk_widget_set_sensitive (GTK_WIDGET (grp_tv), TRUE);*/
  /*}*/
  /*else {*/
    /*gtk_widget_set_sensitive (GTK_WIDGET (grp_tv), FALSE);*/

  /*}*/
/*}*/

void u_cell_edited (GtkCellRendererText *cell, 
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

