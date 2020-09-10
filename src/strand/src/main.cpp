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
#include <latch.hpp>

namespace {

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

void post_handler(bool use_strand_wrap,
    boost::asio::io_context& io_context,
    boost::asio::io_context::strand& strand,
    std::atomic_size_t& current_handlers,
    std::atomic_size_t& pending_handlers,
    std::atomic_bool& parallel_handlers,
    const std::chrono::milliseconds& handler_duration)
{
  auto handler = [use_strand_wrap, &io_context, &strand, &current_handlers,
      &pending_handlers, &parallel_handlers, handler_duration]()
  {
    if (current_handlers++)
    {
      parallel_handlers = true;
    }
    if (post_dec_if_not_zero(pending_handlers))
    {
      post_handler(use_strand_wrap,
          io_context,
          strand,
          current_handlers,
          pending_handlers,
          parallel_handlers,
          handler_duration);
    }
    std::this_thread::sleep_for(handler_duration);
    --current_handlers;
  };
  if (use_strand_wrap)
  {
    boost::asio::post(io_context,strand.wrap(std::move(handler)));
  }
  else
  {
    boost::asio::post(io_context,
        boost::asio::bind_executor(strand, std::move(handler)));
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
  std::size_t initial_handler_num = argc > 2
      ? boost::numeric_cast<std::size_t>(std::stol(argv[2]))
      : static_cast<std::size_t>(500);
  std::size_t concurrently_posted_handler_num = argc > 3
      ? boost::numeric_cast<std::size_t>(std::stol(argv[3]))
      : static_cast<std::size_t>(500);
  std::atomic_size_t strand_posted_handlers(argc > 4
      ? boost::numeric_cast<std::size_t>(std::stol(argv[4]))
      : static_cast<std::size_t>(500));
  std::chrono::milliseconds handler_duration(argc > 5
      ? boost::numeric_cast<std::size_t>(std::stol(argv[5]))
      : static_cast<std::size_t>(200));
  std::size_t thread_num = argc > 6
      ? boost::numeric_cast<std::size_t>(std::stol(argv[6]))
      : boost::numeric_cast<std::size_t>(std::thread::hardware_concurrency());

  std::atomic_size_t current_handlers(0);
  std::atomic_bool parallel_handlers(false);
  boost::asio::io_context io_context(boost::numeric_cast<int>(thread_num));
  boost::asio::io_context::strand strand(io_context);
  for (std::size_t i = 0; i < initial_handler_num; ++i)
  {
    post_handler(use_strand_wrap,
        io_context,
        strand,
        current_handlers,
        strand_posted_handlers,
        parallel_handlers,
        handler_duration);
  }
  for (std::size_t i = 0; i < concurrently_posted_handler_num; ++i)
  {
    boost::asio::post(io_context, [use_strand_wrap, &io_context, &strand,
        &current_handlers, &strand_posted_handlers, &parallel_handlers,
        handler_duration]()
    {
      post_handler(use_strand_wrap,
          io_context,
          strand,
          current_handlers,
          strand_posted_handlers,
          parallel_handlers,
          handler_duration);
    });
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

  return parallel_handlers ? EXIT_FAILURE : EXIT_SUCCESS;
}
