#include <handlebars/dispatcher.hpp>
#include <handlebars/handles.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

// signal type to handle events for
enum class signals
{
  open,
  write,
  close,
  select
};

// class which implements various event handlers and connects them to a dispatcher
struct logger : public tmf::hb::handles<logger, signals, const std::string&>
{
  logger(const std::filesystem::path& log_directory)
    : m_log_directory(log_directory)
  {
    std::filesystem::create_directory(log_directory);
    connect(signals::open, &logger::open_file);
    connect(signals::write, &logger::write_data);
    connect(signals::close, &logger::close_file);
    connect(signals::select, &logger::select_file);
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
  app_logs.push_event(signals::open, "info.log");
  app_logs.push_event(signals::open, "warnings.log");
  app_logs.push_event(signals::open, "errors.log");
  app_logs.push_event(signals::select, "info.log");
  app_logs.push_event(signals::write, "application has started");
  app_logs.push_event(signals::close, "info.log");
  app_logs.push_event(signals::close, "warnings.log");
  app_logs.push_event(signals::close, "errors.log");

  std::cout << "Enter a newline to execute pending operations:\n";
  std::cin.get();

  app_logs.respond();
  return 0;
}
