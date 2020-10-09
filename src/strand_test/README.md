# Test for boost::asio::io_context::strand

Tests if handlers utilizing
[`boost::asio:io_context::strand`](https://www.boost.org/doc/libs/1_74_0/doc/html/boost_asio/reference/io_context__strand.html)
class are not executed in parallel.

Allowed options:

| Name | Value format | Default value | Meaning |
|------|--------------|---------------|---------|
| --threads | Positive integer | Hardware concurrency detected by C++ standard library | Number of worker threads to run |
| --streams | Non-negative integer | 512 | Number of asynchronous streams sharing the same instance of `boost::asio:io_context::strand` |
| --size | Non-negative integer | 16 | Max size of single asynchronous operation, bytes |
| --operations | Non-negative integer | 1000 | Number of asynchronous operations in a single composed operation |

Returns zero exit code if didn't detect any handlers executed through strand running in parallel. Returns non-zero otherwise.
