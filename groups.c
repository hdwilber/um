#include "groups.h"
#include "users.h"
enum {
  GID_COL,
  GNAME_COL,
  GINUSER_COL,
  GISNEW_COL,
  GN_COLS
};

void add_groups_columns(Groups *gs);
void groups_create_model (Groups *gs);

Groups *groups_new (GtkBuilder *b)
{
  Groups *gs = NULL;
  gs =(Groups *)malloc (sizeof (Groups));
  if (gs != NULL) {
    gs->tv = GTK_WIDGET(gtk_builder_get_object(b, "treeview_groups"));
    gs->add = GTK_WIDGET(gtk_builder_get_object(b, "button_add_group"));
    gs->del = GTK_WIDGET(gtk_builder_get_object(b, "button_del_group"));
    gs->set = GTK_WIDGET(gtk_builder_get_object(b, "button_set_group"));

    groups_create_model (gs);
    gtk_tree_view_set_model(GTK_TREE_VIEW(gs->tv), gs->model);
    add_groups_columns(gs);
    return gs;

  }

}

void groups_item_to_string (GroupsItem *g) {
  g_print("Group: id: %i and name: %s\n", g->gid, g->name);
}

void cell_edited (GtkCellRendererText *cell, 
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
  }

}

gboolean separator_row (GtkTreeModel *model,
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


void editing_started (GtkCellRendererText *cell,
    GtkCellEditable *editable, const gchar *path, gpointer data) {
  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (editable),
                                              separator_row, NULL, NULL);
}


void fixed_toggled (GtkCellRendererToggle *cell,
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

/*Función para llenar la variable groups con información de los grupos existentes en el sistema*/
void _populate_groups (Groups *gs) {
  GroupsItem *g = NULL;
  gs->list = NULL;
  struct group *grp =NULL;
  /*Realiza la apertura del archivo por defecto a ser leído*/
  setgrent();         
  /*Obtiene el primer grupo de la lista para continuar con el control y verificación del resto*/
  grp = getgrent();
  while (grp != NULL) {
    g = NULL;
    g = (GroupsItem *)malloc (sizeof (GroupsItem));
    if (g != NULL) {
      g->grp = malloc (sizeof (struct group));
      memcpy(g->grp, grp, sizeof(struct group));
      g->is_new = FALSE;
      g->name = strdup(grp->gr_name);
      g->gid = grp->gr_gid;
      gs->list = g_list_append(gs->list, g);
      g->is_new = FALSE;
      g->in_user = FALSE;
    }
    grp=getgrent();
  }
  /*Cierra el archivo de lectura abierto /etc/groups*/
  endgrent();
}


/* 
 * Se realiza una actualización de los grupos con el usuario seleccionado
 */
void update_model (Groups *gs, UsersItem *u) {
  GtkTreePath *path;
  GtkTreeIter iter;

  /* Se obtiene el origen del modelo para ir recorriendo y estableciendo su pertenencia
   */
  gtk_tree_model_get_iter_from_string (gs->model, &iter, "0");
  path = gtk_tree_model_get_path (gs->model, &iter);
  
  g_printf ("Current: %s\n", u->name);

  do 
  {
    gid_t grp_id;
    gtk_tree_model_get(gs->model,&iter, GID_COL, &grp_id, -1); 
    gtk_list_store_set(GTK_LIST_STORE(gs->model), &iter, GINUSER_COL, FALSE, -1);

    if (u != NULL) {
      if (check_group (u, grp_id)) {
        gtk_list_store_set(GTK_LIST_STORE(gs->model), &iter, GINUSER_COL, TRUE, -1);
      }
      else {
        /*g_print ("error updating...");*/
      }
    }
  }  while (gtk_tree_model_iter_next(gs->model,&iter));

  g_print ("Se ha cambiado lo gruspo \n");
  /*g_free(path);*/
}

/* Crea el modelo de datos para GtkTreeView para la vista de los grupos
 */
void groups_create_model (Groups *gs) {
  GtkListStore *model;
  GtkTreeIter iter;

  model = gtk_list_store_new (GN_COLS,
      G_TYPE_UINT,
      G_TYPE_STRING,
      G_TYPE_BOOLEAN,
      G_TYPE_BOOLEAN
      );
  /*Se crea y llena la lista de grupos como variable global*/
  _populate_groups(gs);
  GList *o_groups = gs->list;

  while(gs->list!= NULL) {
    GroupsItem *grp = gs->list->data;
    gtk_list_store_append(model, &iter);
    gtk_list_store_set(model, &iter, 
        GID_COL, grp->gid,
        GNAME_COL, grp->name,
        GINUSER_COL, grp->in_user,
        GISNEW_COL, grp->is_new,
        -1);
    gs->list = g_list_next(gs->list);
    groups_item_to_string (grp);
  }
  gs->list = o_groups;
  gs->model = GTK_TREE_MODEL(model);
}

void add_groups_columns(Groups *gs) {
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Id", renderer,
      "text", GID_COL, 
      NULL);
  gtk_tree_view_column_set_sort_column_id (column, GID_COL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(gs->tv), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Name", renderer,
      "text", GNAME_COL,
      NULL);

  g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (GNAME_COL));
  g_object_set(renderer, "editable", TRUE, NULL);

  g_signal_connect (renderer, "edited", G_CALLBACK (cell_edited), gs->model);
  gtk_tree_view_column_set_sort_column_id (column,GNAME_COL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(gs->tv), column);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes ("In", renderer,
      "active", GINUSER_COL,
      NULL);
  g_signal_connect (renderer, "toggled", G_CALLBACK (fixed_toggled), gs->model);

  gtk_tree_view_append_column (GTK_TREE_VIEW(gs->tv), column);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes ("New", renderer,
      "active", GISNEW_COL,
      NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(gs->tv), column);

}

void groups_insert (Groups *gs, GroupsItem *g) {
  GtkTreeIter iter;
  gs->list = g_list_append (gs->list, g);

  gtk_list_store_append (GTK_LIST_STORE (gs->model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (gs->model), &iter,
        GID_COL, g->gid,
        GNAME_COL, g->name,
        GISNEW_COL, g->is_new,
        -1);
}
