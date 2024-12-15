#include "reader.h"
#include "common.h"

class ReaderImpl : public Reader {
public:
  ReaderImpl(int fd) : _fd(fd) {}
  bool read() override;
  std::string get(size_t) override;

private:
  int _fd;
  std::string _buf;
};

bool ReaderImpl::read() {
  char b[4096];
  while (true) {
    auto n = ::read(_fd, b, sizeof(b));
    if (n < 0) {
      if (errno == EAGAIN)
        return true;

      ERR("read: ", strerror(errno));
      return false;
    }

    if (n == 0) {
      LOG("EOF");
      return false;
    }

    _buf += std::string(b, n);
  }
}

std::string ReaderImpl::get(size_t n) {
  std::string s;
  if (_buf.size() >= n) {
    s.assign(_buf.data(), n);
    _buf.erase(0, n);
  }

  return s;
}

std::shared_ptr<Reader> Reader::create(int fd) {
  return std::make_shared<ReaderImpl>(fd);
}
