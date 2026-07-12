#include <algorithm>
#include <cstdlib>
#include <editline/readline.h>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <iostream>
#include <librdb/error.hpp>
#include <librdb/parser.hpp>
#include <librdb/process.hpp>
#include <librdb/register_info.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <type_traits>
#include <unistd.h>
#include <variant>
#include <vector>
#include <span>

namespace {

void print_stop_reason(const rdb::process &proc, rdb::stop_reason reason) {
  std::cout << "Process " << proc.pid() << ' ';

  std::string message;
  switch (reason.reason) {
  case rdb::process_state::exited:
    message =
        fmt::format("exited with status {}", static_cast<int>(reason.info));
    break;
  case rdb::process_state::terminated:
    message =
        fmt::format("terminated with signal {}", sigabbrev_np(reason.info));
    break;
  case rdb::process_state::stopped:
    message = fmt::format("stopped with signal {}", sigabbrev_np(reason.info));
    break;
  }
  fmt::print("Process {} {}\n", proc.pid(), message);
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

void print_help(const std::vector<std::string>& args) {
	if (args.size() == 1) {
		std::cerr << R"(
Available commands:
  continue	- Resume the process
  register	- Commands for operating on registers

)";
	} else if (is_prefix(args[1], "register")) {
		std::cerr << R"(
Available commands:
  read	- Same as 'read all'
  read <register>
  read all
  write <register> <value>

)";
	} else {
		std::cerr << "No help available on that\n";
	}
}

void handle_register_read(rdb::process& proc, std::vector<std::string>& args) {
	auto format = [](auto t) {
		if constexpr (std::is_floating_point_v<decltype(t)>) {
			return fmt::format("{}", t);
		} else if constexpr (std::is_integral_v<decltype(t)>) {
			return fmt::format("{:#0{}x}", t, sizeof(t) * 2 + 2);
		} else {
			return fmt::format("[{:#04x}]", fmt::join(std::span(reinterpret_cast<const unsigned char*>(t.data()), t.size()), ","));
		}
	};

	if(args.size() == 2 or (args.size() == 3 and args[2] == "all")) {
		for (auto& info: rdb::g_register_infos) {
			auto should_print = (args.size() == 3 or info.type == rdb::register_type::gpr) and info.name != "orig_rax";
			if (!should_print) 
				continue;

			auto value = proc.get_registers().read(info);
			fmt::print("{}:\t{}\n", info.name, std::visit(format, value));
		}
	} else if (args.size() == 3) {
			try {
				auto info = rdb::register_info_by_name(args[2]);
				auto value = proc.get_registers().read(info);
				fmt::print("{}:\t{}\n", info.name, std::visit(format, value));
			} catch(rdb::error& err) {
				std::cerr << "No such register\n";
				return;
			}
	} else {
		print_help({"help", "register"});
	}
}

rdb::registers::value parse_register_value(const rdb::register_info& info, std::string_view text) {
	try {
		if (info.format == rdb::register_format::uint) {
			switch (info.size) {
				case 1: return rdb::to_integral<std::uint8_t>(text, 16).value();
				case 2: return rdb::to_integral<std::uint16_t>(text, 16).value();
				case 4: return rdb::to_integral<std::uint32_t>(text, 16).value();
				case 8: return rdb::to_integral<std::uint64_t>(text, 16).value();
			}
		} else if (info.format == rdb::register_format::double_float) {
			return rdb::to_float<double>(text).value();
		} else if (info.format == rdb::register_format::long_double) {
			return rdb::to_float<long double>(text).value();
		} else if (info.format == rdb::register_format::vector) {
			if (info.size == 8) {
			return rdb::parse_vector<8>(text);
			} else if (info.size == 16) {
				return rdb::parse_vector<16>(text);
			}
		}
	} catch(...) {}
	rdb::error::send("Invalid format");
}

void handle_register_write(rdb::process& proc, const std::vector<std::string>& args) {
	if (args.size() != 4) {
		print_help({"help", "register"});
		return;
	}
	try {
		auto& info = rdb::register_info_by_name(args[2]);
		auto value = parse_register_value(info, args[3]);
		proc.get_registers().write(info, value);
	} catch(rdb::error& err) {
		std::cerr << err.what() << "\n";
		return;
	}
}

void handle_register_command(rdb::process& proc, std::vector<std::string>& args) {
	if (args.size() < 2) {
		print_help({"help", "register"});
		return;
	}
	if(is_prefix(args[1], "read")) {
		handle_register_read(proc, args);
	} else if(is_prefix(args[1], "write")) {
		handle_register_write(proc, args);
	} else {
		print_help({"help", "register"});
	}
}

void handle_command(std::unique_ptr<rdb::process>& proc, std::string_view line) {
  auto args = split(line, ' ');
  auto command = args[0];

  if (is_prefix(command, "continue")) {
		proc->resume();
		auto reason = proc->wait_on_signal();
		print_stop_reason(*proc, reason);
  } else if (is_prefix(command, "help")) {
		print_help(args);
	} else if (is_prefix(command, "register")) {
		handle_register_command(*proc, args);
	} else {
    std::cerr << "Unkown command\n";
  }
}

void main_loop(std::unique_ptr<rdb::process>&proc) {

  char *line = nullptr;
  while ((line = readline("(rdb) ")) != nullptr) {
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
