#include "remote.h"
#include "../common.h"
#include "../msg.pb.h"
#include "../poller.h"
#include "../tunnel.h"
#include "connector.h"
#include "server.h"

namespace impl {
class Remote : public ::Remote, public Poller {
public:
  Remote(int, SSL_CTX *);
  void start() override;

private:
  bool handle(const epoll_event &);
  bool create();
  bool server2tunnel(Server *, uint32_t);
  bool tunnel2server(uint32_t);
  void handle(const std::string &);

private:
  std::shared_ptr<Connector> _connector;
  std::shared_ptr<Tunnel> _tunnel;
  std::map<uint32_t, std::shared_ptr<Server>> _servers;
};

Remote::Remote(int fd, SSL_CTX *ctx) {
  _connector = Connector::create();
  _tunnel = Tunnel::create(fd, ctx, false);
}

void Remote::start() {
  add(_connector, EPOLLIN);
  add(_tunnel, EPOLLIN | EPOLLOUT | EPOLLET);
  std::vector<epoll_event> v(1024);
  while (true) {
    auto n = wait(v);
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
    return tunnel2server(e.events);

  return server2tunnel(static_cast<Server *>(e.data.ptr), e.events);
}

bool Remote::create() {
  uint32_t id;
  std::string name;
  auto fd = _connector->connect(id, name);
  Msg msg;
  msg.set_id(id);
  auto resp = msg.mutable_resp();
  if (fd != -1) {
    assert(_servers.find(id) == _servers.end());
    auto server = Server::create(fd, id, name);
    _servers[id] = server;
    add(server, EPOLLIN | EPOLLOUT | EPOLLET);

    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getsockname(fd, reinterpret_cast<sockaddr *>(&addr), &len);
    resp->set_result(true);
    resp->set_host(addr.sin_addr.s_addr);
    resp->set_port(addr.sin_port);
  }

  LOG(msg.DebugString());
  auto s = msg.SerializeAsString();
  assert(!s.empty());
  return _tunnel->sendMsg(s);
}

bool Remote::server2tunnel(Server *server, uint32_t e) {
  if (e & EPOLLIN) {
    std::string data;
    if (!server->recvData(data)) {
      _servers.erase(server->id());
      return true;
    }

    Msg msg;
    msg.set_id(server->id());
    msg.set_data(data);
    auto s = msg.SerializeAsString();
    assert(!s.empty());
    if (!_tunnel->sendMsg(s))
      return false;
  }

  if (e & EPOLLOUT) {
    if (!server->writeAll())
      _servers.erase(server->id());
  }

  return true;
}

bool Remote::tunnel2server(uint32_t e) {
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
  Msg msg;
  auto b = msg.ParseFromString(s);
  assert(b);

  if (msg.has_req()) {
    LOG(msg.DebugString());
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

std::shared_ptr<Remote> Remote::create(int fd, SSL_CTX *ctx) {
  return std::make_shared<impl::Remote>(fd, ctx);
}
