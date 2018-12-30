#define HANDLEBARS_FUNCTION_COMMON_MAX_SIZE 64
#include <handlebars/dispatcher.hpp>
#include <handlebars/handler.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

// signal type to handle events for
enum logging_signals
{
  open,
  write,
  close,
  select
};

// class which implements various event handlers and connects them to a dispatcher
struct logger : public handlebars::handler<logger, logging_signals, const std::string&>
{
  logger(const std::filesystem::path& log_directory)
    : m_log_directory(log_directory)
  {
    std::filesystem::create_directory(log_directory);
    connect(open, &logger::open_file);
    connect(write, &logger::write_data);
    connect(close, &logger::close_file);
    connect(select, &logger::select_file);
  }
  void open_file(const std::string& file)
  {
    if (m_open_files.count(file))
      return;
    m_open_files[file] = std::fstream(m_log_directory / file, std::ios::out);
  }
  void close_file(const std::string& file)
  {
    if (m_open_files.count(file))
      m_open_files[file].close();
  }

  void select_file(const std::string& file)
  {
    if (m_open_files.count(file))
      m_selected_file = file;
  }

  void write_data(const std::string& data) { m_open_files[m_selected_file] << data; }

private:
  std::filesystem::path m_log_directory;
  std::unordered_map<std::string, std::fstream> m_open_files;
  std::string m_selected_file;
};

int
main()
{
  logger app_logs("./logs");
  app_logs.push_event(open, "info.log");
  app_logs.push_event(open, "warnings.log");
  app_logs.push_event(open, "errors.log");
  app_logs.push_event(select, "info.log");
  app_logs.push_event(write, "application has started");
  app_logs.push_event(close, "info.log");
  app_logs.push_event(close, "warnings.log");
  app_logs.push_event(close, "errors.log");

  // std::cout << "Enter a newline to execute pending operations:\n";
  // std::cin.get();

  app_logs.respond();
  return 0;
}
