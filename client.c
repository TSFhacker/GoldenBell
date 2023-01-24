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

// cua so login
GtkBuilder *builder;
GtkWidget *loginWindow;
GtkWidget *notiLabel1;
GtkWidget *usernameEntry;
GtkWidget *passEntry;
GtkWidget *loginBtn;
char username[20];
char password[20];
gchar *host;
gchar *joining_player;

// main window
GtkWidget *mainwindow;
GtkWidget *logoutBtn;
GtkTreeView *roomview;

// Cua so tao phong
GtkWidget *createroomwindow;
GtkWidget *createroomlabel;
GtkWidget *startBtn;
GtkWidget *waitinglistlabel;
GtkTreeView *playerview;
GtkTreeView *requestview;

// Cua so pop up doi chu phong chap nhan
GtkWidget *waitingwindow;
GtkWidget *cancelwaitingBtn;

// Cua so pop up yeu cau vao phong
GtkWidget *joinrequestdialog;
GtkWidget *joininfolabel;
GtkWidget *acceptjoinBtn;
GtkWidget *refusejoinBtn;

// cua so doi chu phong bat dau game
GtkWidget *memberwindow;

// man hinh cau hoi
GtkWidget *quizwindow;
GtkWidget *questionlabel;
GtkWidget *timerlabel;
GtkWidget *ABtn;
GtkWidget *BBtn;
GtkWidget *CBtn;

// man hinh sau khi tra loi sai
GtkWidget *loserwindow;

// man hinh ket qua
GtkWidget *resultwindow;
GtkTreeView *resultview;
GtkWidget *endgameBtn;

void on_usernameEntry_changed(GtkEntry *e);
void on_passEntry_changed(GtkEntry *e);
void on_loginBtn_clicked();
void on_cancelwaitingBtn_clicked();
void on_ABtn_clicked();
void on_BBtn_clicked();
void on_CBtn_clicked();
void on_endgameBtn_clicked();

GtkTreeIter curIter;
GtkTreeIter playerIter;
GtkListStore *store;

GtkTreeModel *create_model() {
  gint i = 0;
  store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
  for (i = 0; i < room_number; ++i) {
    // printf("room number %d\n", i);
    gtk_list_store_append(store, &curIter);
    gtk_list_store_set(store, &curIter, 0, roomlist[i].list[0].username, 1,
                       roomlist[i].rank, 2, roomlist[i].player_number, -1);
  }
  // printf("Done with the setting\n");
  return GTK_TREE_MODEL(store);
}

GtkTreeViewColumn *column1, *column2, *column3;
GtkCellRenderer *renderer, *renderer1;

void add_room_columns(GtkTreeView *treeview) {
  renderer = gtk_cell_renderer_text_new();
  column1 = gtk_tree_view_column_new_with_attributes("Host", renderer, "text",
                                                     0, NULL);
  column2 = gtk_tree_view_column_new_with_attributes("Rank", renderer, "text",
                                                     1, NULL);
  column3 = gtk_tree_view_column_new_with_attributes("Players", renderer,
                                                     "text", 2, NULL);
  gtk_tree_view_append_column(treeview, column1);
  gtk_tree_view_append_column(treeview, column2);
  gtk_tree_view_append_column(treeview, column3);
}

int continue_timer = 1;
int s, ms;
// tao mot luong de nghe tin nhan tu server

