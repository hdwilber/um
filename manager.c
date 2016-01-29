#include "manager.h"
#include <gtk/gtk.h>

void manager_user_add(GtkButton*, gpointer);
void manager_user_set(GtkButton*, gpointer);
void manager_user_del(GtkButton*, gpointer);
void manager_group_add(GtkButton *, gpointer );
void manager_group_set(GtkButton *, gpointer );
void manager_group_del(GtkButton *, gpointer );
static gboolean
on_delete_event (GtkWidget *widget, GdkEvent  *event, gpointer   data) {
  g_print ("Going out...\n");
  gtk_main_quit();
  return TRUE;
}

Manager *manager_new(char *p) {
  Manager *m = NULL;
  m = (Manager *)malloc(sizeof(Manager));

  if (m != NULL) {
    m->b = gtk_builder_new_from_file(p);
    m->main = GTK_WIDGET(gtk_builder_get_object(m->b, "main_window"));

    m->us = users_new(m->b);
    m->gs = groups_new (m->b);

    gtk_widget_show (m->main);
    /*Añade las funcionalidades a los eventos */
    g_signal_connect (m->main, "delete-event", G_CALLBACK (on_delete_event), NULL);
    g_signal_connect (m->main, "destroy", G_CALLBACK (gtk_main_quit), NULL);

    g_object_set_data (G_OBJECT(m->us->tv), "manager", (gpointer)m);
    g_object_set_data (G_OBJECT(m->gs->tv), "manager", (gpointer)m);
    
    g_signal_connect(m->us->add, "clicked", G_CALLBACK(manager_user_add), m);
    g_signal_connect(m->us->set, "clicked", G_CALLBACK(manager_user_set), m);
    g_signal_connect(m->us->del, "clicked", G_CALLBACK(manager_user_del), (gpointer)m);

    g_signal_connect(m->gs->add, "clicked", G_CALLBACK(manager_group_add), (gpointer)m);
    /*g_signal_connect(m->gs->set, "clicked", G_CALLBACK(manager_group_set), m);*/
    /*g_signal_connect(m->gs->del, "clicked", G_CALLBACK(manager_group_del), (gpointer)m);*/
  }
  return m;
}

void manager_group_set(GtkButton *button, gpointer data)
{
}
void manager_group_del(GtkButton *button, gpointer data)
{
}
void manager_group_add(GtkButton *button, gpointer data)
{
  Manager *m = (Manager *)data;

  GroupsItem grp;
  grp.gid= 0;
  grp.name = "";
  grp.is_new= TRUE;
  grp.in_user = FALSE;

  groups_insert(m->gs, &grp);
}

void manager_user_add(GtkButton *button, gpointer data)
{
  Manager *m = (Manager *)data;

  UsersItem user;
  user.uid = 0;
  user.name = "";
  user.home = "";
  user.fullname = "";
  user.shell = "/usr/bash";
  user.is_new= TRUE;
  user.n_groups = 0;
  user.groups = NULL;

  users_insert (m->us, &user);

}

void manager_user_set (GtkButton *btn, gpointer data) {
  Manager *m = (Manager *)data;
  Users *us = m->us;
  
  UsersItem *ui = users_get_current (us);
  users_item_to_string (ui);

  char *myoutput=NULL, *mystd=NULL;
  int myexit=0;
  GError *err ;
  char *fullcmd = NULL;
  char **myargs =NULL;

  if (ui->is_new) {
    char cmd[] = "/usr/sbin/useradd";
    char *aux = "";
    if (g_strcmp0(ui->name, "") == 0) {
    }
    else {
      if (g_strcmp0(ui->fullname, "") != 0) {
        aux = g_strdup_printf ("-c|\"%s,,,\"", ui->fullname);
      }
      if (g_strcmp0(ui->home, "") != 0) {
        aux = g_strdup_printf ("%s|-m|-d|%s",aux, ui->home);
      }
      if (g_strcmp0(ui->shell, "") != 0) {
        aux = g_strdup_printf("%s|-s|%s", aux, ui->shell);
      }
      fullcmd = g_strdup_printf("TOTAL CMD: %s|%s|%s", cmd, aux, ui->name);
    }
  }
  else {
    UsersItem *u = _find_by_id (us, ui->uid);
    g_print ("Changing: %s to %s\n", u->name, ui->name);
    if (u != NULL) {
      char cmd[] = "/usr/sbin/usermod";
      char *aux = "";
      if (g_strcmp0(u->name, ui->name) != 0) {
        aux =  g_strdup_printf( "-l|%s",ui->name);
      }
      if(g_strcmp0(u->home, ui->home) != 0) {
        aux =  g_strdup_printf( "%s|-dm|%s",aux, ui->home);
      }
      if (g_strcmp0(u->shell, ui->shell) != 0) {
        aux = g_strdup_printf( "%s|-s|%s", aux, ui->shell);
      }
      if (g_strcmp0(u->fullname, ui->fullname) != 0) {
        aux = g_strdup_printf( "%s|-c|\"%s,,,\"", aux, ui->fullname);
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
void manager_user_del (GtkButton *btn, gpointer data)
{
  Manager *m = (Manager *)data;
  Users *us = m->us;
  UsersItem *ui = users_get_current (us);

  if (ui->name != NULL) {
    char *myoutput=NULL, *mystd=NULL;
    int myexit=0;
    GError *err ;
    char *fullcmd = NULL;
    char **myargs =NULL;

    if (!ui->is_new) {
      char cmd[] = "/usr/sbin/userdel";
      fullcmd =g_strdup_printf ("%s|%s", cmd, ui->name);
      myargs = g_strsplit (fullcmd, "|", -1);
      g_print ("This command to exec: %s\n", fullcmd);
    }
  }
}

