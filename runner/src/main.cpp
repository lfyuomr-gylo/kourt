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
#include "tracee_controller.h"

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

void InitInterceptors(Tracee &tracee,
                      const nlohmann::json &config,
                      std::vector<std::unique_ptr<StoppedTraceeInterceptor>> *result) {
  if (config.contains("interceptors")) {
    nlohmann::json interceptors = config["interceptors"];
    for (auto &interceptor : interceptors) {
      result->push_back(std::move(CreateInterceptor(interceptor["name"], tracee)));
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
      INFO("Successfully loaded configuration %s", config.dump(2, ' ').c_str())
    } else {
      WARN("Failed to load configuration from config file %s. It either does not exist or is empty", argv[1])
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
    std::vector<std::unique_ptr<StoppedTraceeInterceptor>> interceptors;
    InitInterceptors(tracee, config, &interceptors);
    TraceeController controller(tracee, std::move(interceptors));
    int child_status = controller.ExecuteTracee();
    printExitStatus(child_status, config);
  } else {
    // TODO: handle fork failure correctly
    exit(1);
  }
}
