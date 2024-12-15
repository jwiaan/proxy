#include "connection.h"
#include "../common.h"
#include "../tunnel.pb.h"

enum class State { INIT, WAIT, CONNECTING, CONNECTED };

class ConnectionImpl : public Connection {
public:
  ConnectionImpl(int, uint32_t);
  ~ConnectionImpl();
  uint32_t id() const override { return _id; }
  bool read(common::Msg &msg) override { return _readers.at(_state)(msg); }
  bool write(const common::Msg &msg) override {
    return _writers.at(msg.type)(this, msg.msg);
  }

private:
  bool init(common::Msg &);
  bool readReq(common::Msg &);
  bool readData(common::Msg &);
  bool connecting(common::Msg &);
  bool writeResp(const tunnel::Resp &);
  bool writeData(const tunnel::Data &);

private:
  int _fd;
  uint32_t _id, _to = 0;
  State _state = State::INIT;
  std::string _host, _port;
  std::map<State, std::function<bool(common::Msg &)>> _readers;
  std::map<uint16_t, std::function<bool(ConnectionImpl *, const std::string &)>>
      _writers;
};

ConnectionImpl::ConnectionImpl(int fd, uint32_t id) : _fd(fd), _id(id) {
  _readers = {{State::INIT,
               std::bind(&ConnectionImpl::init, this, std::placeholders::_1)},
              {State::WAIT, std::bind(&ConnectionImpl::readReq, this,
                                      std::placeholders::_1)},
              {State::CONNECTING, std::bind(&ConnectionImpl::connecting, this,
                                            std::placeholders::_1)},
              {State::CONNECTED, std::bind(&ConnectionImpl::readData, this,
                                           std::placeholders::_1)}};
  _writers =
      common::create(&ConnectionImpl::writeResp, &ConnectionImpl::writeData);
  LOG(_id);
}

ConnectionImpl::~ConnectionImpl() {
  close(_fd);
  LOG(_id, "->", _to, ",", _host, ":", _port);
}

bool ConnectionImpl::init(common::Msg &msg) {
  char a[2];
  if (!common::read(_fd, a, 2))
    return false;

  assert(a[0] == 5);
  char b[256];
  if (!common::read(_fd, b, a[1]))
    return false;

  a[1] = 0;
  if (!common::write(_fd, a, 2))
    return false;

  msg.type = 0;
  _state = State::WAIT;
  return true;
}

bool ConnectionImpl::readReq(common::Msg &msg) {
  char b[5];
  if (!common::read(_fd, b, 5))
    return false;

  assert(b[0] == 5);
  assert(b[1] == 1);
  assert(b[3] == 3);

  _host.resize(b[4]);
  if (!common::read(_fd, const_cast<char *>(_host.data()), _host.size()))
    return false;

  uint16_t port;
  if (!common::read(_fd, reinterpret_cast<char *>(&port), sizeof(port)))
    return false;

  std::ostringstream oss;
  oss << ntohs(port);
  _port = oss.str();

  tunnel::Req req;
  req.set_id(_id);
  req.set_host(_host);
  req.set_port(_port);
  LOG("\n", req.DebugString());

  msg = common::toMsg(0, req);
  _state = State::CONNECTING;
  return true;
}

bool ConnectionImpl::readData(common::Msg &msg) {
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

bool ConnectionImpl::connecting(common::Msg &) {
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

  assert(0);
}

bool ConnectionImpl::writeResp(const tunnel::Resp &resp) {
  LOG("\n", resp.DebugString());
  assert(_state == State::CONNECTING);

  _to = resp.id();
  sockaddr_in addr;
  addr.sin_addr.s_addr = resp.host();
  addr.sin_port = resp.port();
  LOG(inet_ntoa(addr.sin_addr), ":", ntohs(addr.sin_port));

  char b[10] = {};
  b[0] = 5;
  b[1] = !_to;
  b[3] = 1;
  memcpy(&b[4], &addr.sin_addr.s_addr, sizeof(addr.sin_addr.s_addr));
  memcpy(&b[8], &addr.sin_port, sizeof(addr.sin_port));

  if (!common::write(_fd, b, sizeof(b)))
    return false;

  _state = State::CONNECTED;
  return _to != 0;
}

bool ConnectionImpl::writeData(const tunnel::Data &data) {
  return common::write(_fd, data.data().data(), data.data().size());
}

std::shared_ptr<Connection> Connection::create(int fd, uint32_t id) {
  return std::make_shared<ConnectionImpl>(fd, id);
}