void timer(int socket) {
  pthread_detach(pthread_self());
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 1000000;
  s = 20;
  ms = 0;
  char time[5];
  while (1) {
    if (s == 0 && ms == 0) {
      if (continue_timer) {
        if (sendData(socket, "timeout") == 0)
          printf("error\n");
        int converted_number = htonl(current_question);
        write(client_sock, &converted_number, sizeof(converted_number));
      }
      break;
    }
    if (ms == 0)
      ms = 1000;
    if (ms == 999) {
      s--;
      sprintf(time, "%d", s);
      gtk_label_set_text(GTK_LABEL(timerlabel), (const gchar *)time);
    }
    ms--;
    nanosleep(&ts, &ts);
    // printf("\r %d %d\n", s, ms);
  }
  pthread_exit(NULL);
}
void listenAndPrint(int socket) {
  pthread_detach(pthread_self());
  char buffer[100];
  while (1) {
    if (receiveData(socket, buffer) == 0)
      break;
    printf("%s\n", buffer);
    if (strcmp(buffer, "CREATE_ROOM_SUCCESSFULLY") == 0) {
      printf("create successfully\n");
      playerinfo player;
      read(client_sock, &player, sizeof(player));
      printf("increasing room: %s\n", player.username);
      createRoom(player, room_number);
      gtk_tree_view_set_model(GTK_TREE_VIEW(roomview),
                              GTK_TREE_MODEL(create_model()));
      if (strcmp(username, player.username) == 0) {
        printf("You are the host\n");
        renderer = gtk_cell_renderer_text_new();
        column1 = gtk_tree_view_column_new_with_attributes("Username", renderer,
                                                           "text", 0, NULL);
        column2 = gtk_tree_view_column_new_with_attributes("Rank", renderer,
                                                           "text", 1, NULL);

        gtk_tree_view_append_column(playerview, column1);
        gtk_tree_view_append_column(playerview, column2);
        renderer = gtk_cell_renderer_text_new();
        column1 = gtk_tree_view_column_new_with_attributes("Username", renderer,
                                                           "text", 0, NULL);
        column2 = gtk_tree_view_column_new_with_attributes("Rank", renderer,
                                                           "text", 1, NULL);
        gtk_tree_view_append_column(requestview, column1);
        gtk_tree_view_append_column(requestview, column2);
        store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
        for (int i = 0; i < roomlist[findRoomByHost(username)].player_number;
             ++i) {
          gtk_list_store_append(store, &curIter);
          gtk_list_store_set(
              store, &curIter, 0,
              roomlist[findRoomByHost(username)].list[i].username, 1,
              roomlist[findRoomByHost(username)].list[i].rank, -1);
        }
        gtk_tree_view_set_model(GTK_TREE_VIEW(playerview),
                                GTK_TREE_MODEL(store));
      }

    } else if (strcmp(buffer, "DELETE_ROOM_SUCCESSFULLY") == 0) {
      if (receiveData(socket, buffer) == 0)
        break;
      deleteRoom(buffer);
      // printf("Current number of rooms: %d\n", room_number);
      gtk_tree_view_set_model(GTK_TREE_VIEW(roomview),
                              GTK_TREE_MODEL(create_model()));
    } else if (strcmp(buffer, "JOIN_ROOM") == 0) {
      playerinfo player;
      read(client_sock, &player, sizeof(player));
      printf("%s wants to join your room\n", player.username);

      addPlayerToWaitingList(player, findRoomByHost(username));

      store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
      for (int i = 0; i < roomlist[findRoomByHost(username)].waiting_number;
           ++i) {
        gtk_list_store_append(store, &curIter);
        gtk_list_store_set(
            store, &curIter, 0,
            roomlist[findRoomByHost(username)].waiting_list[i].username, 1,
            roomlist[findRoomByHost(username)].waiting_list[i].rank, -1);
      }
      gtk_tree_view_set_model(GTK_TREE_VIEW(requestview),
                              GTK_TREE_MODEL(store));
    } else if (strcmp(buffer, "CANCEL_JOIN_ROOM") == 0) {
      if (receiveData(socket, buffer) == 0)
        break;
      removePlayerFromWaitingList(buffer, findRoomByHost(username));
      store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
      for (int i = 0; i < roomlist[findRoomByHost(username)].waiting_number;
           ++i) {
        gtk_list_store_append(store, &curIter);
        gtk_list_store_set(
            store, &curIter, 0,
            roomlist[findRoomByHost(username)].waiting_list[i].username, 1,
            roomlist[findRoomByHost(username)].waiting_list[i].rank, -1);
      }
      gtk_tree_view_set_model(GTK_TREE_VIEW(requestview),
                              GTK_TREE_MODEL(store));
    } else if (strcmp(buffer, "ACCEPT_JOIN_ROOM_SUCCESSFULLY") == 0) {
      playerinfo join, invite;
      // join = (playerinfo *)malloc(sizeof(playerinfo));
      // invite = (playerinfo *)malloc(sizeof(playerinfo));
      read(client_sock, &join, sizeof(join));
      read(client_sock, &invite, sizeof(invite));
      addPlayerToRoom(join, findRoomByHost(invite.username));
      removePlayerFromWaitingList(join.username,
                                  findRoomByHost(invite.username));

      gtk_tree_view_set_model(GTK_TREE_VIEW(roomview),
                              GTK_TREE_MODEL(create_model()));
      if (strcmp(username, invite.username) == 0) {

        store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
        for (int i = 0; i < roomlist[findRoomByHost(username)].player_number;
             ++i) {
          gtk_list_store_append(store, &curIter);
          gtk_list_store_set(
              store, &curIter, 0,
              roomlist[findRoomByHost(username)].list[i].username, 1,
              roomlist[findRoomByHost(username)].list[i].rank, -1);
        }
        gtk_tree_view_set_model(GTK_TREE_VIEW(playerview),
                                GTK_TREE_MODEL(store));
        store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
        for (int i = 0; i < roomlist[findRoomByHost(username)].waiting_number;
             ++i) {
          gtk_list_store_append(store, &curIter);
          gtk_list_store_set(
              store, &curIter, 0,
              roomlist[findRoomByHost(username)].waiting_list[i].username, 1,
              roomlist[findRoomByHost(username)].waiting_list[i].rank, -1);
        }
        gtk_tree_view_set_model(GTK_TREE_VIEW(requestview),
                                GTK_TREE_MODEL(store));
        // gtk_widget_show(loginWindow);
      } else if (strcmp(username, join.username) == 0) {
        gtk_widget_destroy(waitingwindow);
        gtk_widget_set_sensitive(mainwindow, FALSE);
        builder = gtk_builder_new_from_file("client.glade");
        memberwindow =
            GTK_WIDGET(gtk_builder_get_object(builder, "memberwindow"));
        gtk_widget_show(memberwindow);
      }
    } else if (strcmp(buffer, "REFUSE_JOIN_ROOM_SUCCESSFULLY") == 0) {
      if (receiveData(socket, buffer) == 0)
        break;
      removePlayerFromWaitingList(buffer, findRoomByHost(username));
      gtk_widget_destroy(waitingwindow);
      gtk_widget_set_sensitive(mainwindow, TRUE);
    } else if (strcmp(buffer, "START_GAME_SUCCESSFULLY") == 0) {
      gtk_widget_hide(mainwindow);
      gtk_widget_destroy(memberwindow);
      gtk_widget_hide(createroomwindow);
      read(client_sock, &questions[0], sizeof(question));
      builder = gtk_builder_new_from_file("client.glade");
      quizwindow = GTK_WIDGET(gtk_builder_get_object(builder, "quizwindow"));
      questionlabel =
          GTK_WIDGET(gtk_builder_get_object(builder, "questionlabel"));
      timerlabel = GTK_WIDGET(gtk_builder_get_object(builder, "timerlabel"));
      ABtn = GTK_WIDGET(gtk_builder_get_object(builder, "ABtn"));
      g_signal_connect(ABtn, "clicked", G_CALLBACK(on_ABtn_clicked), "NULL");
      BBtn = GTK_WIDGET(gtk_builder_get_object(builder, "BBtn"));
      g_signal_connect(BBtn, "clicked", G_CALLBACK(on_BBtn_clicked), "NULL");
      CBtn = GTK_WIDGET(gtk_builder_get_object(builder, "CBtn"));
      g_signal_connect(CBtn, "clicked", G_CALLBACK(on_CBtn_clicked), "NULL");
      char content[500];
      // sprintf(content, "%s\nA: %s\nB: %s\nC: %s", questions[0].content,
      //         questions[0].answer[0], questions[0].answer[1],
      //         questions[0].answer[2]);
      gtk_label_set_text(GTK_LABEL(questionlabel),
                         (const gchar *)questions[0].content);
      gtk_label_set_text(GTK_LABEL(timerlabel), (const gchar *)"20");
      gtk_button_set_label(GTK_BUTTON(ABtn),
                           (const gchar *)questions[0].answer[0]);
      gtk_button_set_label(GTK_BUTTON(BBtn),
                           (const gchar *)questions[0].answer[1]);
      gtk_button_set_label(GTK_BUTTON(CBtn),
                           (const gchar *)questions[0].answer[2]);

      gtk_widget_show(quizwindow);
      if (sendData(socket, "READY_TO_ANSWER") == 0)
        printf("Error\n");
      pthread_t id;
      pthread_create(&id, NULL, (void *)timer, (void *)(intptr_t)client_sock);
    } else if (strcmp(buffer, "CORRECT_ANSWER") == 0) {
      continue_timer = 1;
      gtk_label_set_text(GTK_LABEL(timerlabel), (const gchar *)"20");
      printf("Correct answer\n");
      current_question++;
      gtk_widget_set_sensitive(ABtn, TRUE);
      gtk_widget_set_sensitive(BBtn, TRUE);
      gtk_widget_set_sensitive(CBtn, TRUE);
      read(client_sock, &questions[current_question], sizeof(question));
      gtk_label_set_text(GTK_LABEL(questionlabel),
                         (const gchar *)questions[current_question].content);
      gtk_button_set_label(
          GTK_BUTTON(ABtn),
          (const gchar *)questions[current_question].answer[0]);
      gtk_button_set_label(
          GTK_BUTTON(BBtn),
          (const gchar *)questions[current_question].answer[1]);
      gtk_button_set_label(
          GTK_BUTTON(CBtn),
          (const gchar *)questions[current_question].answer[2]);
      if (sendData(client_sock, "READY_TO_ANSWER") == 0)
        printf("Error\n");
      pthread_t id;
      pthread_create(&id, NULL, (void *)timer, (void *)(intptr_t)client_sock);
    } else if (strcmp(buffer, "WRONG_ANSWER") == 0) {
      gtk_widget_destroy(quizwindow);
      builder = gtk_builder_new_from_file("client.glade");
      loserwindow = GTK_WIDGET(gtk_builder_get_object(builder, "loserwindow"));
      // gtk_tree_view_set_model(GTK_TREE_VIEW(roomview),
      //                         GTK_TREE_MODEL(create_model()));
      gtk_widget_show(loserwindow);
    } else if (strcmp(buffer, "END_GAME") == 0) {
      room a;
      read(socket, &a, sizeof(room));
      gtk_widget_destroy(loserwindow);
      builder = gtk_builder_new_from_file("client.glade");
      resultwindow =
          GTK_WIDGET(gtk_builder_get_object(builder, "resultwindow"));
      resultview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "resultview"));
      endgameBtn = GTK_WIDGET(gtk_builder_get_object(builder, "endgameBtn"));
      g_signal_connect(endgameBtn, "clicked", G_CALLBACK(on_endgameBtn_clicked),
                       "NULL");
      renderer = gtk_cell_renderer_text_new();
      column1 = gtk_tree_view_column_new_with_attributes("Username", renderer,
                                                         "text", 0, NULL);
      column2 = gtk_tree_view_column_new_with_attributes("Rank", renderer,
                                                         "text", 1, NULL);
      column3 = gtk_tree_view_column_new_with_attributes(
          "Correct answers", renderer, "text", 2, NULL);
      gtk_tree_view_append_column(resultview, column1);
      gtk_tree_view_append_column(resultview, column2);
      gtk_tree_view_append_column(resultview, column3);
      store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
      printf("%d\n", a.player_number);
      for (int i = 0; i < a.player_number; ++i) {
        // printf("room number %d\n", i);
        printf("%s %d %d\n", a.list[i].username, a.list[i].rank,
               a.list[i].correct);
        gtk_list_store_append(store, &curIter);
        gtk_list_store_set(store, &curIter, 0, a.list[i].username, 1,
                           a.list[i].rank, 2, a.list[i].correct, -1);
      }
      // gtk_widget_destroy(createroomwindow);
      gtk_tree_view_set_model(GTK_TREE_VIEW(resultview), GTK_TREE_MODEL(store));
      gtk_widget_show(resultwindow);
    } else
      pthread_exit(NULL);
  }

  close(socket);
}

