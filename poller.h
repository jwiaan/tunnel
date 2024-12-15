#pragma once

#include <memory>
#include <sys/epoll.h>

struct Poller {
  virtual ~Poller() = default;
  virtual int add(int) = 0;
  virtual int wait(epoll_event *, int) = 0;
  static std::shared_ptr<Poller> create();
};