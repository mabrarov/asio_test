#if defined(WIN32)
#include <tchar.h>
#endif

#include <boost/asio.hpp>
#include <system_error>
#include <thread>
#include <iostream>
#include <functional>

constexpr unsigned short port = 12000;
namespace asio = boost::asio;
using std::error_code;
using asio::ip::tcp;

void runContext(asio::io_context& io_context)
{
  {
    std::stringstream ss;
    ss << "New thread: " << std::this_thread::get_id() << '\n';
    std::cout << ss.str();
  }
  io_context.run();
  {
    std::stringstream ss;
    ss << "Stopping thread: " << std::this_thread::get_id() << '\n';
    std::cout << ss.str();
  }
}

class server
{
public:
  template <typename Executor>
  explicit server(Executor executor) : acceptor_(executor,
      {asio::ip::make_address_v4("127.0.0.1"), port}), stopped_(false)
  {
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
  }

  void startAccepting()
  {
    acceptor_.listen();
    acceptLoop();
  }

  void stop()
  {
    asio::post(acceptor_.get_executor(), [this]()
    {
      std::cout << "Stopping server\n";
      acceptor_.cancel();
      stopped_ = true;
      std::cout << "Server stopped\n";
    });
  }

private:
  void acceptLoop()
  {
    acceptor_.async_accept([this](error_code errorCode, tcp::socket peer)
    {
      if (!errorCode)
      {
        std::stringstream ss;
        ss << "Accept: peer " << peer.remote_endpoint() << '\n';
        std::cout << ss.str();
        if (stopped_)
        {
          std::cout << "Too late - the server was stopped\n";
        }
        else
        {
          acceptLoop();
        }
      }
      else
      {
        std::stringstream ss;
        ss << "Accept: error " << errorCode.value() << '\n';
        std::cout << ss.str();
      }
    });
  }

  tcp::acceptor acceptor_;
  bool stopped_;
};

#if defined(WIN32)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
  setvbuf(stdout, NULL, _IONBF, 0);
  asio::io_context context;

  // run server
  server server{make_strand(context)};
  server.startAccepting();

  // run client
  tcp::socket socket{make_strand(context)};
  std::future<void> res = socket
      .async_connect({asio::ip::make_address_v4("127.0.0.1"), port},
          asio::use_future);

  std::thread t1(runContext, std::ref(context));
  std::thread t2(runContext, std::ref(context));

  res.get();

  server.stop();

  t1.join();
  t2.join();
  std::cout << "Everything shutdown\n";
}