void exit_main() {
  gtk_main_quit();
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

  if (argc < 2) {
    g_print("Usage: ./client ./client.glade\n");
    return 1;
  }
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(5551);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  gtk_init(&argc, &argv);
  builder = gtk_builder_new_from_file(argv[1]);
  loginWindow = GTK_WIDGET(gtk_builder_get_object(builder, "loginWindow"));
  g_signal_connect(loginWindow, "destroy", G_CALLBACK(exit_main), "NULL");
  gtk_builder_connect_signals(builder, NULL);

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
  gtk_widget_show(loginWindow);
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

  // printf("%s\n", username);
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
    gtk_widget_hide(loginWindow);
    bytes_sent = send(client_sock, "ready", 5, 0);
    if (bytes_sent <= 0) {
      printf("\nConnection closed!\n");
    }
    printf("ready\n");
    bytes_received = read(client_sock, &room_number, sizeof(room_number));
    if (bytes_received <= 0) {
      printf("\nConnection closed!\n");
    }

    room_number = ntohl(room_number);
    printf("There are %d rooms\n", room_number);

    for (int i = 0; i < room_number; i++) {
      bytes_received = read(client_sock, &roomlist[i], sizeof(room));
      if (bytes_received <= 0) {
        printf("\nConnection closed!\n");
      }
      // printf("received room info\n");
      // printf("%s\n", roomlist[i].list[0].username);
      // printf("%d\n", roomlist[i].player_number);
    }

    printf("ready2\n");
    pthread_t id;
    pthread_create(&id, NULL, (void *)listenAndPrint,
                   (void *)(intptr_t)client_sock);

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

    printf("showed room info\n");
    add_room_columns(GTK_TREE_VIEW(roomview));
    if (room_number > 0) {
      gtk_tree_view_set_model(GTK_TREE_VIEW(roomview),
                              GTK_TREE_MODEL(create_model()));
    }

    gtk_widget_set_sensitive(loginWindow, FALSE);
    gtk_widget_show(mainwindow);
  }
}

