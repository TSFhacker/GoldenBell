#ifndef __PROTOCOL__
#define __PROTOCOL__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct player {
  char username[50];
  char password[50];
  int rank;
  int socket;
} playerinfo;

typedef struct room {
  playerinfo list[20];
  int player_number;
  int rank;
  int id;
  int state; // 0 la dang choi, 1 la chua choi
} room;

typedef struct data {
  int i;
  char c;
  char *s;
} Data;

typedef enum {
  LOGIN,    //đăng nhập
  REGISTER, //đăng ký
  LOGOUT,   // đăng xuất
  ANSWER,   // trả lời
  SHOW_ROOM,
  JOIN_ROOM,   // tham gia phòng có sẵn
  CREATE_ROOM, // tạo phòng
  START_GAME,  // bắt đầu trò chơi
  EXIT_ROOM,   // thoát phòng
  EXIT_GAME    // thoát trò chơi
} CLIENT_CODE;

typedef enum {
  LOGIN_SUCCESSFULLY,       //đăng nhập thành công
  LOGIN_UNSUCCESSFULLY,     //đăng nhập không thành công
  ACCOUNT_IN_USE,           // tài khoản đang được sử dụng
  LOGOUT_SUCCESSFULLY,      //Đăng xuất thành công
  REGISTER_SUCCESSFULLY,    //Đăng ký thành công
  USERNAME_EXISTED,         // Tên tài khoản đã tồn tại
  ANSWER_IS_CORRECT,        // Câu trả lời đúng
  ANSWER_IS_INCORRECT,      // Câu trả lời sai
  JOIN_ROOM_SUCCESSFULLY,   // Tham gia phòng thành công
  CREATE_ROOM_SUCCESSFULLY, // Tạo phòng thành công
  GAME_READY,               // Game sẵn sàng bắt đầu
  GAME_NOT_READY,           // Game chưa sẵn sàng bắt đầu
  GAME_END,                 // Game kết thúc
  EXIT_ROOM_SUCESSFULLY,    // Thoát phòng thành công
  EXIT_GAME_SUCCESSFULLY    // Thoát game thành công
} SERVER_CODE;

typedef struct {
  CLIENT_CODE code;
  char message[50];
  Data data[10];
} Request;

typedef struct {
  SERVER_CODE code;
  char message[50];
  Data data[10];
} Response;

// core function
int receiveData(int socket, Request *buff, int size, int flags);
int sendData(int socket, Request *buff, int size, int flags);

// int sendMessage(int socket, Response *msg, int size, int flags);
// int receiveMessage(int socket, Response *msg, int size, int flags);

// // set message response
// void setMessageResponse(Response *msg);
// void readMessageResponse(Response *msg);

// //set opcode request
// void setOpcodeRequest(Request *request, char *input);

// //send question
// int sendQuestion(int socket, Question *question, int size, int flags);
// int receiveQuestion(int socket, Question *question, int size, int flags);
// //send information
// int sendInformation(int socket, Information *infor, int size, int flags);
// int receiveInformation(int socket, Information *infor, int size, int flags);
// //request get
// void requestGet(int socket);
// void requestLogout(int socket, char *username);
// void requestCheckInformation(int socket);
// void requestGetHelp(int socket);
#endif