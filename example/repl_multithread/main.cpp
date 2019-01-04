#include <chrono>
#include <iostream>
#include <thread>

#include "repl.h"

int
main()
{
  std::string input;
  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex, std::defer_lock);
  std::thread repl_thread(repl{}, std::ref(std::cout), std::ref(mutex), std::ref(input));
  while (input != "quit") {
    lock.lock();
    std::cin >> input;
    lock.unlock();
  }
  repl_thread.join();
  return 0;
}