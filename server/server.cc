#include "server.h"
#include "../common.h"
#include "../helper.h"
#include "../poller.h"
#include "../tunnel.h"
#include "../tunnel.pb.h"
#include "connection.h"

struct Result {
  int fd = -1;
  uint32_t id = 0;
};

class ServerImpl : public Server {
public:
  ServerImpl(SSL *);
  ~ServerImpl();
  void start() override;

private:
  bool handle(const epoll_event &);
  bool handle();
  bool handle(Connection *);
  bool writeResp();
  void thread(const common::Msg &);
  int connect(const std::string &, const std::string &);

private:
  int _pipe[2];
  std::shared_ptr<Helper> _helper;
  std::shared_ptr<Poller> _poller;
  std::shared_ptr<Tunnel> _tunnel;
  std::map<uint32_t, std::shared_ptr<Connection>> _connections;
};

ServerImpl::ServerImpl(SSL *ssl) {
  pipe(_pipe);
  _helper = Helper::create();
  _poller = Poller::create();
  _tunnel = Tunnel::create(ssl);
}

ServerImpl::~ServerImpl() {
  close(_pipe[0]);
  close(_pipe[1]);
}

void ServerImpl::start() {
  _poller->add(_pipe[0], this);
  _poller->add(_tunnel->fd(), _tunnel.get());
  std::array<epoll_event, 1024> a;

  while (true) {
    auto n = _poller->wait(a.data(), a.size());
    assert(n > 0);

    for (auto i = 0; i < n; i++) {
      auto e = a[i];
      if (e.events != EPOLLIN)
        LOG(_poller->print(e.events));

      if (!handle(e))
        return;
    }
  }
}

bool ServerImpl::handle(const epoll_event &e) {
  if (e.data.ptr == this)
    return writeResp();

  if (e.data.ptr == _tunnel.get())
    return handle();

  return handle(static_cast<Connection *>(e.data.ptr));
}

bool ServerImpl::handle() {
  common::Msg msg;
  if (!_tunnel->read(msg))
    return false;

  if (msg.type == tunnel::Req::TYPE) {
    std::thread t(std::bind(&ServerImpl::thread, this, std::placeholders::_1),
                  msg);
    t.detach();
    return true;
  }

  auto it = _connections.find(msg.id);
  if (it == _connections.end()) {
    ERR(msg.id, " not found");
    return true;
  }

  if (!it->second->write(msg))
    _connections.erase(it);

  return true;
}

bool ServerImpl::handle(Connection *co) {
  common::Msg msg;
  if (!co->read(msg)) {
    _connections.erase(co->id());
    return true;
  }

  return _tunnel->write(msg);
}

bool ServerImpl::writeResp() {
  Result res;
  auto n = read(_pipe[0], &res, sizeof(res));
  assert(n == sizeof(res));

  uint32_t id = 0;
  sockaddr_in addr = {};
  if (res.fd != -1) {
    socklen_t len = sizeof(addr);
    getsockname(res.fd, reinterpret_cast<sockaddr *>(&addr), &len);
    id = _helper->id();
    auto co = Connection::create(res.fd, id, res.id);
    _connections[id] = co;
    _poller->add(res.fd, co.get());
  }

  tunnel::Resp resp;
  resp.set_id(id);
  resp.set_host(addr.sin_addr.s_addr);
  resp.set_port(addr.sin_port);
  LOG("\n", resp.DebugString());
  return _tunnel->write(common::toMsg(res.id, resp));
}

void ServerImpl::thread(const common::Msg &msg) {
  tunnel::Req req;
  auto b = req.ParseFromString(msg.msg);
  assert(b);
  LOG("\n", req.DebugString());

  Result res;
  res.fd = connect(req.host(), req.port());
  res.id = req.id();
  auto n = write(_pipe[1], &res, sizeof(res));
  if (n < 0) {
    ERR("write: ", strerror(errno));
    return;
  }

  assert(n == sizeof(res));
}

int ServerImpl::connect(const std::string &host, const std::string &port) {
  addrinfo *l, hints = {};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  auto e = getaddrinfo(host.c_str(), port.c_str(), &hints, &l);
  if (e) {
    ERR("getaddrinfo: ", gai_strerror(e));
    return -1;
  }

  for (auto a = l; a; a = a->ai_next) {
    auto fd = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
    if (fd < 0) {
      ERR("socket: ", strerror(errno));
      continue;
    }

    if (!::connect(fd, a->ai_addr, a->ai_addrlen)) {
      freeaddrinfo(l);
      LOG(host, ":", port, " connected");
      return fd;
    }

    ERR("connect: ", strerror(errno));
    close(fd);
  }

  return -1;
}

std::shared_ptr<Server> Server::create(SSL *ssl) {
  return std::make_shared<ServerImpl>(ssl);
}
