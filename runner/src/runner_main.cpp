#include <kourt/runner/runner_main.h>

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

#include <kourt/runner/logging.h>
#include <kourt/runner/config.h>
#include <kourt/runner/interceptors.h>
#include <kourt/runner/tracee_controller.h>

const char *kDefaultStdoutFile = "stdout.txt";
const char *kDefaultStderrFile = "stderr.txt";
const char *kDefaultExitStatusFile = "exit-status.json";

const char *kStdoutFileKey = "stdoutFile";
const char *kStderrFileKey = "stderrFile";
const char *kExitStatusFileKey = "exitStatusFile";

static void PipeStdoutAndStderrToFiles(const nlohmann::json &config) {
  // TODO: handle syscall errors
  std::string stdou_file_name = config.value(kStdoutFileKey, kDefaultStdoutFile);
  int stdout_file = creat(stdou_file_name.c_str(), 0644);
  dup2(stdout_file, 1);
  close(stdout_file);

  std::string stderr_file_name = config.value(kStderrFileKey, kDefaultStderrFile);
  int stderr_file = creat(stderr_file_name.c_str(), 0644);
  dup2(stderr_file, 2);
  close(stderr_file);
}

static void PrintExitStatus(int exit_status, const nlohmann::json &config) {
  std::string out_file_name = config.value(kExitStatusFileKey, kDefaultExitStatusFile);
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

static void InitInterceptors(Tracee &tracee,
                             const nlohmann::json &config,
                             std::vector<std::unique_ptr<StoppedTraceeInterceptor>> *result) {
  auto interceptors = config.value("interceptors", nlohmann::json::array());
  for (auto &interceptor : interceptors) {
    result->push_back(std::move(CreateInterceptor(interceptor["name"], tracee)));
  }
}

void LaunchRunner(const nlohmann::json &config, const char *path_to_executable, char *const *executable_argv) {
  pid_t child_pid = fork();
  if (0 == child_pid) {
    // child
    PipeStdoutAndStderrToFiles(config);
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    execv(path_to_executable, executable_argv);
    perror("execv");
    exit(1);
  } else if (child_pid > 0) {
    // parent
    Tracee tracee(child_pid);
    std::vector<std::unique_ptr<StoppedTraceeInterceptor>> interceptors;
    InitInterceptors(tracee, config, &interceptors);
    TraceeController controller(tracee, std::move(interceptors));
    int child_status = controller.ExecuteTracee();
    PrintExitStatus(child_status, config);
  } else {
    int error_code = errno;
    throw std::runtime_error(std::string("Failed to fork due to error") + strerror(error_code));
  }
}

void ParseConfigAndLaunchRunner(const char *path_to_config,
                                const char *path_to_executable,
                                char *const *executable_argv) {
  nlohmann::json config;
  {
    std::ifstream config_stream(path_to_config);
    if (config_stream.peek() != std::ifstream::traits_type::eof()) {
      // read only if the config is non-empty
      config_stream >> config;
      INFO("Successfully loaded configuration %s", config.dump(2, ' ').c_str())
    } else {
      WARN("Failed to load configuration from config file %s. It either does not exist or is empty", path_to_config)
    }
  }

  LaunchRunner(config, path_to_executable, executable_argv);
}

int RunnerMain(int argc, char *const *argv) {
  // argv[1] --- path to the config file
  // argv[2] --- executable to run
  // argv[3]..argv[argc-1] --- cmd arguments to the executable
  // argv[argc] --- NULL according to paragraph 5.1.2.2.1 of the C language Standard

  try {
    if (argc < 3) {
      throw std::invalid_argument("At least three arguments should be passed to runner.");
    }

    ParseConfigAndLaunchRunner(argv[1], argv[2], argv + 2);
    return 0;
  } catch (std::exception &e) {
    ERROR("Uncaught exception: %s", e.what());
  } catch (...) {
    ERROR("Something non-assignable to std::exception& was thrown during runner execution.");
  }
  return 1;
}
