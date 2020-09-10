#ifndef ASIO_TEST_LATCH_HPP
#define ASIO_TEST_LATCH_HPP

#include <cstddef>
#include <mutex>
#include <condition_variable>
#include <boost/noncopyable.hpp>

namespace asio_test {

class latch : private boost::noncopyable
{
private:
  typedef std::mutex                   mutex_type;
  typedef std::unique_lock<mutex_type> lock_type;
  typedef std::lock_guard<mutex_type>  lock_guard_type;
  typedef std::condition_variable      condition_variable_type;

public:
  typedef std::size_t value_type;

  explicit latch(value_type value = 0);
  void count_down_and_wait();

private:
  value_type count_down();
  void wait();

  mutex_type mutex_;
  condition_variable_type condition_variable_;
  value_type value_;
};

}

#endif // ASIO_TEST_LATCH_HPP
