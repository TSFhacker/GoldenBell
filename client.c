#include "protocol.h"
#include <arpa/inet.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <mysql.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// set up server
struct sockaddr_in server_addr;
int bytes_sent, bytes_received;
int client_sock;
Data data;

// cua so login
GtkBuilder *builder;
GtkWidget *loginWindow;
GtkWidget *notiLabel1;
GtkWidget *loginfixed;
GtkWidget *usernameEntry;
GtkWidget *passEntry;
GtkWidget *loginBtn;
char username[50];
char password[50];

// main window
GtkWidget *mainwindow;
GtkWidget *logoutBtn;
GtkTreeView *roomview;

// Cua so tao phong
GtkWidget *createroomwindow;
GtkWidget *createroomlabel;
GtkWidget *logoutBtn;
GtkTreeView *playerview;
GtkTreeView *requestview;

void on_usernameEntry_changed(GtkEntry *e);
void on_passEntry_changed(GtkEntry *e);
void on_loginBtn_clicked();

GtkTreeIter curIter;
GtkListStore *store;
int online_number = 0;

GtkTreeModel *create_model() {
  gint i = 0;
  store = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_INT);
  for (i = 0; i < online_number; ++i) {
    gtk_list_store_append(store, &curIter);
    gtk_list_store_set(store, &curIter, 0, 1, 1, online_number, -1);
  }
  return GTK_TREE_MODEL(store);
}

GtkTreeViewColumn *column1, *column2, *column3;
GtkCellRenderer *renderer, *renderer1;

void add_room_columns(GtkTreeView *treeview) {

  GtkTreeModel *model = gtk_tree_view_get_model(treeview);
  renderer = gtk_cell_renderer_text_new();
  column1 = gtk_tree_view_column_new_with_attributes("Rank", renderer, "text",
                                                     0, NULL);
  column2 = gtk_tree_view_column_new_with_attributes("Number of players",
                                                     renderer, "text", 1, NULL);
  gtk_tree_view_append_column(treeview, column1);
  gtk_tree_view_append_column(treeview, column2);
}

int main(int argc, char *argv[]) {

  if (argc < 2) {
    g_print("Usage: ./client ./client.glade\n");
    return 1;
  }
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(5550);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  gtk_init(&argc, &argv);
  builder = gtk_builder_new_from_file(argv[1]);
  loginWindow = GTK_WIDGET(gtk_builder_get_object(builder, "loginWindow"));
  g_signal_connect(loginWindow, "destroy", G_CALLBACK(gtk_main_quit), "NULL");
  gtk_builder_connect_signals(builder, NULL);

  loginfixed = GTK_WIDGET(gtk_builder_get_object(builder, "loginfixed"));
  notiLabel1 = GTK_WIDGET(gtk_builder_get_object(builder, "notiLabel1"));
  passEntry = GTK_WIDGET(gtk_builder_get_object(builder, "passEntry"));
  gtk_entry_set_visibility(GTK_ENTRY(passEntry), FALSE);
  usernameEntry = GTK_WIDGET(gtk_builder_get_object(builder, "usernameEntry"));
  loginBtn = GTK_WIDGET(gtk_builder_get_object(builder, "loginBtn"));

  gtk_widget_set_sensitive(loginBtn, FALSE);

  gtk_widget_show(loginWindow);
  gtk_main();
  return 0;
}

void on_logoutBtn_clicked() {
  gtk_widget_destroy(mainwindow);
  gtk_widget_set_sensitive(loginWindow, TRUE);
}

void on_usernameEntry_changed(GtkEntry *e) {
  gtk_widget_set_sensitive(loginBtn, TRUE);
}

void on_passEntry_changed(GtkEntry *e) {
  gtk_widget_set_sensitive(loginBtn, TRUE);
}

