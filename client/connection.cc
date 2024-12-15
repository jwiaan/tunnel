#include "connection.h"
#include "../common.h"
#include "client.h"

constexpr char VER = 5;

enum class State { INIT, WAIT, CONNECTING, CONNECTED };

class ConnectionImpl : public Connection {
public:
  ConnectionImpl(int);
  ~ConnectionImpl();
  bool handle() override;
  bool sendResp(int, uint32_t, uint16_t) override;
  bool sendData(const std::string &) override;

private:
  bool onInit();
  bool onReq();
  bool onData();

private:
  int _fd, _to = -1;
  State _state = State::INIT;
  std::map<State, std::function<bool()>> _functions;
};

ConnectionImpl::ConnectionImpl(int fd)
    : _fd(fd),
      _functions{{State::INIT, std::bind(&ConnectionImpl::onInit, this)},
                 {State::WAIT, std::bind(&ConnectionImpl::onReq, this)},
                 {State::CONNECTED, std::bind(&ConnectionImpl::onData, this)}} {
  std::cout << __func__ << " " << _fd << std::endl;
}

ConnectionImpl::~ConnectionImpl() {
  close(_fd);
  std::cout << __func__ << " " << _fd << std::endl;
}

bool ConnectionImpl::handle() { return _functions.at(_state)(); }

bool ConnectionImpl::sendResp(int from, uint32_t host, uint16_t port) {
  char b[10] = {};
  b[0] = VER;
  b[1] = from > -1 ? 0 : 1;
  b[3] = 1;
  sprintf(&b[4], "%u", host);
  sprintf(&b[8], "%hu", port);
  if (!call(write, _fd, b, 10))
    return false;

  _to = from;
  _state = State::CONNECTED;
  return from > -1;
}

bool ConnectionImpl::sendData(const std::string &s) {
  return call(write, _fd, s.data(), s.size());
}

bool ConnectionImpl::onInit() {
  char a[2];
  if (!call(read, _fd, a, 2))
    return false;

  assert(a[0] == VER);
  char b[256];
  if (!call(read, _fd, b, a[1]))
    return false;

  a[1] = 0;
  if (!call(write, _fd, a, 2))
    return false;

  _state = State::WAIT;
  return true;
}

bool ConnectionImpl::onReq() {
  char b[5];
  if (!call(read, _fd, b, 5))
    return false;

  assert(b[0] == VER);
  assert(b[1] == 1);
  assert(b[3] == 3);
  char host[256] = {};
  if (!call(read, _fd, host, b[4]))
    return false;

  uint16_t port;
  if (!call(read, _fd, &port, 2))
    return false;

  std::ostringstream oss;
  oss << ntohs(port);
  if (!Client::get().sendReq(_fd, host, oss.str()))
    return false;

  _state = State::CONNECTING;
  return true;
}

bool ConnectionImpl::onData() {
  char b[4096];
  auto n = read(_fd, b, sizeof(b));
  if (n <= 0)
    return false;

  if (!Client::get().sendData(_to, std::string(b, n)))
    return false;

  return true;
}

std::shared_ptr<Connection> Connection::create(int fd) {
  return std::make_shared<ConnectionImpl>(fd);
}

std::map<int, std::shared_ptr<Connection>> connections;
