#if defined(WIN32)
#include <tchar.h>
#endif

#include <cstdlib>
#include <cstddef>
#include <utility>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <exception>
#include <iostream>
#include <boost/system/error_code.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/asio.hpp>
#include <latch.hpp>

namespace strand_test {

typedef boost::asio::deadline_timer deadline_timer;

std::size_t post_dec_if_not_zero(std::atomic_size_t& counter);

void post_handler(bool use_strand_wrap,
    boost::asio::io_context& io_context,
    boost::asio::io_context::strand& strand,
    std::atomic_size_t& current_handlers,
    std::atomic_size_t& pending_handlers,
    std::atomic_bool& parallel_handlers,
    const std::chrono::milliseconds& handler_duration);

void async_wait(deadline_timer& timer,
    bool use_strand_wrap,
    boost::asio::io_context& io_context,
    boost::asio::io_context::strand& strand,
    std::atomic_size_t& current_handlers,
    std::atomic_size_t& pending_handlers,
    std::atomic_bool& parallel_handlers,
    const std::chrono::milliseconds& handler_duration);

class handler
{
public:
  handler(bool use_strand_wrap,
      boost::asio::io_context& io_context,
      boost::asio::io_context::strand& strand,
      std::atomic_size_t& current_handlers,
      std::atomic_size_t& pending_handlers,
      std::atomic_bool& parallel_handlers,
      const std::chrono::milliseconds& handler_duration);

  void operator()(const boost::system::error_code& ignored
      = boost::system::error_code());

private:
  handler& operator=(const handler&);

  bool use_strand_wrap_;
  boost::asio::io_context& io_context_;
  boost::asio::io_context::strand& strand_;
  std::atomic_size_t& current_handlers_;
  std::atomic_size_t& pending_handlers_;
  std::atomic_bool& parallel_handlers_;
  std::chrono::milliseconds handler_duration_;
};

const char* help_option_name                   = "help";
const char* use_strand_wrap_option_name        = "use-strand-wrap";
const char* thread_num_option_name             = "threads";
const char* handler_duration_option_name       = "duration";
const char* init_handler_num_option_name       = "init";
const char* concurrent_handler_num_option_name = "concurrent";
const char* strand_handler_num_option_name     = "strand";
const char* timer_handler_num_option_name      = "timer";

boost::program_options::options_description build_program_options_description(
    std::size_t hardware_concurrency);

#if defined(WIN32)
boost::program_options::variables_map parse_program_options(
    const boost::program_options::options_description& options_description,
    int argc, _TCHAR* argv[]);
#else
boost::program_options::variables_map parse_program_options(
    const boost::program_options::options_description& options_description,
    int argc, char* argv[]);
#endif

} // strand_test namespace

