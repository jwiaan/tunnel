#include "connection.h"
#include "../common.h"
#include "server.h"

class ConnectionImpl : public Connection {
public:
  ConnectionImpl(int fd, int to, std::weak_ptr<Server> server)
      : _fd(fd), _to(to), _server(server) {}
  ~ConnectionImpl();
  bool handle() override;
  bool sendData(const std::string &) override;

private:
  int _fd, _to;
  std::weak_ptr<Server> _server;
};

ConnectionImpl::~ConnectionImpl() { close(_fd); }

bool ConnectionImpl::handle() {
  char b[4096];
  auto n = read(_fd, b, sizeof(b));
  if (n <= 0)
    return false;

  if (!_server.lock()->sendData(_to, std::string(b, n)))
    return false;

  return true;
}

bool ConnectionImpl::sendData(const std::string &s) {
  return call(write, _fd, s.data(), s.size());
}

std::shared_ptr<Connection> Connection::create(int fd, int peer,
                                               std::weak_ptr<Server> client) {
  return std::make_shared<ConnectionImpl>(fd, peer, client);
}