#pragma once

#include <map>
#include <memory>

struct Connection {
  virtual ~Connection() = default;
  virtual bool handle() = 0;
  virtual bool sendResp(int, uint32_t, uint16_t) = 0;
  virtual bool sendData(const std::string &) = 0;
  static std::shared_ptr<Connection> create(int);
};

extern std::map<int, std::shared_ptr<Connection>> connections;
