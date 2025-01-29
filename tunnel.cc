#include "tunnel.h"
#include "common.h"

namespace impl {
class Tunnel : public ::Tunnel {
public:
  Tunnel(int, SSL_CTX *, bool);
  ~Tunnel();
  int fd() const override { return _fd; }
  bool writeAll() override;
  bool sendMsg(const std::string &) override;
  bool recvMsgs(std::vector<std::string> &) override;

private:
  bool check() const;
  bool readAll();
  bool write();
  bool read(std::vector<std::string> &);
  bool read();
  void getMsgs(std::vector<std::string> &);

private:
  int _fd;
  SSL *_ssl;
  BIO *_rbio, *_wbio;
  std::string _in;
  std::queue<std::string> _out;
};

Tunnel::Tunnel(int fd, SSL_CTX *ctx, bool cli) : _fd(fd) {
  BIO_socket_nbio(_fd, 1);
  _rbio = BIO_new(BIO_s_mem());
  _wbio = BIO_new(BIO_s_mem());
  _ssl = SSL_new(ctx);
  SSL_set_bio(_ssl, _rbio, _wbio);
  if (cli)
    SSL_set_connect_state(_ssl);
  else
    SSL_set_accept_state(_ssl);
}

Tunnel::~Tunnel() {
  SSL_shutdown(_ssl);
  SSL_free(_ssl);
  close(_fd);
}

bool Tunnel::writeAll() {
  while (BIO_ctrl_pending(_wbio)) {
    char *p;
    auto l = BIO_get_mem_data(_wbio, &p);
    auto n = ::write(_fd, p, l);
    if (n < 0) {
      if (errno == EAGAIN)
        return true;

      ERR("write ", strerror(errno));
      return false;
    }

    char b[n];
    size_t s;
    auto a = BIO_read_ex(_wbio, b, n, &s);
    assert(a == 1);
    assert(s == static_cast<size_t>(n));
  }

  return true;
}

bool Tunnel::readAll() {
  while (true) {
    char b[1024 * 1024];
    auto n = ::read(_fd, b, sizeof(b));
    if (n < 0) {
      if (errno == EAGAIN)
        return true;

      ERR("read: ", strerror(errno));
      return false;
    }

    if (n == 0)
      return false;

    size_t s;
    auto a = BIO_write_ex(_rbio, b, n, &s);
    assert(a == 1);
    assert(s == static_cast<size_t>(n));
  }
}

bool Tunnel::sendMsg(const std::string &s) {
  assert(s.size() <= UINT32_MAX);
  uint32_t size = s.size();
  size = htonl(size);
  _out.emplace(reinterpret_cast<char *>(&size), sizeof(size));
  _out.back() += s;
  return write();
}

bool Tunnel::write() {
  while (!_out.empty()) {
    size_t n;
    auto a = SSL_write_ex(_ssl, _out.front().data(), _out.front().size(), &n);
    if (!writeAll())
      return false;

    if (!a)
      return check();

    assert(n == _out.front().size());
    _out.pop();
  }

  return true;
}

bool Tunnel::recvMsgs(std::vector<std::string> &msgs) {
  if (!readAll())
    return false;

  if (!write())
    return false;

  return read(msgs);
}

bool Tunnel::read(std::vector<std::string> &v) {
  if (!read())
    return false;

  getMsgs(v);
  return true;
}

bool Tunnel::read() {
  while (true) {
    char b[1024 * 1024];
    size_t n;
    auto a = SSL_read_ex(_ssl, b, sizeof(b), &n);
    if (!writeAll())
      return false;

    if (!a)
      return check();

    _in += std::string(b, n);
  }
}

void Tunnel::getMsgs(std::vector<std::string> &v) {
  while (!_in.empty()) {
    uint32_t size;
    if (_in.size() < sizeof(size))
      return;

    size = *reinterpret_cast<uint32_t *>(_in.data());
    size = ntohl(size);
    size_t n = sizeof(size) + size;
    if (_in.size() < n)
      return;

    v.emplace_back(&_in.at(sizeof(size)), size);
    _in.erase(0, n);
  }
}

bool Tunnel::check() const {
  auto e = SSL_get_error(_ssl, 0);
  if (e == SSL_ERROR_WANT_READ) {
    assert(!BIO_ctrl_pending(_rbio));
    return true;
  }

  ERR(e);
  assert(0);
  return false;
}
} // namespace impl

std::shared_ptr<Tunnel> Tunnel::create(int fd, SSL_CTX *ctx, bool cli) {
  return std::make_shared<impl::Tunnel>(fd, ctx, cli);
}
