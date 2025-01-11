#include "../common.h"
#include "remote.h"

int main() {
  auto ctx = SSL_CTX_new(TLS_server_method());
  SSL_CTX_use_PrivateKey_file(ctx, "../../key", SSL_FILETYPE_PEM);
  SSL_CTX_use_certificate_file(ctx, "../../crt", SSL_FILETYPE_PEM);
  auto fd = start(bind, "0.0.0.0", 1111);
  listen(fd, 1024);
  while (true) {
    auto c = accept(fd, nullptr, nullptr);
    auto remote = Remote::create(c, ctx);
    std::thread t([remote] { remote->start(); });
    t.detach();
  }
}