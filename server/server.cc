#include "server.h"
#include "../common.h"
#include "../poller.h"
#include "../tunnel.pb.h"
#include "connection.h"

struct Resp {
  int to, from;
};

class ServerImpl : public Server,
                   public std::enable_shared_from_this<ServerImpl> {
public:
  ServerImpl(SSL *);
  ~ServerImpl();
  void start() override;
  bool sendData(int, const std::string &) override;

private:
  void handle();
  void onReq(const tunnel::Req &);
  void onData(const tunnel::Data &);
  void connect(const tunnel::Req &);
  void sendResp();

private:
  SSL *_ssl;
  int _pipe[2];
  std::shared_ptr<Poller> _poller;
  Functions<ServerImpl> _functions;
  std::map<int, std::shared_ptr<Connection>> _connections;
};

ServerImpl::ServerImpl(SSL *ssl) : _ssl(ssl), _poller(Poller::create()) {
  pipe(_pipe);
  _functions = ::create(&ServerImpl::onReq, &ServerImpl::onData);
}

ServerImpl::~ServerImpl() {
  close(_pipe[0]);
  close(_pipe[1]);
  close(SSL_get_fd(_ssl));
  SSL_free(_ssl);
}

void ServerImpl::onReq(const tunnel::Req &msg) {
  auto f = std::bind(&ServerImpl::connect, this, std::placeholders::_1);
  std::thread t(f, msg);
  t.detach();
}

void ServerImpl::onData(const tunnel::Data &msg) {
  _connections.at(msg.to())->sendData(msg.data());
}

bool ServerImpl::sendData(int to, const std::string &data) {
  tunnel::Data msg;
  msg.set_to(to);
  msg.set_data(data);
  return sendMsg(_ssl, msg);
}

void ServerImpl::connect(const tunnel::Req &msg) {
  int fd = -1;
  addrinfo h, *l;
  h.ai_family = AF_INET;
  h.ai_socktype = SOCK_STREAM;
  auto e = getaddrinfo(msg.host().c_str(), msg.port().c_str(), &h, &l);
  if (e) {
    std::cerr << gai_strerror(e) << std::endl;
    goto done;
  }

  for (auto a = l; a; a = a->ai_next) {
    fd = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
    if (fd < 0) {
      perror("socket");
      continue;
    }

    if (!::connect(fd, a->ai_addr, a->ai_addrlen)) {
      freeaddrinfo(l);
      goto done;
    }

    perror("connect");
    close(fd);
  }

  fd = -1;
done:
  Resp resp;
  resp.to = msg.from();
  resp.from = fd;
  write(_pipe[1], &resp, sizeof(resp));
}

void ServerImpl::handle() {
  std::string s;
  auto t = readMsg(_ssl, s);
  _functions.at(t)(weak_from_this(), s);
}

void ServerImpl::sendResp() {
  Resp resp;
  read(_pipe[0], &resp, sizeof(resp));

  auto fd = resp.from;
  sockaddr_in addr = {};
  if (fd != -1) {
    socklen_t len = sizeof(addr);
    getsockname(fd, reinterpret_cast<sockaddr *>(&addr), &len);
    _connections[fd] = Connection::create(fd, resp.to, weak_from_this());
    _poller->add(fd);
  }

  tunnel::Resp msg;
  msg.set_to(resp.to);
  msg.set_from(resp.from);
  msg.set_host(addr.sin_addr.s_addr);
  msg.set_port(addr.sin_port);
  sendMsg(_ssl, msg);
}

void ServerImpl::start() {
  SSL_accept(_ssl);
  auto fd = SSL_get_fd(_ssl);
  _poller->add(fd);
  std::array<epoll_event, 1024> a;
  assert(a.size() == 1024);
  while (true) {
    auto n = _poller->wait(a.data(), a.size());
    assert(n > 0);
    for (auto i = 0; i < n; ++i) {
      if (a[i].data.fd == fd) {
        handle();
      } else if (a[i].data.fd == _pipe[0]) {
        sendResp();
      } else {
        _connections.at(a[i].data.fd)->handle();
      }
    }
  }
}

std::shared_ptr<Server> Server::create(SSL *ssl) {
  return std::make_shared<ServerImpl>(ssl);
}
