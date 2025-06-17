#include <csignal>
#include <cstdlib>
#include <librdb/error.hpp>
#include <librdb/process.hpp>
#include <memory>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

std::unique_ptr<rdb::process> rdb::process::launch(std::filesystem::path path) {

  pid_t pid = 0;

  if ((pid = fork()) < 0) {
    error::send_errno("Fork failed");
  }

  if (pid == 0) {
    // in child process
    // execute debugee
    if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0) {
      error::send_errno("Traceme failed");
    }
    if (execlp(path.c_str(), path.c_str(), nullptr) < 0) {
      error::send_errno("Execlp failed");
    }
  }
  std::unique_ptr<rdb::process> proc(
      new rdb::process(pid, /*terminate_on_end=*/true));
  proc->wait_on_signal();

  return proc;
}

std::unique_ptr<rdb::process> rdb::process::attach(pid_t pid) {

  if (pid <= 0) {
    error::send("Invalid pid");
  }
  if (ptrace(PTRACE_ATTACH, pid, /*addr=*/nullptr, /*data=*/nullptr) < 0) {
    error::send_errno("Couldn't attach to process");
  }
  std::unique_ptr<rdb::process> proc(
      new rdb::process(pid, /*terminate_on_end=*/false));
  proc->wait_on_signal();

  return proc;
}

void rdb::process::resume() {
  if (ptrace(PTRACE_CONT, pid_, nullptr, nullptr) < 0) {
    error::send_errno("Could not resume");
  }
  state_ = process_state::running;
}

rdb::stop_reason rdb::process::wait_on_signal() {
  int wait_status;
  int options = 0;

  if (waitpid(pid_, &wait_status, options) < 0) {
    error::send_errno("Waitpid failed");
  }
  stop_reason reason(wait_status);
  state_ = reason.reason;
  return reason;
}

rdb::stop_reason::stop_reason(int wait_status) {
  if (WIFEXITED(wait_status)) {
    reason = process_state::exited;
    info = WEXITSTATUS(wait_status);
  } else if (WIFSIGNALED(wait_status)) {
    reason = process_state::terminated;
    info = WTERMSIG(wait_status);
  } else if (WIFSTOPPED(wait_status)) {
    reason = process_state::stopped;
    info = WSTOPSIG(wait_status);
  }
}

rdb::process::~process() {
  if (pid_ > 0) {
    int status;

    if (state_ == process_state::running) {
      kill(pid_, SIGSTOP);
      waitpid(pid_, &status, 0);
    }
    ptrace(PTRACE_DETACH, pid_, nullptr, nullptr);
    kill(pid_, SIGCONT);

    if (terminate_on_end_) {
      kill(pid_, SIGINT);
      waitpid(pid_, &status, 0);
    }
  }
}
