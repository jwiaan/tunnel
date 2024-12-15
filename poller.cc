#include "poller.h"
#include <unistd.h>

class PollerImpl : public Poller {
public:
  PollerImpl();
  ~PollerImpl();
  int add(int) override;
  int wait(epoll_event *, int) override;

private:
  int _fd;
};

PollerImpl::PollerImpl() { _fd = epoll_create1(0); }

PollerImpl::~PollerImpl() { close(_fd); }

int PollerImpl::add(int fd) {
  epoll_event e;
  e.events = EPOLLIN;
  e.data.fd = fd;
  return epoll_ctl(_fd, EPOLL_CTL_ADD, fd, &e);
}

int PollerImpl::wait(epoll_event *e, int n) {
  return epoll_wait(_fd, e, n, -1);
}

std::shared_ptr<Poller> Poller::create() {
  return std::make_shared<PollerImpl>();
}