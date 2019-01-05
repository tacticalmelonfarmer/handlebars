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
  bool in_quote = false;
  for (auto c = input.begin(); c != input.end(); ++c) {
    if (c == input.end()) {
      break;
    }
    if (*c == '\\') { // for escaping character sequences
      work += *(++c);
      continue;
    }
    if (*c == '"') { // start and end a quoted token
      in_quote = (in_quote) ? false : true;
      continue;
    }
    if (std::isspace(*c) && !in_quote) { // end, push and then start a new token
      result.push_back(work);
      work.clear();
    } else { // append char to current token
      work += *c;
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