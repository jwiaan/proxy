#include "../common.h"
#include "local.h"

int main() {
  auto ctx = SSL_CTX_new(TLS_client_method());
  auto cfd = start(connect, "127.0.0.1", 1111);
  auto sfd = start(bind, "0.0.0.0", 2222);
  listen(sfd, 1024);
  auto local = Local::create(sfd, cfd, ctx);
  local->start();
}
