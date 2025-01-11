#include "local.h"
#include "../common.h"
#include "../poller.h"
#include "../proxy.pb.h"
#include "../tunnel.h"
#include "acceptor.h"
#include "client.h"

namespace impl {
class Local : public ::Local {
public:
  Local(int, int, SSL_CTX *);
  void start() override;

private:
  bool handle(const epoll_event &);
  bool accept();
  bool handle(Client *, uint32_t);
  bool handle(uint32_t);
  void forward(const std::string &);

private:
  std::shared_ptr<Acceptor> _acceptor;
  std::shared_ptr<Poller> _poller;
  std::shared_ptr<Tunnel> _tunnel;
  std::map<uint32_t, std::shared_ptr<Client>> _clients;
};

Local::Local(int s, int c, SSL_CTX *ctx) {
  _acceptor = Acceptor::create(s);
  _tunnel = Tunnel::create(c, ctx, true);
  _poller = Poller::create();
}

void Local::start() {
  _poller->add(_acceptor, EPOLLIN);
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

bool Local::handle(const epoll_event &e) {
  if (e.data.ptr == _acceptor.get())
    return accept();

  if (e.data.ptr == _tunnel.get())
    return handle(e.events);

  return handle(static_cast<Client *>(e.data.ptr), e.events);
}

bool Local::accept() {
  uint32_t id;
  auto fd = _acceptor->accept(id);
  if (fd < 0)
    return false;

  assert(_clients.find(id) == _clients.end());
  auto client = Client::create(id, fd);
  _clients[id] = client;
  _poller->add(client, EPOLLIN | EPOLLOUT | EPOLLET);
  return true;
}

bool Local::handle(Client *client, uint32_t e) {
  if (e & EPOLLIN) {
    std::optional<std::string> s;
    if (!client->recvMsg(s)) {
      _clients.erase(client->id());
      return true;
    }

    if (s.has_value()) {
      if (!_tunnel->sendMsg(s.value()))
        return false;
    }
  }

  if (e & EPOLLOUT) {
    if (!client->writeAll())
      _clients.erase(client->id());
  }

  return true;
}

bool Local::handle(uint32_t e) {
  if (e & EPOLLIN) {
    std::vector<std::string> v;
    if (!_tunnel->recvMsgs(v))
      return false;

    for (const auto &s : v)
      forward(s);
  }

  if (e & EPOLLOUT) {
    if (!_tunnel->writeAll())
      return false;
  }

  return true;
}

void Local::forward(const std::string &s) {
  proxy::Msg msg;
  auto b = msg.ParseFromString(s);
  assert(b);

  auto it = _clients.find(msg.id());
  if (it == _clients.end())
    return;

  if (msg.has_resp()) {
    LOG(msg.DebugString());
    if (!it->second->sendResp(msg.resp().result(), msg.resp().host(),
                              msg.resp().port()))
      _clients.erase(it);

    return;
  }

  if (!it->second->sendData(msg.data()))
    _clients.erase(it);
}
} // namespace impl

std::shared_ptr<Local> Local::create(int s, int c, SSL_CTX *ctx) {
  return std::make_shared<impl::Local>(s, c, ctx);
}
