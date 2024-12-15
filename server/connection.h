#pragma once

#include <memory>

struct Server;

struct Connection {
  virtual ~Connection() = default;
  virtual bool handle() = 0;
  virtual bool sendData(const std::string &) = 0;
  static std::shared_ptr<Connection> create(int, int, std::weak_ptr<Server>);
};