void on_createroomwindow_destroy() {
  gtk_widget_set_sensitive(mainwindow, TRUE);
  printf("Destroyed\n");
  if (sendData(client_sock, "DELETE_ROOM") == 0)
    printf("Error\n");
  if (sendData(client_sock, username) == 0)
    printf("Error\n");
}
void on_createroomBtn_clicked() {
  if (sendData(client_sock, "CREATE_ROOM") == 0)
    printf("Error\n");
  if (bytes_sent <= 0) {
    printf("\nConnection closed!\n");
  }
  char buffer[20];
  sendData(client_sock, username);
  builder = gtk_builder_new_from_file("client.glade");
  createroomwindow =
      GTK_WIDGET(gtk_builder_get_object(builder, "createroomwindow"));
  createroomlabel =
      GTK_WIDGET(gtk_builder_get_object(builder, "createroomlabel"));
  // printf("done with the creating 1\n");
  char roomname[20];
  strcpy(roomname, username);
  // printf("done with the creating 2\n");
  strcat(roomname, "'s room");
  gtk_label_set_text(GTK_LABEL(createroomlabel), (const gchar *)roomname);
  gtk_builder_connect_signals(builder, NULL);
  playerview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "playerview"));
  requestview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "requestview"));

  gtk_widget_set_sensitive(mainwindow, FALSE);
  gtk_widget_show(createroomwindow);
}