void on_loginBtn_clicked() {
  sprintf(username, "%s", gtk_entry_get_text(GTK_ENTRY(usernameEntry)));
  sprintf(password, "%s", gtk_entry_get_text(GTK_ENTRY(passEntry)));

  client_sock = socket(AF_INET, SOCK_STREAM, 0);

  if (connect(client_sock, (struct sockaddr *)&server_addr,
              sizeof(struct sockaddr)) < 0) {
    printf("\nError!Cannot connect to server! Client exit immediately!\n");
    return;
  }

  bytes_sent = send(client_sock, username, strlen(username), 0);
  if (bytes_sent <= 0) {
    printf("\nConnection closed!\n");
  }
  char buff[10];
  // Doi server nhan duoc username roi moi gui password
  recv(client_sock, buff, 10, 0);
  bytes_sent = send(client_sock, password, strlen(password), 0);
  if (bytes_sent <= 0) {
    printf("\nConnection closed!\n");
  }
  char result[50];
  bytes_received = recv(client_sock, result, 50, 0);
  result[bytes_received] = '\0';

  if (strcmp(result, "Wrong username or password") == 0) {
    gtk_label_set_text(GTK_LABEL(notiLabel1), (const gchar *)result);
  } else {
    bytes_sent = send(client_sock, "ready", 5, 0);
    if (bytes_sent <= 0) {
      printf("\nConnection closed!\n");
    }
    printf("ready\n");
    bytes_received = read(client_sock, &online_number, sizeof(online_number));
    if (bytes_received <= 0) {
      printf("\nConnection closed!\n");
    }
    online_number = ntohl(online_number);
    gtk_label_set_text(GTK_LABEL(notiLabel1),
                       (const gchar *)"Log in successfully");
    builder = gtk_builder_new_from_file("client.glade");
    mainwindow = GTK_WIDGET(gtk_builder_get_object(builder, "mainwindow"));
    g_signal_connect(mainwindow, "destroy", G_CALLBACK(on_logoutBtn_clicked),
                     "NULL");
    gtk_builder_connect_signals(builder, NULL);
    logoutBtn = GTK_WIDGET(gtk_builder_get_object(builder, "logoutBtn"));
    g_signal_connect(logoutBtn, "clicked", G_CALLBACK(on_logoutBtn_clicked),
                     "NULL");
    roomview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "roomview"));

    if (online_number > 0) {
      gtk_tree_view_set_model(GTK_TREE_VIEW(roomview),
                              GTK_TREE_MODEL(create_model()));
      add_room_columns(GTK_TREE_VIEW(roomview));
    }

    gtk_widget_set_sensitive(loginWindow, FALSE);
    gtk_widget_show(mainwindow);
  }
}

void on_createroomBtn_clicked() {
  if (bytes_sent <= 0) {
    printf("\nConnection closed!\n");
  }
  builder = gtk_builder_new_from_file("client.glade");
  createroomwindow =
      GTK_WIDGET(gtk_builder_get_object(builder, "createroomwindow"));
  createroomlabel =
      GTK_WIDGET(gtk_builder_get_object(builder, "createroomlabel"));
  char *roomname;
  strcpy(roomname, username);
  strcat(roomname, "'s room");
  gtk_label_set_text(GTK_LABEL(createroomlabel), (const gchar *)roomname);
  g_signal_connect(mainwindow, "destroy", G_CALLBACK(on_logoutBtn_clicked),
                   "NULL");
  gtk_builder_connect_signals(builder, NULL);
  logoutBtn = GTK_WIDGET(gtk_builder_get_object(builder, "logoutBtn"));
  //   g_signal_connect(logoutBtn, "clicked", G_CALLBACK(on_logoutBtn_clicked),
  //                    "NULL");
  playerview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "playerview"));
  requestview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "requestview"));

  gtk_widget_set_sensitive(mainwindow, FALSE);
  gtk_widget_show(createroomwindow);
}

void on_createroomwindow_destroy() {
  gtk_widget_set_sensitive(mainwindow, TRUE);
}