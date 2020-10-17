#if defined(WIN32)
#include <tchar.h>
#endif

#include <cstdlib>
#include <cstddef>
#include <utility>
#include <exception>
#include <memory>
#include <iostream>
#include <algorithm>
#include <vector>
#include <set>
#include <chrono>
#include <thread>
#include <functional>
#include <boost/system/error_code.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <latch.hpp>

namespace {

class access_registrar
{
public:
  typedef std::function<void()> handler_type;

  explicit access_registrar(handler_type handler)
      : handler_(std::move(handler))
      , concurrent_(false)
  {}

  void enter()
  {
    auto thread_id = std::this_thread::get_id();
    bool concurrent;
    {
      std::lock_guard<mutex_type> mutex_guard(mutex_);
      concurrent_ |= threads_.size() != threads_.count(thread_id);
      threads_.insert(thread_id);
      concurrent = concurrent_;
    }
    if (concurrent)
    {
      handler_();
    }
  }

  void leave()
  {
    auto thread_id = std::this_thread::get_id();
    std::lock_guard<mutex_type> mutex_guard(mutex_);
    threads_.erase(threads_.find(thread_id));
  }

  bool concurrent() const
  {
    return concurrent_;
  }

private:
  typedef std::mutex mutex_type;

  bool concurrent_;
  mutex_type mutex_;
  std::multiset<std::thread::id> threads_;
  handler_type handler_;
};

class async_stream
{
public:
  typedef boost::asio::io_context::executor_type executor_type;

  async_stream(boost::asio::io_context& io_context, access_registrar& registrar, size_t size)
      : io_context_(io_context)
      , registrar_(registrar)
      , size_(size)
  {}

  executor_type get_executor()
  {
    return io_context_.get_executor();
  }

  template <typename ConstBufferSequence, typename WriteHandler>
  void async_write_some(const ConstBufferSequence& buffers, WriteHandler&& handler)
  {
    registrar_.enter();
    // Assuming asynchronous write completed successfully
    auto error = boost::system::error_code();
    auto transferred = (std::min)(size_, boost::asio::buffer_size(buffers));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    io_context_.post(boost::asio::detail::bind_handler(handler, error, transferred));
    registrar_.leave();
  }


  template <typename MutableBufferSequence, typename ReadHandler>
  void async_read_some(const MutableBufferSequence& buffers, ReadHandler&& handler)
  {
    registrar_.enter();
    // Assuming asynchronous read completed successfully
    auto error = boost::system::error_code();
    auto transferred = (std::min)(size_, boost::asio::buffer_size(buffers));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::fill_n(boost::asio::buffers_begin(buffers), transferred, 0);
    io_context_.post(boost::asio::detail::bind_handler(handler, error, transferred));
    registrar_.leave();
  }

private:
  boost::asio::io_context& io_context_;
  access_registrar& registrar_;
  std::size_t size_;
};

class client
{
public:
  client(boost::asio::io_context::strand& strand,
      access_registrar& registrar,
      std::size_t operations_num,
      std::size_t operation_size,
      std::size_t composed_operations_num)
      : strand_(strand)
      , stream_(strand.context(), registrar, operation_size)
      , receive_buffers_(operation_size * operations_num, 0)
      , send_buffers_(operation_size * operations_num, 0)
      , read_op_num_(composed_operations_num)
      , write_op_num_(composed_operations_num)
  {}

  void start()
  {
    strand_.post([this]()
    {
      this->start_read();
      this->start_write();
    });
  }

private:
  void start_read()
  {
    if (read_op_num_)
    {
      --read_op_num_;
      boost::asio::async_read(stream_, boost::asio::buffer(receive_buffers_), strand_.wrap(
          [this](const boost::system::error_code&, std::size_t)
          {
            this->start_read();
          }));
    }
  }

  void start_write()
  {
    if (write_op_num_)
    {
      --write_op_num_;
      boost::asio::async_write(stream_, boost::asio::buffer(send_buffers_), strand_.wrap(
          [this](const boost::system::error_code&, std::size_t)
          {
            this->start_write();
          }));
    }
  }

