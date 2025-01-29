#include "connector.h"
#include "../common.h"

struct Result {
  int fd;
  uint32_t id;
};

namespace impl {
class Connector : public ::Connector {
public:
  Connector();
  ~Connector();
  int fd() const override { return _pipe[0]; }
  void connect(uint32_t, const std::string &, const std::string &) override;
  int connect(uint32_t &, std::string &) override;

private:
  void thread(uint32_t, const std::string &, const std::string &);
  int connect(const std::string &, const std::string &);

private:
  int _pipe[2];
  std::map<uint32_t, std::string> _names;
};

Connector::Connector() { pipe(_pipe); }

Connector::~Connector() {
  close(_pipe[0]);
  close(_pipe[1]);
}

void Connector::connect(uint32_t id, const std::string &host,
                        const std::string &port) {
  assert(_names.find(id) == _names.end());
  _names[id] = host + ":" + port;
  std::thread t(&Connector::thread, this, id, host, port);
  t.detach();
}

void Connector::thread(uint32_t id, const std::string &host,
                       const std::string &port) {
  Result res;
  res.id = id;
  res.fd = connect(host, port);
  auto n = write(_pipe[1], &res, sizeof(res));
  assert(n == sizeof(res));
}

int Connector::connect(const std::string &host, const std::string &port) {
  addrinfo *p;
  auto e = getaddrinfo(host.c_str(), port.c_str(), NULL, &p);
  if (e) {
    ERR(host, ":", port, " ", gai_strerror(e));
    return -1;
  }

  for (auto a = p; a; a = a->ai_next) {
    auto fd = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
    if (fd < 0) {
      ERR("socket: ", strerror(errno));
      continue;
    }

    if (!::connect(fd, a->ai_addr, a->ai_addrlen)) {
      freeaddrinfo(p);
      LOG(host, ":", port, " connected");
      return fd;
    }

    ERR(host, ":", port, " ", strerror(errno));
    close(fd);
  }

  return -1;
}

int Connector::connect(uint32_t &id, std::string &name) {
  Result res;
  auto n = read(_pipe[0], &res, sizeof(res));
  assert(n == sizeof(res));
  id = res.id;
  name = _names.at(id);
  _names.erase(id);
  return res.fd;
}
} // namespace impl

std::shared_ptr<Connector> Connector::create() {
  return std::make_shared<impl::Connector>();
}
