#include "../common.h"
#include "../poller.h"
#include "client.h"
#include "connection.h"

void accept(std::shared_ptr<Poller> poller, int s) {
  auto c = accept(s, nullptr, nullptr);
  if (c < 0) {
    perror("accept");
    return;
  }

  connections[c] = Connection::create(c);
  poller->add(c);
}

void start(int c, int s) {
  auto poller = Poller::create();
  poller->add(c);
  poller->add(s);
  std::array<epoll_event, 1024> a;
  assert(a.size() == 1024);
  while (true) {
    auto n = poller->wait(a.data(), a.size());
    assert(n > 0);
    for (auto i = 0; i < n; ++i) {
      auto fd = a[i].data.fd;
      if (fd == s) {
        accept(poller, s);
      } else if (fd == c) {
        Client::get().handle();
      } else {
        if (!connections.at(fd)->handle())
          connections.erase(fd);
      }
    }
  }
}

int main() {
  auto c = start(connect, "127.0.0.1", 1111);
  Client::get().connect(c);
  auto s = start(bind, "0.0.0.0", 2222);
  listen(s, 1024);
  start(c, s);
}
