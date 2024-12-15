#include "connection.h"
#include "../common.h"
#include "../tunnel.pb.h"

class ConnectionImpl : public Connection {
public:
  ConnectionImpl(int fd, uint32_t id, uint32_t to) : _fd(fd), _id(id), _to(to) {
    LOG(_id, "->", _to);
  }
  ~ConnectionImpl();
  uint32_t id() const override { return _id; }
  bool read(common::Msg &) override;
  bool write(const common::Msg &) override;

private:
  int _fd;
  uint32_t _id, _to;
};

ConnectionImpl::~ConnectionImpl() {
  close(_fd);
  LOG(_id, "->", _to);
}

bool ConnectionImpl::read(common::Msg &msg) {
  char b[4096];
  auto n = ::read(_fd, b, sizeof(b));
  if (n < 0) {
    ERR("read: ", strerror(errno));
    return false;
  }

  if (n == 0) {
    LOG("EOF");
    return false;
  }

  tunnel::Data data;
  data.set_data(std::string(b, n));
  msg = common::toMsg(_to, data);
  return true;
}

bool ConnectionImpl::write(const common::Msg &msg) {
  tunnel::Data data;
  auto b = data.ParseFromString(msg.msg);
  assert(b);
  return common::write(_fd, data.data().data(), data.data().size());
}

std::shared_ptr<Connection> Connection::create(int fd, uint32_t id,
                                               uint32_t to) {
  return std::make_shared<ConnectionImpl>(fd, id, to);
}