  boost::asio::io_context::strand& strand_;
  async_stream stream_;
  std::vector<char> receive_buffers_;
  std::vector<char> send_buffers_;
  std::size_t read_op_num_;
  std::size_t write_op_num_;
};

const char* thread_num_option_name     = "threads";
const char* stream_num_option_name     = "streams";
const char* operation_size_option_name = "size";
const char* operations_num_option_name = "operations";
const char* composed_operations_num_option_name = "repeats";

boost::program_options::options_description build_program_options_description(
    std::size_t hardware_concurrency)
{
  boost::program_options::options_description description("Allowed options");
  description.add_options()
      (
          thread_num_option_name,
          boost::program_options::value<std::size_t>()->default_value(hardware_concurrency),
          "number of threads"
      )
      (
          stream_num_option_name,
          boost::program_options::value<std::size_t>()->default_value(512),
          "number of streams"
      )
      (
          operation_size_option_name,
          boost::program_options::value<std::size_t>()->default_value(16),
          "max size of single asynchronous operation"
      )
      (
          operations_num_option_name,
          boost::program_options::value<std::size_t>()->default_value(1000),
          "number of asynchronous operations"
      )
      (
          composed_operations_num_option_name,
          boost::program_options::value<std::size_t>()->default_value(10),
          "number of asynchronous composed operations"
      );
  return std::move(description);
}

#if defined(WIN32)
boost::program_options::variables_map parse_program_options(
    const boost::program_options::options_description& options_description,
    int argc, _TCHAR* argv[])
#else
boost::program_options::variables_map parse_program_options(
    const boost::program_options::options_description& options_description,
    int argc, char* argv[])
#endif
{
  boost::program_options::variables_map values;
  boost::program_options::store(boost::program_options::parse_command_line(
      argc, argv, options_description), values);
  boost::program_options::notify(values);
  return std::move(values);
}

} // anonymous namespace

#if defined(WIN32)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
  try
  {
    auto po_description = build_program_options_description(
        boost::numeric_cast<std::size_t>(std::thread::hardware_concurrency()));
    auto po_values = parse_program_options(po_description, argc, argv);
    std::size_t thread_num = po_values[thread_num_option_name].as<std::size_t>();
    if (!thread_num)
    {
      throw boost::program_options::error(
          "number of threads should be positive integer");
    }
    std::size_t stream_num = po_values[stream_num_option_name].as<std::size_t>();
    std::size_t operation_size = po_values[operation_size_option_name].as<std::size_t>();
    if (!operation_size)
    {
      throw boost::program_options::error(
          "max size of single asynchronous operation should be positive integer");
    }
    std::size_t operations_num = po_values[operations_num_option_name].as<std::size_t>();
    if (!operations_num)
    {
      throw boost::program_options::error(
          "number of asynchronous operations should be positive integer");
    }
    std::size_t composed_operations_num = po_values[composed_operations_num_option_name].as<std::size_t>();
    if (!composed_operations_num)
    {
      throw boost::program_options::error(
          "number of asynchronous composed operations should be positive integer");
    }

    boost::asio::io_context io_context;
    boost::asio::io_context::strand strand(io_context);
    access_registrar registrar([&io_context]()
    {
      io_context.stop();
    });

    std::vector<std::unique_ptr<client>> clients;
    clients.reserve(stream_num);
    for (std::size_t i = 0; i != stream_num; ++i)
    {
      std::unique_ptr<client> c(new client(strand, registrar,
          operations_num, operation_size, composed_operations_num));
      c->start();
      clients.push_back(std::move(c));
    }

    asio_test::latch run_barrier(thread_num);
    std::vector<std::thread> threads;
    threads.reserve(thread_num);
    for (std::size_t i = 0; i < thread_num; ++i)
    {
      threads.emplace_back([&io_context, &run_barrier]()
      {
        run_barrier.count_down_and_wait();
        io_context.run();
      });
    }
    for (std::size_t i = 0; i < thread_num; ++i)
    {
      threads[i].join();
    }

    return registrar.concurrent() ? EXIT_FAILURE : EXIT_SUCCESS;
  }
  catch (const boost::program_options::error& e)
  {
    std::cerr << "Error reading options: " << e.what() << std::endl;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "Unknown error" << std::endl;
  }
  return EXIT_FAILURE;
}
