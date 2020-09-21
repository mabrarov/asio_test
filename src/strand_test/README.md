# Test for boost::asio::io_context::strand

Tests if handlers utilizing
[`boost::asio:io_context::strand`](https://www.boost.org/doc/libs/1_74_0/doc/html/boost_asio/reference/io_context__strand.html)
class are not executed in parallel.

Allowed options:

| Name | Value format | Default value | Meaning |
|------|--------------|---------------|---------|
| --help | | | Show help and exit |
| --use-strand-wrap | 0 or 1 | 1 | Use [`boost::asio::io_context::strand::wrap`](https://www.boost.org/doc/libs/1_74_0/doc/html/boost_asio/reference/io_context__strand/wrap.html) method instead of [`boost::asio::bind_executor`](https://www.boost.org/doc/libs/1_74_0/doc/html/boost_asio/reference/io_context__strand/wrap.html) function when wrapping handler with strand |
| --threads | Positive integer | Hardware concurrency detected by C++ standard library | Number of worker threads to run |
| --duration | Non-negative integer | 200 | Duration of handler, milliseconds |
| --init | Non-negative integer | 500 | Number of handlers posted initially, before starting worker threads |
| --concurrent | Non-negative integer | 500 | Number of handlers posted concurrently from worker threads |
| --strand | Non-negative | 500 | Number of handlers posted from code executed inside strand |
| --timer | Non-negative | 500 | Number of handlers executed using deadline_timer asynchronous wait |

Returns zero exit code if didn't detect any handlers executed through strand
running in parallel. Returns non-zero otherwise.
