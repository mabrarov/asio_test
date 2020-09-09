#if defined(WIN32)
#include <tchar.h>
#endif

#include <cstdlib>
#include <string>
#include <thread>
#include <vector>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/asio.hpp>

#if defined(WIN32)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
  std::size_t thread_num = argc > 1
      ? boost::numeric_cast<std::size_t>(std::stol(argv[1]))
      : boost::numeric_cast<std::size_t>(std::thread::hardware_concurrency());
  boost::asio::io_context io_context(boost::numeric_cast<int>(thread_num));
  std::vector<std::thread> threads;
  threads.reserve(thread_num);
  for (std::size_t i = 0; i < thread_num; ++i)
  {
    threads.emplace_back([&io_context]()
    {
      io_context.run();
    });
  }

  for (std::size_t i = 0; i < thread_num; ++i)
  {
    threads[i].join();
  }
  return EXIT_SUCCESS;
}
