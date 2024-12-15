#include "../common.h"
#include "server.h"

int main() {
  auto ctx = SSL_CTX_new(TLS_server_method());
  SSL_CTX_use_PrivateKey_file(ctx, "key", SSL_FILETYPE_PEM);
  SSL_CTX_use_certificate_file(ctx, "crt", SSL_FILETYPE_PEM);
  auto s = start(bind, "0.0.0.0", 1111);
  listen(s, 1024);
  while (true) {
    auto fd = accept(s, nullptr, nullptr);
    auto ssl = SSL_new(ctx);
    SSL_set_fd(ssl, fd);
    auto server = Server::create(ssl);
    std::thread t([server] { server->start(); });
    t.detach();
  }
}