#if defined(WIN32)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
  try
  {
    auto po_description = strand_test::build_program_options_description(
        boost::numeric_cast<std::size_t>(std::thread::hardware_concurrency()));
    auto po_values = strand_test::parse_program_options(po_description, argc, argv);
    if (po_values.count(strand_test::help_option_name))
    {
      std::cout << po_description;
      return EXIT_SUCCESS;
    }

    std::size_t thread_num =
        po_values[strand_test::thread_num_option_name].as<std::size_t>();
    if (!thread_num)
    {
      throw boost::program_options::error(
          "number of threads should be positive integer");
    }
    bool use_strand_wrap =
        po_values[strand_test::use_strand_wrap_option_name].as<bool>();
    std::chrono::milliseconds handler_duration(
        po_values[strand_test::handler_duration_option_name].as<std::size_t>());
    std::size_t initial_handler_num =
        po_values[strand_test::init_handler_num_option_name].as<std::size_t>();
    std::size_t concurrently_posted_handler_num =
        po_values[strand_test::concurrent_handler_num_option_name].as<std::size_t>();
    std::atomic_size_t strand_posted_handlers(
        po_values[strand_test::strand_handler_num_option_name].as<std::size_t>());
    std::atomic_size_t timer_posted_handlers(
        po_values[strand_test::timer_handler_num_option_name].as<std::size_t>());

    std::atomic_size_t current_handlers(0);
    std::atomic_bool parallel_handlers(false);
    boost::asio::io_context io_context(boost::numeric_cast<int>(thread_num));
    boost::asio::io_context::strand strand(io_context);
    for (std::size_t i = 0; i < initial_handler_num; ++i)
    {
      strand_test::post_handler(use_strand_wrap,
          io_context,
          strand,
          current_handlers,
          strand_posted_handlers,
          parallel_handlers,
          handler_duration);
    }
    for (std::size_t i = 0; i < concurrently_posted_handler_num; ++i)
    {
      boost::asio::post(io_context, [use_strand_wrap,
          &io_context,
          &strand,
          &current_handlers,
          &strand_posted_handlers,
          &parallel_handlers,
          handler_duration]()
          {
            strand_test::post_handler(use_strand_wrap,
                io_context,
                strand,
                current_handlers,
                strand_posted_handlers,
                parallel_handlers,
                handler_duration);
          });
    }
    strand_test::deadline_timer timer(io_context);
    timer.expires_at(boost::posix_time::pos_infin);
    for (std::size_t i = 0; i < timer_posted_handlers; ++i)
    {
      strand_test::async_wait(timer,
          use_strand_wrap,
          io_context,
          strand,
          current_handlers,
          strand_posted_handlers,
          parallel_handlers,
          handler_duration);
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
    timer.expires_at(boost::posix_time::neg_infin);
    for (std::size_t i = 0; i < thread_num; ++i)
    {
      threads[i].join();
    }

    return parallel_handlers ? EXIT_FAILURE : EXIT_SUCCESS;
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
} // main

std::size_t strand_test::post_dec_if_not_zero(std::atomic_size_t& counter)
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

void strand_test::post_handler(bool use_strand_wrap,
    boost::asio::io_context& io_context,
    boost::asio::io_context::strand& strand,
    std::atomic_size_t& current_handlers,
    std::atomic_size_t& pending_handlers,
    std::atomic_bool& parallel_handlers,
    const std::chrono::milliseconds& handler_duration)
{
  if (use_strand_wrap)
  {
    boost::asio::post(io_context, strand.wrap(
        handler(use_strand_wrap,
            io_context,
            strand,
            current_handlers,
            pending_handlers,
            parallel_handlers,
            handler_duration)));
  }
  else
  {
    boost::asio::post(io_context, boost::asio::bind_executor(strand,
        handler(use_strand_wrap,
            io_context,
            strand,
            current_handlers,
            pending_handlers,
            parallel_handlers,
            handler_duration)));
  }
} // strand_test::post_handler

void strand_test::async_wait(deadline_timer& timer,
    bool use_strand_wrap,
    boost::asio::io_context& io_context,
    boost::asio::io_context::strand& strand,
    std::atomic_size_t& current_handlers,
    std::atomic_size_t& pending_handlers,
    std::atomic_bool& parallel_handlers,
    const std::chrono::milliseconds& handler_duration)
{
  if (use_strand_wrap)
  {
    timer.async_wait(strand.wrap(
        handler(use_strand_wrap,
            io_context,
            strand,
            current_handlers,
            pending_handlers,
            parallel_handlers,
            handler_duration)));
  }
  else
  {
    timer.async_wait(boost::asio::bind_executor(strand,
        handler(use_strand_wrap,
            io_context,
            strand,
            current_handlers,
            pending_handlers,
            parallel_handlers,
            handler_duration)));
  }
} // strand_test::async_wait

strand_test::handler::handler(bool use_strand_wrap,
    boost::asio::io_context& io_context,
    boost::asio::io_context::strand& strand,
    std::atomic_size_t& current_handlers,
    std::atomic_size_t& pending_handlers,
    std::atomic_bool& parallel_handlers,
    const std::chrono::milliseconds& handler_duration)
    : use_strand_wrap_(use_strand_wrap)
    , io_context_(io_context)
    , strand_(strand)
    , current_handlers_(current_handlers)
    , pending_handlers_(pending_handlers)
    , parallel_handlers_(parallel_handlers)
    , handler_duration_(handler_duration)
{}

void strand_test::handler::operator()(const boost::system::error_code&)
{
  if (current_handlers_++)
  {
    parallel_handlers_ = true;
  }
  if (post_dec_if_not_zero(pending_handlers_))
  {
    post_handler(use_strand_wrap_,
        io_context_,
        strand_,
        current_handlers_,
        pending_handlers_,
        parallel_handlers_,
        handler_duration_);
  }
  std::this_thread::sleep_for(handler_duration_);
  --current_handlers_;
} // strand_test::handler::operator()

boost::program_options::options_description strand_test::build_program_options_description(
    std::size_t hardware_concurrency)
{
  boost::program_options::options_description description("Allowed options");
  description.add_options()
      (
          help_option_name,
          "produce help message"
      )
      (
          use_strand_wrap_option_name,
          boost::program_options::value<bool>()->default_value(true),
          "use boost::asio::io_context::strand::wrap instead of boost::asio::bind_executor"
      )
      (
          thread_num_option_name,
          boost::program_options::value<std::size_t>()->default_value(hardware_concurrency),
          "number of threads"
      )
      (
          handler_duration_option_name,
          boost::program_options::value<std::size_t>()->default_value(200),
          "duration of handler, milliseconds"
      )
      (
          init_handler_num_option_name,
          boost::program_options::value<std::size_t>()->default_value(500),
          "number of initially posted handlers"
      )
      (
          concurrent_handler_num_option_name,
          boost::program_options::value<std::size_t>()->default_value(500),
          "number of concurrently posted handlers"
      )
      (
          strand_handler_num_option_name,
          boost::program_options::value<std::size_t>()->default_value(500),
          "number of handlers posted from strand"
      )
      (
          timer_handler_num_option_name,
          boost::program_options::value<std::size_t>()->default_value(500),
          "number of handlers posted through deadline_timer"
      );
  return std::move(description);
} // strand_test::build_program_options_description

#if defined(WIN32)
boost::program_options::variables_map strand_test::parse_program_options(
    const boost::program_options::options_description& options_description,
    int argc, _TCHAR* argv[])
#else
boost::program_options::variables_map strand_test::parse_program_options(
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
