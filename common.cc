#include "common.h"

void ptime(std::ostream &os) {
  auto now = std::chrono::system_clock::now();
  auto ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
  auto t = std::chrono::system_clock::to_time_t(ms);
  os << std::put_time(std::localtime(&t), "%T") << ".";
  os << std::setw(3) << std::setfill('0')
     << ms.time_since_epoch().count() % 1000;
}

int start(int (*action)(int, const sockaddr *, socklen_t), const char *host,
          uint16_t port) {
  auto fd = socket(AF_INET, SOCK_STREAM, 0);
  int val = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  auto a = inet_aton(host, &addr.sin_addr);
  assert(a);

  if (action(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr))) {
    ERR(strerror(errno));
    return -1;
  }

  return fd;
}
