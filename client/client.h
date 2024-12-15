#pragma once

#include <string>

struct Client {
  virtual ~Client() = default;
  virtual int connect(int) = 0;
  virtual void handle() = 0;
  virtual bool sendReq(int, const std::string &, const std::string &) = 0;
  virtual bool sendData(int, const std::string &) = 0;
  static Client &get();
};
