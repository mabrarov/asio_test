#if defined(WIN32)
#include <tchar.h>
#endif

#include <cstdlib>
#include <cstddef>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <utility>
#include <chrono>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

namespace {

class latch : private boost::noncopyable
{
private:
  typedef std::mutex                   mutex_type;
  typedef std::unique_lock<mutex_type> lock_type;
  typedef std::lock_guard<mutex_type>  lock_guard_type;
  typedef std::condition_variable      condition_variable_type;

public:
  typedef std::size_t value_type;

  explicit latch(value_type value = 0) : value_(value) {}

  void count_down_and_wait()
  {
    if (count_down())
    {
      wait();
    }
  }

private:
  value_type count_down()
  {
    lock_guard_type lock_guard(mutex_);
    if (value_)
    {
      --value_;
    }
    if (!value_)
    {
      condition_variable_.notify_all();
    }
    return value_;
  }

  void wait()
  {
    lock_type lock(mutex_);
    while (value_)
    {
      condition_variable_.wait(lock);
    }
  }

  mutex_type mutex_;
  condition_variable_type condition_variable_;
  value_type value_;
};

void post_handler(bool use_strand_wrap,
    boost::asio::io_context& io_context,
    boost::asio::io_context::strand& strand,
    std::atomic_size_t& current_handlers,
    std::atomic_size_t& pending_handlers,
    std::atomic_bool& concurrent_handlers_detected,
    const std::chrono::milliseconds& handler_duration);

std::size_t post_dec_if_not_zero(std::atomic_size_t& counter)
{
  std::size_t value = counter.load();
  while (value)
  {
    if (counter.compare_exchange_strong(value, value - 1))
    {
      return value;
    }
    std::this_thread::yield();
  }
  return 0;
}

class handler
{
public:
  handler(bool use_strand_wrap,
      boost::asio::io_context& io_context,
      boost::asio::io_context::strand& strand,
      std::atomic_size_t& current_handlers,
      std::atomic_size_t& pending_handlers,
      std::atomic_bool& concurrent_handlers_detected,
      const std::chrono::milliseconds& handler_duration)
    : use_strand_wrap_(use_strand_wrap)
    , io_context_(io_context)
    , strand_(strand)
    , current_handlers_(current_handlers)
    , pending_handlers_(pending_handlers)
    , concurrent_handlers_detected_(concurrent_handlers_detected)
    , handler_duration_(handler_duration)
  {}

  void operator()()
  {
    if (current_handlers_++)
    {
      concurrent_handlers_detected_ = true;
    }
    if (post_dec_if_not_zero(pending_handlers_))
    {
      post_handler(use_strand_wrap_,
          io_context_,
          strand_,
          current_handlers_,
          pending_handlers_,
          concurrent_handlers_detected_,
          handler_duration_);
    }
    std::this_thread::sleep_for(handler_duration_);
    --current_handlers_;
  }

private:
  bool use_strand_wrap_;
  boost::asio::io_context& io_context_;
  boost::asio::io_context::strand& strand_;
  std::atomic_size_t& current_handlers_;
  std::atomic_size_t& pending_handlers_;
  std::atomic_bool& concurrent_handlers_detected_;
  std::chrono::milliseconds handler_duration_;
};

void post_handler(bool use_strand_wrap,
    boost::asio::io_context& io_context,
    boost::asio::io_context::strand& strand,
    std::atomic_size_t& current_handlers,
    std::atomic_size_t& pending_handlers,
    std::atomic_bool& concurrent_handlers_detected,
    const std::chrono::milliseconds& handler_duration)
{
  handler h(use_strand_wrap,
      io_context,
      strand,
      current_handlers,
      pending_handlers,
      concurrent_handlers_detected,
      handler_duration);
  if (use_strand_wrap)
  {
    boost::asio::post(io_context, strand.wrap(std::move(h)));
  }
  else
  {
    boost::asio::post(io_context, boost::asio::bind_executor(strand, std::move(h)));
  }
}

}

#if defined(WIN32)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
  bool use_strand_wrap = argc <= 1 || std::stoi(argv[1]) != 0;
  std::size_t handler_num = argc > 2
      ? boost::numeric_cast<std::size_t>(std::stol(argv[2]))
      : static_cast<std::size_t>(1000);
  std::chrono::milliseconds handler_duration(argc > 3
      ? boost::numeric_cast<std::size_t>(std::stol(argv[3]))
      : static_cast<std::size_t>(200));
  std::size_t thread_num = argc > 4
      ? boost::numeric_cast<std::size_t>(std::stol(argv[4]))
      : boost::numeric_cast<std::size_t>(std::thread::hardware_concurrency());

  std::atomic_size_t current_handlers(0);
  std::atomic_size_t pending_handlers(handler_num / 10);
  std::atomic_bool concurrent_handlers_detected(false);
  boost::asio::io_context io_context(boost::numeric_cast<int>(thread_num));
  boost::asio::io_context::strand strand(io_context);
  for (std::size_t i = 0; i < handler_num - pending_handlers; ++i)
  {
    post_handler(use_strand_wrap,
        io_context,
        strand,
        current_handlers,
        pending_handlers,
        concurrent_handlers_detected,
        handler_duration);
  }

  latch run_barrier(thread_num);
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

  return concurrent_handlers_detected ? EXIT_FAILURE : EXIT_SUCCESS;
}
