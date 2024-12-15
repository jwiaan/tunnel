#include "client.h"
#include "../common.h"
#include "../tunnel.pb.h"
#include "connection.h"

class ClientImpl : public Client,
                   public std::enable_shared_from_this<ClientImpl> {
public:
  ClientImpl();
  int connect(int fd) override;
  void handle() override;
  bool sendReq(int, const std::string &, const std::string &) override;
  bool sendData(int, const std::string &) override;

private:
  void onResp(const tunnel::Resp &);
  void onData(const tunnel::Data &);

private:
  SSL *_ssl;
  Functions<ClientImpl> _functions;
};

ClientImpl::ClientImpl() {
  auto tls = TLS_client_method();
  auto ctx = SSL_CTX_new(tls);
  _ssl = SSL_new(ctx);
  _functions = ::create(&ClientImpl::onResp, &ClientImpl::onData);
}

int ClientImpl::connect(int fd) {
  SSL_set_fd(_ssl, fd);
  return SSL_connect(_ssl);
}

void ClientImpl::handle() {
  std::string s;
  auto t = readMsg(_ssl, s);
  assert(t);
  _functions.at(t)(weak_from_this(), s);
}

bool ClientImpl::sendReq(int from, const std::string &host,
                         const std::string &port) {
  tunnel::Req msg;
  msg.set_from(from);
  msg.set_host(host);
  msg.set_port(host);
  return sendMsg(_ssl, msg);
}

bool ClientImpl::sendData(int to, const std::string &data) {
  tunnel::Data msg;
  msg.set_to(to);
  msg.set_data(data);
  return sendMsg(_ssl, msg);
}

void ClientImpl::onResp(const tunnel::Resp &msg) {
  if (!connections.at(msg.to())->sendResp(msg.from(), msg.host(), msg.port()))
    connections.erase(msg.to());
}

void ClientImpl::onData(const tunnel::Data &msg) {
  if (!connections.at(msg.to())->sendData(msg.data()))
    connections.erase(msg.to());
}

Client &Client::get() {
  static ClientImpl c;
  return c;
}
