#include <WinSock2.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

extern int win_echo_select_serv();

int main() {
  win_echo_select_serv();
  return 0;
}
