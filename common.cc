#include "common.h"

namespace common {
void nonblock(int fd) {
  auto a = fcntl(fd, F_GETFL);
  assert(a != -1);
  a = fcntl(fd, F_SETFL, a | O_NONBLOCK);
  assert(!a);
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

bool read(int fd, char *p, size_t s) {
  while (s) {
    auto n = ::read(fd, p, s);
    if (n < 0) {
      ERR("read: ", strerror(errno));
      return false;
    }

    if (n == 0) {
      LOG("EOF");
      return false;
    }

    p += n;
    s -= n;
  }

  return true;
}

bool write(int fd, const char *p, size_t s) {
  while (s) {
    auto n = ::write(fd, p, s);
    if (n < 0) {
      ERR("write: ", strerror(errno));
      return false;
    }

    p += n;
    s -= n;
  }

  return true;
}

void printTime(std::ostream &os) {
  auto now = std::chrono::system_clock::now();
  auto ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
  auto t = std::chrono::system_clock::to_time_t(ms);
  os << std::put_time(std::localtime(&t), "%T") << ".";
  os << std::setw(3) << std::setfill('0')
     << ms.time_since_epoch().count() % 1000;
}
} // namespace common
