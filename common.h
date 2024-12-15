#pragma once

#include <arpa/inet.h>
#include <cassert>
#include <chrono>
#include <fcntl.h>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <netdb.h>
#include <openssl/ssl.h>
#include <sstream>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>

#define PRINT(os, s, ...)                                                      \
  do {                                                                         \
    common::printTime(os);                                                     \
    os << s << ":" << __func__ << ":" << __FILE__ << ":" << __LINE__ << ": ";  \
    common::print(os, ##__VA_ARGS__);                                          \
  } while (0)

#define LOG(...) PRINT(std::clog, "", ##__VA_ARGS__)
#define ERR(...) PRINT(std::cerr, "!!!!!ERROR", ##__VA_ARGS__)

namespace common {
struct Msg {
  uint32_t id = 0;
  uint16_t type = 0;
  std::string msg;
};

template <typename T> Msg toMsg(uint32_t id, const T &t) {
  Msg msg;
  msg.id = id;
  msg.type = T::TYPE;
  auto b = t.SerializeToString(&msg.msg);
  assert(b);
  return msg;
}

template <typename T>
std::map<uint16_t, std::function<bool(T *, const std::string &)>> create() {
  return {};
}

template <typename T, typename U, typename... V>
std::map<uint16_t, std::function<bool(T *, const std::string &)>>
create(bool (T::*f)(const U &), bool (T::*...v)(const V &)) {
  auto a = create<T>(v...);
  a[U::TYPE] = [f](T *t, const std::string &s) {
    U u;
    auto b = u.ParseFromString(s);
    assert(b);
    return (t->*f)(u);
  };

  return a;
}

inline void print(std::ostream &os) { os << std::endl; }
template <typename T, typename... V>
void print(std::ostream &os, const T &t, const V &...v) {
  os << t;
  print(os, v...);
}

int start(int (*)(int, const sockaddr *, socklen_t), const char *, uint16_t);
void nonblock(int);
bool read(int, char *, size_t);
bool write(int, const char *, size_t);
void printTime(std::ostream &);
} // namespace common
