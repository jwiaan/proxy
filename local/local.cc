#include "local.h"
#include "../common.h"
#include "../msg.pb.h"
#include "../poller.h"
#include "../tunnel.h"
#include "acceptor.h"
#include "client.h"

namespace impl {
class Local : public ::Local, public Poller {
public:
  Local(int, int, SSL_CTX *);
  void start() override;

private:
  bool handle(const epoll_event &);
  bool accept();
  bool client2tunnel(Client *, uint32_t);
  bool tunnel2client(uint32_t);
  void forward(const std::string &);

private:
  std::shared_ptr<Acceptor> _acceptor;
  std::shared_ptr<Tunnel> _tunnel;
  std::map<uint32_t, std::shared_ptr<Client>> _clients;
};

Local::Local(int sfd, int cfd, SSL_CTX *ctx) {
  _acceptor = Acceptor::create(sfd);
  _tunnel = Tunnel::create(cfd, ctx, true);
}

void Local::start() {
  add(_acceptor, EPOLLIN);
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

bool Local::handle(const epoll_event &e) {
  if (e.data.ptr == _acceptor.get())
    return accept();

  if (e.data.ptr == _tunnel.get())
    return tunnel2client(e.events);

  return client2tunnel(static_cast<Client *>(e.data.ptr), e.events);
}

bool Local::accept() {
  uint32_t id;
  auto fd = _acceptor->accept(id);
  if (fd < 0)
    return false;

  assert(_clients.find(id) == _clients.end());
  auto client = Client::create(fd, id);
  _clients[id] = client;
  add(client, EPOLLIN | EPOLLOUT | EPOLLET);
  return true;
}

bool Local::client2tunnel(Client *client, uint32_t e) {
  if (e & EPOLLIN) {
    std::optional<std::string> opt;
    if (!client->recvMsg(opt)) {
      _clients.erase(client->id());
      return true;
    }

    if (opt.has_value()) {
      if (!_tunnel->sendMsg(opt.value()))
        return false;
    }
  }

  if (e & EPOLLOUT) {
    if (!client->writeAll())
      _clients.erase(client->id());
  }

  return true;
}

bool Local::tunnel2client(uint32_t e) {
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
  Msg msg;
  auto b = msg.ParseFromString(s);
  assert(b);

  auto it = _clients.find(msg.id());
  if (it == _clients.end())
    return;

  if (msg.has_resp()) {
    LOG(msg.DebugString());
    const auto &resp = msg.resp();
    b = it->second->sendResp(resp.result(), resp.host(), resp.port());
  } else {
    b = it->second->sendData(msg.data());
  }

  if (!b)
    _clients.erase(it);
}
} // namespace impl

std::shared_ptr<Local> Local::create(int sfd, int cfd, SSL_CTX *ctx) {
  return std::make_shared<impl::Local>(sfd, cfd, ctx);
}
