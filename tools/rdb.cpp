#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <editline/readline.h>
#include <iostream>
#include <librdb/error.hpp>
#include <librdb/process.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {

void print_stop_reason(std::unique_ptr<rdb::process> &proc,
                       rdb::stop_reason reason) {
  std::cout << "Process " << proc->pid() << ' ';

  switch (reason.reason) {
  case rdb::process_state::exited:
    std::cout << "exited with status " << static_cast<int>(reason.info);
    break;
  case rdb::process_state::terminated:
    std::cout << "terminated with signal " << sigabbrev_np(reason.info);
    break;
  case rdb::process_state::stopped:
    std::cout << "stopped with signal " << sigabbrev_np(reason.info);
    break;
  }
  std::cout << std::endl;
}

std::unique_ptr<rdb::process> attach(int argc, const char **argv) {
  pid_t pid = 0;
  // Passing PID
  if (argc == 3 && argv[1] == std::string_view("-p")) {
    pid = std::atoi(argv[2]);
    return rdb::process::attach(pid);
  }
  // Passing program name
  else {
    const char *program_path = argv[1];
    return rdb::process::launch(program_path);
  }
}

std::vector<std::string> split(std::string_view str, char delimiter) {
  std::vector<std::string> out{};
  std::stringstream ss{std::string{str}};

  std::string item;

  while (std::getline(ss, item, delimiter)) {
    out.push_back(item);
  }
  return out;
}

// clang-format off
bool is_prefix(std::string_view str, std::string_view of) {
	if (str.size() > of.size()) return false;
	return std::equal(str.begin(), str.end(), of.begin());
}

void handle_command(std::unique_ptr<rdb::process>& proc, std::string_view line) {
  auto args = split(line, ' ');
  auto command = args[0];

  if (is_prefix(command, "continue")) {
		proc->resume();
		auto reason = proc->wait_on_signal();
		print_stop_reason(proc, reason);
  } else {
    std::cerr << "Unkown command\n";
  }
}

void main_loop(std::unique_ptr<rdb::process>&proc) {

  char *line = nullptr;
  while ((line = readline("rdb> ")) != nullptr) {
    std::string line_str;

    if (line == std::string_view("")) {
      free(line);
      if (history_length > 0) {
        line_str = history_list()[history_length - 1]->line;
      }
    } else {
			line_str = line;
      add_history(line);
      free(line);
    }
    if (!line_str.empty()) {
			try {
      handle_command(proc, line_str);
			}
			catch(const rdb::error& err) {
				std::cout << err.what() << "\n";
			}
    }
  }
}

} // namespace

int main(int argc, const char **argv) {
  if (argc == 1) {
    std::cerr << "No arguments given\n";
    return -1;
  }

	try {
		auto process = attach(argc, argv);
		main_loop(process);
	}
	catch(const rdb::error& err) {
		std::cout << err.what() << "\n";
	}

}
