#include "server.h"
#include "../buffer.h"
#include "../common.h"

namespace impl {
class Server : public ::Server, public Buffer {
public:
  Server(int fd, uint32_t id, const std::string &name)
      : Buffer(fd), _id(id), _name(name) {}
  int fd() const override { return _fd; }
  uint32_t id() const override { return _id; }
  bool writeAll() override { return write(); }
  bool recvData(std::string &) override;
  bool sendData(const std::string &) override;

private:
  uint32_t _id;
  std::string _name;
  std::shared_ptr<Buffer> _buffer;
};

bool Server::recvData(std::string &s) {
  if (!read())
    return false;

  s = std::move(_in);
  _in.clear();
  LOG(_id, ".", _name, " <", s.size());
  return true;
}

bool Server::sendData(const std::string &s) {
  LOG(_id, ".", _name, " ", s.size(), ">");
  _out += s;
  return write();
}
} // namespace impl

std::shared_ptr<Server> Server::create(int fd, uint32_t id,
                                       const std::string &name) {
  return std::make_shared<impl::Server>(fd, id, name);
}