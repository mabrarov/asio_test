#include <latch.hpp>

asio_test::latch::latch(value_type value) : value_(value) {}

void asio_test::latch::count_down_and_wait()
{
  if (count_down())
  {
    wait();
  }
}

asio_test::latch::value_type asio_test::latch::count_down()
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

void asio_test::latch::wait()
{
  lock_type lock(mutex_);
  while (value_)
  {
    condition_variable_.wait(lock);
  }
}
