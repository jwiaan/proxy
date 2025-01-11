#include "../common.h"
#include "local.h"

int main() {
  auto ctx = SSL_CTX_new(TLS_client_method());
  auto c = start(connect, "127.0.0.1", 1111);
  auto s = start(bind, "0.0.0.0", 2222);
  listen(s, 1024);
  auto local = Local::create(s, c, ctx);
  local->start();
}
