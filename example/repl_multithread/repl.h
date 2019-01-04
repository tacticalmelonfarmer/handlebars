#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

struct repl
{
  repl()
    : m_running(true)
    , m_step(0)
  {
    m_prompt.resize(10, ' ');
    m_prompt[0] = '-';
  }

  std::string& step_prompt()
  {
    std::rotate(m_prompt.begin(), m_prompt.begin() + 1, m_prompt.end());
    return m_prompt;
  }

  void operator()(std::ostream& output, std::mutex& mutex, std::string& input)
  {
    using namespace std::chrono_literals;
    std::unique_lock<std::mutex> lock(mutex, std::defer_lock);
    while (m_running) {
      lock.lock();
      output << '\r'; // clear console line
      std::this_thread::sleep_for(250ms);
      output << step_prompt();
      if (input == "quit")
        m_running = false;
      lock.unlock();
    }
  }

private:
  bool m_running;
  size_t m_step;
  std::string m_prompt;
};