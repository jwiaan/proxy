#include "server.h"
#include "../buffer.h"
#include "../common.h"

namespace impl {
class Server : public ::Server {
public:
  Server(uint32_t id, const std::string &name, int fd)
      : _id(id), _name(name), _buffer(Buffer::create(fd)) {}
  int fd() const override { return _buffer->fd(); }
  uint32_t id() const override { return _id; }
  bool writeAll() override { return _buffer->write(); }
  bool recvData(std::string &) override;
  bool sendData(const std::string &) override;

private:
  uint32_t _id;
  std::string _name;
  std::shared_ptr<Buffer> _buffer;
};

bool Server::recvData(std::string &s) {
  if (!_buffer->read())
    return false;

  s = _buffer->pop();
  LOG(_id, ".", _name, " <", s.size());
  return true;
}

bool Server::sendData(const std::string &s) {
  LOG(_id, ".", _name, " >", s.size());
  return _buffer->write(s);
}
} // namespace impl

std::shared_ptr<Server> Server::create(uint32_t id, const std::string &name,
                                       int fd) {
  return std::make_shared<impl::Server>(id, name, fd);
}