void on_cancelwaitingBtn_clicked() {
  gtk_widget_destroy(waitingwindow);
  if (sendData(client_sock, "CANCEL_JOIN_ROOM") == 0)
    printf("Error\n");
  if (sendData(client_sock, username) == 0)
    printf("Error\n");
  if (sendData(client_sock, (char *)host) == 0)
    printf("Error\n");
  gtk_widget_set_sensitive(mainwindow, TRUE);
}

void on_roomview_row_activated(GtkTreeView *view, GtkTreePath *path,
                               GtkTreeViewColumn *col, gpointer user_data) {
  GtkTreeModel *model;
  GtkTreeIter iter;

  model = gtk_tree_view_get_model(view);

  if (!gtk_tree_model_get_iter(model, &iter, path))
    return;
  gtk_tree_model_get(model, &iter, 0, &host, -1);

  printf("%s\n", host);
  if (sendData(client_sock, "JOIN_ROOM") == 0)
    printf("Error\n");
  if (sendData(client_sock, (char *)host) == 0)
    printf("Error\n");
  if (sendData(client_sock, username) == 0)
    printf("Error\n");
  builder = gtk_builder_new_from_file("client.glade");
  waitingwindow = GTK_WIDGET(gtk_builder_get_object(builder, "waitingwindow"));
  cancelwaitingBtn =
      GTK_WIDGET(gtk_builder_get_object(builder, "cancelwaitingBtn"));
  g_signal_connect(cancelwaitingBtn, "clicked",
                   G_CALLBACK(on_cancelwaitingBtn_clicked), "NULL");
  gtk_widget_show(waitingwindow);
  gtk_widget_set_sensitive(mainwindow, FALSE);
}

void on_acceptjoinBtn_clicked() {
  if (sendData(client_sock, "ACCEPT_JOIN_ROOM") == 0)
    printf("Error\n");
  if (sendData(client_sock, (char *)joining_player) == 0)
    printf("Error\n");
  if (sendData(client_sock, username) == 0)
    printf("Error\n");
  gtk_widget_destroy(joinrequestdialog);
}

