#include "../common.h"
#include "client.h"

int main() {
  auto ctx = SSL_CTX_new(TLS_client_method());
  SSL_CTX_clear_mode(ctx, SSL_MODE_AUTO_RETRY);
  auto ssl = SSL_new(ctx);

  auto fd = common::start(connect, "127.0.0.1", 1111);
  SSL_set_fd(ssl, fd);
  auto a = SSL_connect(ssl);
  assert(a == 1);

  fd = common::start(bind, "0.0.0.0", 2222);
  listen(fd, 1024);

  auto client = Client::create(fd, ssl);
  client->start();
}
