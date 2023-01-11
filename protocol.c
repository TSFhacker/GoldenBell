#include "protocol.h"

int receiveRequest(int socket, Request *buff, int size, int flags)
{
  int n;

  n = recv(socket, buff, size, flags);
  if (n < 0)
    perror("Error: ");
  return n;
}

int sendRequest(int socket, Request *buff, int size, int flags)
{
  int n;

  n = send(socket, buff, size, flags);
  if (n < 0)
    perror("Error: ");
  return n;
}

int sendResponse(int socket, Response *msg, int size, int flags)
{
  int n;
  n = send(socket, msg, size, flags);
  if (n < 0)
    perror("Error: ");
  return n;
}

int receiveResponse(int socket, Response *msg, int size, int flags)
{
  int n;
  n = recv(socket, msg, size, flags);
  if (n < 0)
    perror("Error: ");
  return n;
}

void setMessageResponse(Response *msg)
{
  if (msg->code != NULL)
  {
    switch (msg->code)
    {
    case SYNTAX_ERROR:
      strcpy(msg->message, "Syntax error "); // sai cu phap
      break;
    case INVALID_OPERATION:
      strcpy(msg->message, "Invalid operation "); 
      break;
    case USER_NAME_FOUND:
      strcpy(msg->message, "Username is correct "); // tai khoan sai
      break;
    case USER_NAME_NOT_FOUND:
      strcpy(msg->message, "Cannot find account ");// tai khoan k tim thay
      break;
    case USER_NAME_BLOCKED:
      strcpy(msg->message, "Account is blocked "); // tai khoan bi khoa
      break;
    //case USER_NAME_IS_SIGNIN:
      //strcpy(msg->message, "Login only one account ");
      //break;
    case PASSWORD_CORRECT:
      strcpy(msg->message, "Login successful "); // dang nhap thanh cong
      break;
    case PASSWORD_INCORRECT:
      strcpy(msg->message, "Password incorrect "); // pass sai
      break;
    case PASSWORD_INCORRECT_THREE_TIMES:
      strcpy(msg->message, "Password is incorrect. Account is blocked "); // nhap 3 lan sai -> khoa tai khoan
      break;
    case LOGOUT_SUCCESS:
      strcpy(msg->message, "Logout successful "); // thoat thanh cong
      break;
    case REGISTER_SUCCESSFULL:
      strcpy(msg->message, "Register successfull "); // dang ki thanh cong
      break;
    case REGISTER_USERNAME_EXISTED:
      strcpy(msg->message, "Username is existed "); // tai khoan da dc dang ki
      break;
    case PASSWORD_CORRECT_BUT_ACCOUNT_IS_SIGNINED_IN_ORTHER_CLIENT:
      strcpy(msg->message, "Account is signin in orhter client ");// tai khhoan dc dang nhap o client khac
      break;
    case ANSWER_IS_CORRECT:
      strcpy(msg->message, "The answer is correct ");// cau tra loi dung
      break;
    case ANSWER_IS_INCORRECT:
      strcpy(msg->message, "The answer is incorrect \nEnd game"); // cau tra loi sai -> quit game
      break;
    case ANSWER_IS_INVALID:
      strcpy(msg->message, "The answer is invalid ");// cau tra loi k ton tai
      break;
    case TOPIC_USER_CHOOSE_LEVEL:
      strcpy(msg->message, ""); // chon level
      break;
    case TOPIC_TYPE_INVALID:
      strcpy(msg->message, "User choose level is invalid "); // level k ton tai
      break;
    case INFORMATION_SUCCESS:
      strcpy(msg->message, "");
      break;
    case INFORMATION_ORTHER_PLAYER_ANSWERING:
      strcpy(msg->message, "");
      break;
    case GAME_READY:
      strcpy(msg->message, "Game ready "); // game san sang
      break;
    case GAME_NOT_READY:
      strcpy(msg->message, "Waiting orther player... "); // cho nguoi choi khac
      break;
    case TOPIC_USER_DONT_CHOOSE_LEVEL:
      strcpy(msg->message, "");
      break;
    case GAME_END_WIN:
      strcpy(msg->message, "End game.\nYou win ");
      break;
    case GAME_IS_PLAYING_DONT_LOG_IN:
      strcpy(msg->message, "\nGame is playing!! You can't login \n");
      break;
    default:
      strcpy(msg->message, "Exception ");
      break;
    }
  }
}

void readMessageResponse(Response *msg)
{
  if (msg->code != NULL)
  {
    printf("%s\n", msg->message);
    switch (msg->code)
    {
    case PASSWORD_CORRECT:
      printf("Hello %s\n", msg->data);
      break;
    case LOGOUT_SUCCESS:
      printf("Goodbye %s\n", msg->data);
      break;
    case TOPIC_USER_CHOOSE_LEVEL:
      printf("User choose level %s\n", msg->data);
      break;
    default:
      break;
    }
  }
}

void setOpcodeRequest(Request *request, char *input)
{
  char code[BUFF_SIZE], data[BUFF_SIZE];
  splitMessage(input, code, data);
  strcpy(request->message, data);
  if (strcmp(code, "USER") == 0)
    request->code = USER;
  else if (strcmp(code, "PASS") == 0)
    request->code = PASS;
  else if (strcmp(code, "REGISTER") == 0)
    request->code = REGISTER;
  else if (strcmp(code, "LOGOUT") == 0)
    request->code = LOGOUT;
  else if (strcmp(code, "CHECK") == 0)
    request->code = CHECK;
  else if (strcmp(code, "INFORMATION") == 0)
    request->code = INFORMATION;
  else if (strcmp(code, "ANSWER") == 0)
    request->code = CHOOSE_ANWSER;
  else if (strcmp(code, "TOPIC") == 0)
    request->code = TOPIC_LEVEL;

}

int sendQuestion(int socket, Question *question, int size, int flags)
{
  int n;
  n = send(socket, question, size, flags);
  if (n < 0)
    perror("Error: ");
  return n;
}
int receiveQuestion(int socket, Question *question, int size, int flags)
{
  int n;
  n = recv(socket, question, size, flags);
  if (n < 0)
    perror("Error: ");
  return n;
}

void requestGet(int socket)
{
  Request *request = (Request *)malloc(sizeof(Request));
  setOpcodeRequest(request, "CHECK check");
  sendRequest(socket, request, sizeof(Request), 0);
}
void requestCheckInformation(int socket)
{
  Request *request = (Request *)malloc(sizeof(Request));
  setOpcodeRequest(request, "INFORMATION information");
  sendRequest(socket, request, sizeof(Request), 0);
}
int sendInformation(int socket, Information *infor, int size, int flags)
{
  int n;
  n = send(socket, infor, size, flags);
  if (n < 0)
    perror("Error: ");
  return n;
}
int receiveInformation(int socket, Information *infor, int size, int flags)
{
  int n;
  n = recv(socket, infor, size, flags);
  if (n < 0)
    perror("Error: ");
  return n;
}
void requestLogout(int socket, char *username)
{
  Request *request = (Request *)malloc(sizeof(Request));
  char buff[BUFF_SIZE];
  strcpy(buff, "LOGOUT ");
  strcat(buff, username);
  setOpcodeRequest(request, buff);
  sendRequest(socket, request, sizeof(Request), 0);
}
void requestGetHelp(int socket)
{
  Request *request = (Request *)malloc(sizeof(Request));
  setOpcodeRequest(request, "HELP help");
  sendRequest(socket, request, sizeof(Request), 0);
}