#include "protocol.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int online_number = 0;
int room_number = 0;
int number_of_info = 0;
room roomlist[50];
playerinfo list[100];
playerinfo online_player_list[20];

int receiveData(int socket, char *buffer) {
  int n;
  int bytes_received;
  bytes_received = read(socket, &n, sizeof(n));
  if (bytes_received <= 0) {
    printf("\nConnection closed!\n");
    return 0;
  }
  n = ntohl(n);
  bytes_received = recv(socket, buffer, n, 0);
  buffer[bytes_received] = '\0';
  if (bytes_received <= 0) {
    printf("\nConnection closed!\n");
    return 0;
  }
  return 1;
}

int sendData(int socket, char *msg) {
  int bytes_sent;
  int n = htonl(strlen(msg));
  bytes_sent = write(socket, &n, sizeof(n));
  if (bytes_sent <= 0) {
    printf("\nConnection closed!\n");
    return 0;
  }
  bytes_sent = send(socket, msg, strlen(msg), 0);
  if (bytes_sent <= 0) {
    printf("\nConnection closed!\n");
    return 0;
  }
  return 1;
}

int findUser(char *username) {
  for (int i = 0; i < online_number; i++) {
    if (strcmp(online_player_list[i].username, username) == 0) {
      return i;
    }
  }
  return -1; // khong tim duoc user
}

void createRoom(playerinfo player, int roomnumber) {
  strcpy(roomlist[room_number].list[0].username, player.username);
  strcpy(roomlist[room_number].list[0].password, player.password);
  roomlist[room_number].list[0].rank = player.rank;
  roomlist[room_number].list[0].socket = player.socket;
  roomlist[room_number].rank = player.rank;
  roomlist[room_number].state = 0;
  roomlist[room_number].player_number = 1;
  room_number++;
}

void addPlayerToRoom(playerinfo player, int roomnumber) {
  strcpy(
      roomlist[room_number].list[roomlist[room_number].player_number].username,
      player.username);
  strcpy(
      roomlist[room_number].list[roomlist[room_number].player_number].password,
      player.password);
  roomlist[room_number].list[roomlist[room_number].player_number].rank =
      player.rank;
  roomlist[room_number].list[roomlist[room_number].player_number].socket =
      player.socket;
  roomlist[room_number].player_number++;
}