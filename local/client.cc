#include "client.h"
#include "../buffer.h"
#include "../common.h"
#include "../msg.pb.h"

namespace impl {
class Client : public ::Client, public Buffer {
public:
  Client(int fd, uint32_t id) : Buffer(fd), _id(id) {}
  int fd() const override { return _fd; }
  uint32_t id() const override { return _id; }
  bool writeAll() override { return write(); }
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
  bool (Client::*_step)(std::optional<std::string> &) = &Client::handshake;
};

bool Client::recvMsg(std::optional<std::string> &opt) {
  if (!read())
    return false;

  return (this->*_step)(opt);
}

bool Client::handshake(std::optional<std::string> &) {
  if (_in.size() < 3)
    return true;

  std::string s = {5, 1, 0};
  assert(_in == s);
  _in.clear();

  _out.push_back(5);
  _out.push_back(0);
  if (!write())
    return false;

  _step = &Client::request;
  return true;
}

bool Client::request(std::optional<std::string> &opt) {
  if (_in.size() < 5)
    return true;

  std::string s = {5, 1, 0, 3};
  assert(_in.substr(0, 4) == s);

  size_t n = 5 + _in.at(4) + 2;
  if (_in.size() < n)
    return true;

  _host.assign(&_in.at(5), _in.at(4));
  uint16_t port = *reinterpret_cast<uint16_t *>(&_in.at(5 + _in.at(4)));
  std::ostringstream oss;
  oss << ntohs(port);
  _port = oss.str();

  _in.erase(0, n);
  assert(_in.empty());

  Msg msg;
  msg.set_id(_id);
  auto req = msg.mutable_req();
  req->set_host(_host);
  req->set_port(_port);
  opt = msg.SerializeAsString();
  assert(!opt->empty());

  LOG(msg.DebugString());
  return true;
}

bool Client::forward(std::optional<std::string> &opt) {
  LOG(_id, ".", _host, ":", _port, " ", _in.size(), ">");
  Msg msg;
  msg.set_id(_id);
  msg.set_data(std::move(_in));
  opt = msg.SerializeAsString();
  assert(!opt->empty());
  _in.clear();
  return true;
}

bool Client::sendResp(bool res, uint32_t host, uint16_t port) {
  assert(_step == &Client::request);
  std::string s = {5, 0, 0, 1};
  s.at(1) = res ? 0 : 1;
  s += std::string(reinterpret_cast<char *>(&host), sizeof(host));
  s += std::string(reinterpret_cast<char *>(&port), sizeof(port));
  assert(s.size() == 10);

  _out += s;
  if (!write())
    return false;

  _step = &Client::forward;
  return res;
}

bool Client::sendData(const std::string &s) {
  assert(_step == &Client::forward);
  LOG(_id, ".", _host, ":", _port, " <", s.size());
  _out += s;
  return write();
}
} // namespace impl

std::shared_ptr<Client> Client::create(int fd, uint32_t id) {
  return std::make_shared<impl::Client>(fd, id);
}
