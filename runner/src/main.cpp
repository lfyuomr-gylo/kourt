#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <nlohmann/json.hpp>

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

int main(int argc, char **argv) {
  // argv[1] --- path to the config file
  // argv[2] --- executable to run
  // argv[3]..argv[argc-1] --- cmd arguments to the executable
  // argv[argc] --- NULL according to paragraph 5.1.2.2.1 of the C language Standard

  pid_t pid = fork();
  nlohmann::json config;
  {
    std::ifstream config_stream(argv[1]);
    if (config_stream.peek() != std::ifstream::traits_type::eof()) {
      // read only if the config is non-empty
      config_stream >> config;
    }
  }
  if (0 == pid) {
    // child
    PipeStdoutAndStderrToFiles(config);
    execv(argv[2], argv + 2);
    perror("execv");
    exit(1);
  } else if (pid > 0) {
    // parent
    int child_status;
    waitpid(pid, &child_status, 0);
    printExitStatus(child_status, config);
  } else {
        // TODO: handle fork failure correctly
        exit(1);
    }
}
