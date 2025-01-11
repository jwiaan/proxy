#pragma once

#include <arpa/inet.h>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <netdb.h>
#include <openssl/bio.h>
#include <queue>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>

#define PRINT(os, s, ...)                                                      \
  do {                                                                         \
    ptime(os);                                                                 \
    os << s << ":" << __func__ << ":" << __FILE__ << ":" << __LINE__ << ": ";  \
    print(os, ##__VA_ARGS__);                                                  \
  } while (0)

#define LOG(...) PRINT(std::clog, "", ##__VA_ARGS__)
#define ERR(...) PRINT(std::cerr, "!!!!!ERROR", ##__VA_ARGS__)

inline void print(std::ostream &os) { os << std::endl; }
template <typename T, typename... V>
void print(std::ostream &os, const T &t, const V &...v) {
  os << t;
  print(os, v...);
}

void ptime(std::ostream &);
int start(int (*)(int, const sockaddr *, socklen_t), const char *, uint16_t);
