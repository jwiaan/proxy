#include "remote.h"
#include "../common.h"
#include "../poller.h"
#include "../proxy.pb.h"
#include "../tunnel.h"
#include "connector.h"
#include "server.h"

namespace impl {
class Remote : public ::Remote {
public:
  Remote(int, SSL_CTX *);
  void start() override;

private:
  bool handle(const epoll_event &);
  bool create();
  bool handle(Server *, uint32_t);
  bool handle(uint32_t);
  void handle(const std::string &);

private:
  std::shared_ptr<Connector> _connector;
  std::shared_ptr<Poller> _poller;
  std::shared_ptr<Tunnel> _tunnel;
  std::map<uint32_t, std::shared_ptr<Server>> _servers;
  std::map<uint32_t, std::string> _names;
};

Remote::Remote(int c, SSL_CTX *ctx) {
  _tunnel = Tunnel::create(c, ctx, false);
  _connector = Connector::create();
  _poller = Poller::create();
}

void Remote::start() {
  _poller->add(_connector, EPOLLIN);
  _poller->add(_tunnel, EPOLLIN | EPOLLOUT | EPOLLET);
  std::vector<epoll_event> v(1024);
  while (true) {
    auto n = _poller->wait(v);
    for (auto i = 0; i < n; ++i) {
      if (!handle(v.at(i)))
        return;
    }
  }
}

bool Remote::handle(const epoll_event &e) {
  if (e.data.ptr == _connector.get())
    return create();

  if (e.data.ptr == _tunnel.get())
    return handle(e.events);

  return handle(static_cast<Server *>(e.data.ptr), e.events);
}

bool Remote::create() {
  uint32_t id;
  auto fd = _connector->connect(id);
  proxy::Msg msg;
  msg.set_id(id);
  auto resp = msg.mutable_resp();
  if (fd != -1) {
    assert(_servers.find(id) == _servers.end());
    auto server = Server::create(id, _names.at(id), fd);
    _servers[id] = server;
    _poller->add(server, EPOLLIN | EPOLLOUT | EPOLLET);

    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getsockname(fd, reinterpret_cast<sockaddr *>(&addr), &len);
    resp->set_result(true);
    resp->set_host(addr.sin_addr.s_addr);
    resp->set_port(addr.sin_port);
  }

  _names.erase(id);
  std::string s;
  auto b = msg.SerializeToString(&s);
  assert(b);
  LOG(msg.DebugString());
  return _tunnel->sendMsg(s);
}

bool Remote::handle(Server *server, uint32_t e) {
  if (e & EPOLLIN) {
    std::string s;
    if (!server->recvData(s)) {
      _servers.erase(server->id());
      return true;
    }

    proxy::Msg msg;
    msg.set_id(server->id());
    msg.set_data(s);
    auto b = msg.SerializeToString(&s);
    assert(b);
    if (!_tunnel->sendMsg(s))
      return false;
  }

  if (e & EPOLLOUT) {
    if (!server->writeAll())
      _servers.erase(server->id());
  }

  return true;
}

bool Remote::handle(uint32_t e) {
  if (e & EPOLLIN) {
    std::vector<std::string> v;
    if (!_tunnel->recvMsgs(v))
      return false;

    for (const auto &s : v)
      handle(s);
  }

  if (e & EPOLLOUT) {
    if (!_tunnel->writeAll())
      return false;
  }

  return true;
}

void Remote::handle(const std::string &s) {
  proxy::Msg msg;
  auto b = msg.ParseFromString(s);
  assert(b);

  if (msg.has_req()) {
    LOG(msg.DebugString());
    assert(_names.find(msg.id()) == _names.end());
    _names[msg.id()] = msg.req().host() + ":" + msg.req().port();
    _connector->connect(msg.id(), msg.req().host(), msg.req().port());
    return;
  }

  auto it = _servers.find(msg.id());
  if (it == _servers.end())
    return;

  if (!it->second->sendData(msg.data()))
    _servers.erase(it);
}
} // namespace impl

std::shared_ptr<Remote> Remote::create(int c, SSL_CTX *ctx) {
  return std::make_shared<impl::Remote>(c, ctx);
}
