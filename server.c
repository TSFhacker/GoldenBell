#include "protocol.h"
#include <arpa/inet.h>
#include <errno.h>
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

playerinfo list[100];
int number_of_info = 0;
playerinfo online_player_list[20];
int online_number = 0;
room roomlist[50];
int room_number = 0;

void sig_chld(int signo);

void broadcast_msg(char *msg, int socket) {
  // printf("%s\n", msg);
  for (int i = 0; i < online_number; i++) {
    if (online_player_list[i].socket == socket)
      continue;
    send(online_player_list[i].socket, msg, strlen(msg), 0);
  }
}

// void broadcast_num(int n, int socket) {
//   for (int i = 0; i < online_number; i++) {
//     if (online_player_list[i].socket == socket)
//       continue;
//     send(online_player_list[i].socket, , strlen(msg), 0);
//   }
// }

void createThread(int conn_sock) {
  while (1) {
    int bytes_sent, bytes_received;
    char username[50];
    char password[50];
    // char last_sign_in_try[50];
    // char signed_in[50];
    // int password_error = 0;
    char result[50];
    bytes_received = recv(conn_sock, username, 50, 0);
    username[bytes_received] = '\0';
    // printf("username: %s\n", username);
    bytes_sent = send(conn_sock, "received", 8, 0);
    // if (bytes_sent <= 0) {
    //   printf("\nConnection closed!\n");
    // }
    bytes_received = recv(conn_sock, password, 50, 0);
    password[bytes_received] = '\0';
    // printf("password: %s\n", password);
    int count = 0;
    for (int i = 0; i < number_of_info; i++) {
      if (strcmp(username, list[i].username) == 0) {
        if (strcmp(password, list[i].password) == 0) {
          strcpy(result, "Hello\0");
          count++;
          strcpy(online_player_list[online_number].username, username);
          online_player_list[online_number].rank = atoi(username);
          online_player_list[online_number].socket = conn_sock;
          online_number++;
        }
      }
    }
    if (count == 0)
      strcpy(result, "Wrong username or password\0");
    bytes_sent = send(conn_sock, result, strlen(result), 0);
    // if (bytes_sent <= 0) {
    //   printf("\nConnection closed!\n");
    // }
    char buff[20];
    if (count > 0) {
      recv(conn_sock, buff, 10, 0);
      // printf("%s\n", buff);
      int converted_number = htonl(room_number);
      bytes_sent =
          write(conn_sock, &converted_number, sizeof(converted_number));
      printf("Sent room number\n");
      for (int i = 0; i < room_number; i++) {
        write(conn_sock, &roomlist[i], sizeof(room));
      }
      printf("Sent room list\n");
      // if (bytes_sent <= 0) {
      //   printf("\nConnection closed!\n");
      // }
    }

    recv(conn_sock, buff, 20, 0);
    if (strcmp(buff, "CREATE_ROOM") == 0) {
      // send(conn_sock, "received", 8, 0);
      bytes_received = recv(conn_sock, buff, 10, 0);
      buff[bytes_received] = '\0';
      printf("%s\n", buff);
      char username[20];
      strcpy(username, buff);
      strcpy(roomlist[room_number].list[0].username, username);
      roomlist[room_number].player_number = 0;
      roomlist[room_number].list[roomlist[room_number].player_number].socket =
          conn_sock;
      roomlist[room_number].player_number++;
      room_number++;
      broadcast_msg("CREATE_ROOM_SUCCESSFULLY", conn_sock);
      // recv(conn_sock, buff, 20, 0);
      // printf("%s\n", buff);
      broadcast_msg(username, conn_sock);

      // int converted_number = htonl(room_number);
      // write(conn_sock, &converted_number, sizeof(converted_number));
    }
  }
}

void main() {
  MYSQL *con = mysql_init(NULL);
  if (con == NULL) {
    fprintf(stderr, "%s\n", mysql_error(con));
    exit(1);
  }

  if (mysql_real_connect(con, "localhost", "root", "root", "golden-bell", 0,
                         NULL, 0) == NULL) {
    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
    exit(1);
  }

  if (mysql_query(con, "SELECT * FROM players")) {
    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
    exit(1);
  }

  MYSQL_RES *result = mysql_store_result(con);
  int num_fields = mysql_num_fields(result);

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result))) {
    unsigned long *lengths;
    lengths = mysql_fetch_lengths(result);
    strcpy(list[number_of_info].username, row[1]);
    strcpy(list[number_of_info].password, row[2]);
    list[number_of_info].rank = atoi(row[3]);
    number_of_info++;
    // for(int i = 0; i < num_fields; i++)
    // {
    //     printf("[%.*s] ", (int) lengths[i], row[i] ? row[i] : "NULL");
    // }
    // printf("\n");
  }
  mysql_close(con);

  int listen_sock, conn_sock;
  struct sockaddr_in server;
  struct sockaddr_in client;
  int sin_size;
  int bytes_sent, bytes_received;

  if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) ==
      -1) { /* calls socket() */
    printf("socket() error\n");
    return;
  }

  bzero(&server, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(5551);
  server.sin_addr.s_addr =
      htonl(INADDR_ANY); /* INADDR_ANY puts your IP address automatically */

  if (bind(listen_sock, (struct sockaddr *)&server, sizeof(server)) == -1) {
    perror("\nError: ");
    return;
  }

  if (listen(listen_sock, 20) == -1) {
    perror("\nError: ");
    return;
  }

  /* Establish a signal handler to catch SIGCHLD */
  signal(SIGCHLD, sig_chld);

  while (1) {
    sin_size = sizeof(struct sockaddr_in);
    if ((conn_sock = accept(listen_sock, (struct sockaddr *)&client,
                            &sin_size)) == -1) {
      if (errno == EINTR)
        continue;
      else {
        perror("\nError: ");
        return;
      }
    }

    pthread_t id;
    pthread_create(&id, NULL, (void *)createThread,
                   (void *)(intptr_t)conn_sock);
  }
  /* The parent closes the connected socket since the child handles the new
   * client */
  close(conn_sock);
  close(listen_sock);
  return;
}

void sig_chld(int signo) {
  pid_t pid;
  int stat;

  /* Wait the child process terminate */
  while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
    printf("\nChild %d terminated\n", pid);
}