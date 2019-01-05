#include <cctype>
#include <iostream>
#include <string>
#include <vector>

#include <handlebars/dispatcher.hpp>

std::vector<std::string>
tokenize(const std::string& input)
{
  std::string work;
  std::vector<std::string> result;
  for (const auto& c : input) {
    if (std::isspace(c)) {
      result.push_back(work);
      work.clear();
    } else {
      work += c;
    }
  }
  result.push_back(work);
  return result;
}

struct repl
{
  using events = handlebars::dispatcher<std::string, std::vector<std::string>>;
  repl()
    : m_step(0)
  {
    events::connect("help", [](auto argv) { std::cout << "valid commands: help quit echo\n"; });
    events::connect("quit", [this](auto argv) { this->m_active = false; });
    events::connect("echo", [](auto argv) {
      for (auto t = argv.begin() + 1; t != argv.end(); ++t) {
        std::cout << *t << '\n';
      }
      std::cout << std::endl;
    });
  }

  void operator()(const std::string& input)
  {
    if (input.size() > 0) {
      auto tokens = tokenize(input);
      events::push_event(tokens[0], tokens);
    }
    // do basic interpret step like: &&, ||, piping, multi command single data, etc...
    events::respond();
    std::cout << "[" + std::to_string(m_step++) + "]:>"; // new prompt
  }

  bool active() const { return m_active; }

private:
  bool m_active;
  size_t m_step;
};