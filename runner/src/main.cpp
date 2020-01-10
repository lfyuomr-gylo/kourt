#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <nlohmann/json.hpp>

#include "logging.h"
#include "interceptors.h"

void PipeStdoutAndStderrToFiles(const nlohmann::json &config) {
  // TODO: handle syscall errors
  std::string stdou_file_name = config.contains("stdoutFile")
      ? config["stdoutFile"]
      : "stdout.txt";
  int stdout_file = creat(stdou_file_name.c_str(), 0644);
  dup2(stdout_file, 1);
  close(stdout_file);

  std::string stderr_file_name = config.contains("stderrFile")
      ? config["stderrFile"]
      : "stderr.txt";
  int stderr_file = creat(stderr_file_name.c_str(), 0644);
  dup2(stderr_file, 2);
  close(stderr_file);
}

void printExitStatus(int exit_status, const nlohmann::json &config) {
  std::string out_file_name = config.contains("exitStatusFile")
      ? config["exitStatusFile"]
      : "exit-status.json";
  std::ofstream out(out_file_name, std::ofstream::out | std::ofstream::trunc);

  nlohmann::json json;
  if (WIFEXITED(exit_status)) {
    json["exitCode"] = WEXITSTATUS(exit_status);
  } else if (WIFSIGNALED(exit_status)) {
    json["signal"] = WTERMSIG(exit_status);
  } else {
    std::cerr << "Unexpected child exit status: " << exit_status << std::endl;
  }
  out << json;
}

void InitInterceptors(Tracee &tracee, const nlohmann::json &config, std::vector<SyscallInterceptor *> *result) {
  if (config.contains("interceptors")) {
    nlohmann::json interceptors = config["interceptors"];
    for (auto &interceptor : interceptors) {
      result->push_back(CreateInterceptor(interceptor["name"], tracee));
    }
  }
}

int main(int argc, char **argv) {
  // argv[1] --- path to the config file
  // argv[2] --- executable to run
  // argv[3]..argv[argc-1] --- cmd arguments to the executable
  // argv[argc] --- NULL according to paragraph 5.1.2.2.1 of the C language Standard

  nlohmann::json config;
  {
    std::ifstream config_stream(argv[1]);
    if (config_stream.peek() != std::ifstream::traits_type::eof()) {
      // read only if the config is non-empty
      config_stream >> config;
      std::cout << "Successfully loaded config " << config << std::endl;
    }
  }
  pid_t child_pid = fork();
  if (0 == child_pid) {
    // child
    PipeStdoutAndStderrToFiles(config);
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    execv(argv[2], argv + 2);
    perror("execv");
    exit(1);
  } else if (child_pid > 0) {
    // parent
    Tracee tracee(child_pid);
    std::vector<SyscallInterceptor *> interceptors;
    InitInterceptors(tracee, config, &interceptors);
    int child_status = waitpid(child_pid, &child_status, 0); // this is SIGTRAP-caused stop
    bool before_syscall = false; // initially the child is stopped due to SIGTRAP
    for (bool keep_tracing = true; keep_tracing;) {
      tracee.Ptrace(PTRACE_SYSCALL, nullptr, nullptr);
      child_status = tracee.Wait();
      keep_tracing = WIFSTOPPED(child_status);
      if (keep_tracing) {
        before_syscall = !before_syscall;
        for (auto interceptor : interceptors) {
          if (before_syscall) {
            interceptor->BeforeSyscallEntered();
          } else {
            interceptor->AfterSyscallReturned();
          }
        }
      }
    }
    printExitStatus(child_status, config);
  } else {
        // TODO: handle fork failure correctly
        exit(1);
    }
}
