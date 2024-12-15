#pragma once

#include <arpa/inet.h>
#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <netdb.h>
#include <openssl/ssl.h>
#include <sstream>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>

struct Head {
  uint16_t type, size;
};

int start(int (*)(int, const sockaddr *, socklen_t), const char *, uint16_t);

uint16_t readMsg(SSL *, std::string &);

bool sendMsg(SSL *, uint16_t, const std::string &);

template <typename Msg> bool sendMsg(SSL *ssl, const Msg &msg) {
  return sendMsg(ssl, Msg::TYPE, msg.SerializeAsString());
}

template <typename Func, typename Desc, typename Buf>
bool call(Func f, Desc d, Buf *b, size_t n) {
  auto p = (char *)b;
  while (n) {
    auto a = f(d, p, n);
    if (a <= 0) {
      perror("");
      return false;
    }

    p += a;
    n -= a;
  }

  return true;
}

template <typename Obj>
using Function = std::function<void(std::weak_ptr<Obj>, const std::string &)>;

template <typename Obj> using Functions = std::map<uint16_t, Function<Obj>>;

template <typename Obj> inline Functions<Obj> create() { return {}; }

template <typename Obj, typename Msg, typename... Msgs>
Functions<Obj> create(void (Obj::*f)(const Msg &),
                      void (Obj::*...v)(const Msgs &)) {
  auto a = create<Obj>(v...);
  a[Msg::TYPE] = [f](std::weak_ptr<Obj> obj, const std::string &s) {
    Msg msg;
    msg.ParseFromArray(s.data(), s.size());
    (obj.lock().get()->*f)(msg);
  };

  return a;
}
