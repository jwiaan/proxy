#include "tunnel.h"
#include "common.h"

struct Header {
  uint32_t id = 0;
  uint16_t type = 0;
  uint16_t size = 0;
};

class TunnelImpl : public Tunnel {
public:
  TunnelImpl(SSL *ssl) : _ssl(ssl) { LOG(_ssl); }
  ~TunnelImpl();
  int fd() const override { return SSL_get_fd(_ssl); }
  bool read(common::Msg &) override;
  bool write(const common::Msg &) override;

private:
  bool read(char *, size_t);
  bool write(const char *, size_t);

private:
  SSL *_ssl;
};

TunnelImpl::~TunnelImpl() {
  auto fd = SSL_get_fd(_ssl);
  SSL_shutdown(_ssl);
  SSL_free(_ssl);
  close(fd);
  LOG(_ssl);
}

bool TunnelImpl::read(common::Msg &msg) {
  Header h;
  if (!read(reinterpret_cast<char *>(&h), sizeof(h)))
    return false;

  msg.id = ntohl(h.id);
  msg.type = ntohs(h.type);
  msg.msg.resize(ntohs(h.size));
  return read(const_cast<char *>(msg.msg.data()), msg.msg.size());
}

bool TunnelImpl::write(const common::Msg &msg) {
  Header h;
  h.id = htonl(msg.id);
  h.type = htons(msg.type);
  h.size = htons(msg.msg.size());

  std::string s(reinterpret_cast<char *>(&h), sizeof(h));
  s += msg.msg;
  return write(s.data(), s.size());
}

bool TunnelImpl::read(char *p, size_t s) {
  while (s) {
    auto n = SSL_read(_ssl, p, s);
    if (n <= 0) {
      int e = SSL_get_error(_ssl, n);
      if (e == SSL_ERROR_WANT_READ)
        return true;

      ERR("SSL_read: ", e);
      return false;
    }

    p += n;
    s -= n;
  }

  return true;
}

bool TunnelImpl::write(const char *p, size_t s) {
  while (s) {
    auto n = SSL_write(_ssl, p, s);
    if (n <= 0) {
      ERR("SSL_write: ", SSL_get_error(_ssl, n));
      return false;
    }

    p += n;
    s -= n;
  }

  return true;
}

std::shared_ptr<Tunnel> Tunnel::create(SSL *ssl) {
  return std::make_shared<TunnelImpl>(ssl);
}