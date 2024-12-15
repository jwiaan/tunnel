#include "common.h"

int start(int (*action)(int, const sockaddr *, socklen_t), const char *host,
          uint16_t port) {
  auto fd = socket(AF_INET, SOCK_STREAM, 0);
  int val = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  auto a = inet_aton(host, &addr.sin_addr);
  assert(a);

  if (action(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr))) {
    perror("");
    return -1;
  }

  return fd;
}

uint16_t readMsg(SSL *ssl, std::string &s) {
  Head h;
  if (!call(SSL_read, ssl, &h, sizeof(h)))
    return 0;

  h.type = ntohs(h.type);
  h.size = ntohs(h.size);
  s.resize(h.size);
  if (!call(SSL_read, ssl, s.data(), s.size()))
    return 0;

  return h.type;
}

bool sendMsg(SSL *ssl, uint16_t type, const std::string &s) {
  Head h;
  h.type = htons(type);
  h.size = htons(s.size());
  if (!call(SSL_write, ssl, &h, sizeof(h)))
    return false;

  if (!call(SSL_write, ssl, s.data(), s.size()))
    return false;

  return true;
}
