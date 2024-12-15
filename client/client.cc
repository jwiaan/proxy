#include "client.h"
#include "../common.h"
#include "../helper.h"
#include "../poller.h"
#include "../tunnel.h"
#include "connection.h"

class ClientImpl : public Client {
public:
  ClientImpl(int, SSL *);
  void start() override;

private:
  void accept();
  bool handle(const epoll_event &);
  bool handle();
  bool handle(Connection *);

private:
  int _fd;
  std::shared_ptr<Helper> _helper;
  std::shared_ptr<Poller> _poller;
  std::shared_ptr<Tunnel> _tunnel;
  std::map<uint32_t, std::shared_ptr<Connection>> _connections;
};

ClientImpl::ClientImpl(int fd, SSL *ssl) : _fd(fd) {
  _helper = Helper::create();
  _poller = Poller::create();
  _tunnel = Tunnel::create(ssl);
}

void ClientImpl::start() {
  _poller->add(_fd, this);
  _poller->add(_tunnel->fd(), _tunnel.get());
  std::array<epoll_event, 1024> a;

  while (true) {
    auto n = _poller->wait(a.data(), a.size());
    assert(n > 0);

    for (auto i = 0; i < n; ++i) {
      auto e = a[i];
      if (e.events != EPOLLIN)
        LOG(_poller->print(e.events));

      if (e.data.ptr == this) {
        accept();
        continue;
      }

      if (!handle(e))
        return;
    }
  }
}

void ClientImpl::accept() {
  auto fd = ::accept(_fd, nullptr, nullptr);
  if (fd < 0) {
    ERR("accept: ", strerror(errno));
    return;
  }

  auto id = _helper->id();
  auto co = Connection::create(fd, id);
  _connections[id] = co;
  _poller->add(fd, co.get());
}

bool ClientImpl::handle(const epoll_event &e) {
  if (e.data.ptr == _tunnel.get())
    return handle();

  return handle(static_cast<Connection *>(e.data.ptr));
}

bool ClientImpl::handle() {
  common::Msg msg;
  if (!_tunnel->read(msg))
    return false;

  if (!msg.type)
    return true;

  auto it = _connections.find(msg.id);
  if (it == _connections.end()) {
    ERR(msg.id, " not found");
    return true;
  }

  if (!it->second->write(msg))
    _connections.erase(it);

  return true;
}

bool ClientImpl::handle(Connection *co) {
  common::Msg msg;
  if (!co->read(msg)) {
    _connections.erase(co->id());
    return true;
  }

  if (!msg.type)
    return true;

  return _tunnel->write(msg);
}

std::shared_ptr<Client> Client::create(int fd, SSL *ssl) {
  return std::make_shared<ClientImpl>(fd, ssl);
}