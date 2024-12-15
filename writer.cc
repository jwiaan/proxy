#include "writer.h"
#include "common.h"

class WriterImpl : public Writer {
public:
  WriterImpl(int fd) : _fd(fd) {}
  bool write(const std::string &) override;

private:
  int _fd;
  std::string _buf;
};

bool WriterImpl::write(const std::string &s) {
  _buf += s;
  while (!_buf.empty()) {
    auto n = ::write(_fd, _buf.data(), _buf.size());
    if (n < 0) {
      if (errno == EAGAIN)
        return true;

      ERR("write: ", strerror(errno));
      return false;
    }

    _buf.erase(0, n);
  }

  return true;
}

std::shared_ptr<Writer> Writer::create(int fd) {
  return std::make_shared<WriterImpl>(fd);
}