void on_refusejoinBtn_clicked() {
  if (sendData(client_sock, "REFUSE_JOIN_ROOM") == 0)
    printf("Error\n");
  if (sendData(client_sock, (char *)joining_player) == 0)
    printf("Error\n");
  if (sendData(client_sock, username) == 0)
    printf("Error\n");
  gtk_widget_destroy(joinrequestdialog);
  removePlayerFromWaitingList(joining_player, findRoomByHost(username));
  store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
  for (int i = 0; i < roomlist[findRoomByHost(username)].waiting_number; ++i) {
    printf("There are %d people waiting\n",
           roomlist[findRoomByHost(username)].waiting_number);
    gtk_list_store_append(store, &curIter);
    gtk_list_store_set(
        store, &curIter, 0,
        roomlist[findRoomByHost(username)].waiting_list[i].username, 1,
        roomlist[findRoomByHost(username)].waiting_list[i].rank, -1);
  }
  gtk_tree_view_set_model(GTK_TREE_VIEW(requestview), GTK_TREE_MODEL(store));
}

void on_requestview_row_activated(GtkTreeView *view, GtkTreePath *path,
                                  GtkTreeViewColumn *col, gpointer user_data) {
  GtkTreeModel *model;
  GtkTreeIter iter;

  model = gtk_tree_view_get_model(view);

  if (!gtk_tree_model_get_iter(model, &iter, path))
    return;
  int *rank = (int *)malloc(sizeof(int));
  char *join;
  gtk_tree_model_get(model, &iter, 0, &joining_player, 1, rank, -1);
  printf("%d\n", *rank);
  builder = gtk_builder_new_from_file("client.glade");
  joinrequestdialog =
      GTK_WIDGET(gtk_builder_get_object(builder, "joinrequestdialog"));
  g_signal_connect(loginWindow, "destroy", G_CALLBACK(gtk_main_quit), "NULL");
  gtk_builder_connect_signals(builder, NULL);
  joininfolabel = GTK_WIDGET(gtk_builder_get_object(builder, "joininfolabel"));
  acceptjoinBtn = GTK_WIDGET(gtk_builder_get_object(builder, "acceptjoinBtn"));
  refusejoinBtn = GTK_WIDGET(gtk_builder_get_object(builder, "refusejoinBtn"));
  char info[100];
  sprintf(info, "%s (Rank :%d) wants to join your room", joining_player, *rank);
  gtk_label_set_text(GTK_LABEL(joininfolabel), (const gchar *)info);
  free(rank);
  gtk_widget_show(joinrequestdialog);
}
void on_startBtn_clicked() {
  printf("Starting the game\n");
  if (sendData(client_sock, "START_GAME") == 0)
    printf("Error\n");
  if (sendData(client_sock, username) == 0)
    printf("Error\n");
}

void on_ABtn_clicked() {
  // printf("Pick A\n");
  continue_timer = 0;
  char time[6];
  sprintf(time, "%d.%d", s, ms);
  if (sendData(client_sock, "PickA") == 0)
    printf("Error\n");
  if (sendData(client_sock, time) == 0)
    printf("Error\n");
  int converted_number = htonl(current_question);
  write(client_sock, &converted_number, sizeof(converted_number));
  gtk_widget_set_sensitive(ABtn, FALSE);
  gtk_widget_set_sensitive(BBtn, FALSE);
  gtk_widget_set_sensitive(CBtn, FALSE);
}

void on_BBtn_clicked() {
  continue_timer = 0;
  if (sendData(client_sock, "PickB") == 0)
    printf("Error\n");
  char time[6];
  sprintf(time, "%d.%d", s, ms);
  if (sendData(client_sock, time) == 0)
    printf("Error\n");
  int converted_number = htonl(current_question);
  write(client_sock, &converted_number, sizeof(converted_number));
  gtk_widget_set_sensitive(ABtn, FALSE);
  gtk_widget_set_sensitive(BBtn, FALSE);
  gtk_widget_set_sensitive(CBtn, FALSE);
}

void on_CBtn_clicked() {
  continue_timer = 0;
  if (sendData(client_sock, "PickC") == 0)
    printf("Error\n");
  char time[6];
  sprintf(time, "%d.%d", s, ms);
  if (sendData(client_sock, time) == 0)
    printf("Error\n");
  int converted_number = htonl(current_question);
  write(client_sock, &converted_number, sizeof(converted_number));
  gtk_widget_set_sensitive(ABtn, FALSE);
  gtk_widget_set_sensitive(BBtn, FALSE);
  gtk_widget_set_sensitive(CBtn, FALSE);
}

void on_endgameBtn_clicked() {
  gtk_widget_destroy(createroomwindow);
  gtk_widget_destroy(resultwindow);
  gtk_tree_view_set_model(GTK_TREE_VIEW(roomview),
                          GTK_TREE_MODEL(create_model()));
  gtk_widget_show(mainwindow);
  gtk_widget_set_sensitive(mainwindow, TRUE);
}