#include "client.h"
#include "../buffer.h"
#include "../common.h"
#include "../proxy.pb.h"

namespace impl {
class Client : public ::Client {
public:
  Client(uint32_t id, int fd) : _id(id), _buffer(Buffer::create(fd)) {}
  int fd() const override { return _buffer->fd(); }
  uint32_t id() const override { return _id; }
  bool writeAll() override { return _buffer->write(); }
  bool recvMsg(std::optional<std::string> &) override;
  bool sendResp(bool, uint32_t, uint16_t) override;
  bool sendData(const std::string &) override;

private:
  bool handshake(std::optional<std::string> &);
  bool request(std::optional<std::string> &);
  bool forward(std::optional<std::string> &);

private:
  uint32_t _id;
  std::string _host, _port;
  std::shared_ptr<Buffer> _buffer;
  bool (Client::*_step)(std::optional<std::string> &) = &Client::handshake;
};

bool Client::recvMsg(std::optional<std::string> &msg) {
  if (!_buffer->read())
    return false;

  return (this->*_step)(msg);
}

bool Client::handshake(std::optional<std::string> &) {
  auto a = _buffer->peek(3);
  if (!a.has_value())
    return true;

  std::string t = {5, 1, 0};
  assert(a.value() == t);

  _buffer->pop(3);
  assert(_buffer->empty());

  if (!_buffer->write({5, 0}))
    return false;

  _step = &Client::request;
  return true;
}

bool Client::request(std::optional<std::string> &s) {
  auto a = _buffer->peek(5);
  if (!a.has_value())
    return false;

  std::string t = {5, 1, 0, 3};
  assert(a.value().substr(0, 4) == t);

  size_t n = 5 + a.value().at(4) + 2;
  a = _buffer->peek(n);
  if (!a.has_value())
    return true;

  _buffer->pop(n);
  assert(_buffer->empty());

  _host.assign(&a.value().at(5), a.value().at(4));
  uint16_t port =
      *reinterpret_cast<uint16_t *>(&a.value().at(5 + a.value().at(4)));
  std::ostringstream oss;
  oss << ntohs(port);
  _port = oss.str();

  proxy::Msg msg;
  msg.set_id(_id);
  auto req = msg.mutable_req();
  req->set_host(_host);
  req->set_port(_port);
  s = msg.SerializeAsString();
  assert(!s->empty());

  LOG(msg.DebugString());
  return true;
}

bool Client::forward(std::optional<std::string> &s) {
  proxy::Msg msg;
  msg.set_id(_id);
  msg.set_data(_buffer->pop());
  s = msg.SerializeAsString();
  assert(!s->empty());
  LOG(_id, ".", _host, ":", _port, " >", msg.data().size());
  return true;
}

bool Client::sendResp(bool res, uint32_t host, uint16_t port) {
  std::string s = {5, 0, 0, 1};
  s.at(1) = res ? 0 : 1;
  s += std::string(reinterpret_cast<char *>(&host), sizeof(host));
  s += std::string(reinterpret_cast<char *>(&port), sizeof(port));
  assert(s.size() == 10);

  if (!_buffer->write(s))
    return false;

  assert(_step == &Client::request);
  _step = &Client::forward;
  return res;
}

bool Client::sendData(const std::string &s) {
  assert(_step == &Client::forward);
  LOG(_id, ".", _host, ":", _port, " <", s.size());
  return _buffer->write(s);
}
} // namespace impl

std::shared_ptr<Client> Client::create(uint32_t id, int fd) {
  return std::make_shared<impl::Client>(id, fd);
}
