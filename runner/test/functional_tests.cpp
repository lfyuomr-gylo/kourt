#include <algorithm>
#include <random>
#include <functional>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
#include <unordered_set>
#include <string>

#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include <nlohmann/json.hpp>
#include <gtest/gtest.h>

#include <kourt/runner/config.h>
#include <kourt/runner/runner_main.h>

namespace fs = std::filesystem;

//region random_int

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

static std::random_device random_device;
static std::default_random_engine random_engine(random_device()); // NOLINT(cert-err58-cpp)
static std::uniform_int_distribution<int> random_distribution(0, 2'000'000'000); // NOLINT(cert-err58-cpp)
static auto random_int = std::bind(random_distribution, random_engine); // NOLINT(cert-err58-cpp)

#pragma clang diagnostic pop
//endregion

std::string ReadTextFile(const fs::path &file_path) {
  std::ifstream input(file_path);
  auto size = fs::file_size(file_path);
  auto data = new char[size + 1];
  input.read(data, size);
  data[input.gcount()] = '\0';
  std::string result(data);
  delete[] data;
  return result;
}

std::vector<std::string> ReadLines(const fs::path &file_path) {
  std::ifstream input(file_path);
  std::vector<std::string> lines;
  for (std::string line; std::getline(input, line);) {
    lines.push_back(line);
  }
  return lines;
}

nlohmann::json ReadJsonFile(const fs::path &file_path) {
  std::ifstream input(file_path);
  nlohmann::json result;
  input >> result;
  return result;
}

void DiscoverOpenFileDescriptors(std::unordered_set<int> *out_file_descriptors) {
  int dirfd = open("/dev/fd", O_RDONLY);
  DIR *dir = fdopendir(dirfd);

  for (dirent *entry = readdir(dir); entry; entry = readdir(dir)) {
    const std::string entry_name = entry->d_name;
    int fd;
    if (entry_name != "." && entry_name != ".." && (fd = (int) std::stol(entry_name)) != dirfd) {
      out_file_descriptors->insert(fd);
    }
  }

  closedir(dir);
  close(dirfd);
}

class FunctionalTest : public ::testing::Test {
 protected:

  void SetUp() override {
    original_working_directory_ = get_current_dir_name();

    std::string directory_name("runner_functional_test_" + std::to_string(random_int()));
    working_directory_ = fs::temp_directory_path() / directory_name;
    if (!fs::create_directory(working_directory_)) {
      throw std::runtime_error("Failed to create test working directory");
    }
    chdir(working_directory_.c_str());

    WithConfig(nlohmann::json::object());
    program_source_file_ = working_directory_ / "program.c";
    program_binary_file_ = working_directory_ / "program";
  }

  void TearDown() override {
    fs::remove_all(working_directory_);

    chdir(original_working_directory_);
    free(original_working_directory_);
  }

  void WithProgram(const std::string &program_text) {
    char *compiler_path = std::getenv("CC");
    if (!compiler_path) {
      throw std::runtime_error("CC environment variable is not set");
    }

    WithFile(program_source_file_, program_text);
    auto compile_command =
        compiler_path + std::string(" ") + program_source_file_.string() + " -o " + program_binary_file_.string();
    if (int status = system(compile_command.c_str()); status == -1 || !WIFEXITED(status) || 0 != WEXITSTATUS(status)) {
      throw std::runtime_error("Compilation failed");
    }
  }
  void WithConfig(const nlohmann::json &config) {
    config_file_ = working_directory_ / "config.json";
    WithFile(config_file_, config);

    program_stdout_file_ = config.value(kStdoutFileKey, working_directory_ / kDefaultStdoutFile);
    program_stderr_file_ = config.value(kStderrFileKey, working_directory_ / kDefaultStderrFile);
    program_exit_status_file_ = config.value(kExitStatusFileKey, working_directory_ / kDefaultExitStatusFile);
  }

  /// \return exit status returned by main function of the runner
  int ExecuteRunner() {
    std::string argv0 = "runner";
    const char *argv[] = {argv0.c_str(), config_file_.c_str(), program_binary_file_.c_str(), nullptr};
    return RunnerMain(3, const_cast<char *const *>(argv));
  }

  template<typename Data>
  void WithFile(const fs::path &a_path, const Data &data) {
    fs::path path = (a_path.is_absolute())
        ? a_path
        : (working_directory_ / a_path);
    std::ofstream out(path, std::fstream::out | std::fstream::trunc);
    out << data;
    if (out.bad()) {
      throw std::runtime_error("Failed to write data to file " + path.string());
    }
  }

  [[nodiscard]] const fs::path &program_stdout_file() const {
    return program_stdout_file_;
  }
  [[nodiscard]] const fs::path &program_stderr_file() const {
    return program_stderr_file_;
  }
  [[nodiscard]] const fs::path &program_exit_status_file() const {
    return program_exit_status_file_;
  }

 private:

  char *original_working_directory_;

  fs::path working_directory_;
  fs::path program_source_file_;
  fs::path program_binary_file_;
  fs::path config_file_;

  fs::path program_stdout_file_;
  fs::path program_stderr_file_;
  fs::path program_exit_status_file_;
};

TEST_F(FunctionalTest, ShouldSuccessfullyDetectExitStatusCode) {
  // given:
  WithProgram(/* language=C */ R"c05ae13d(
    int main() {
      return 25;
    }
  )c05ae13d");

  // when:
  int runner_exit_status = ExecuteRunner();

  // then: runner finished successfully
  ASSERT_EQ(runner_exit_status, 0);

  // and: runner detected program exit code correctly
  nlohmann::json exit_status_content = ReadJsonFile(program_exit_status_file());
  ASSERT_TRUE(exit_status_content.contains("exitCode"));
  ASSERT_EQ(exit_status_content["exitCode"], 25);
}

TEST_F(FunctionalTest, ShouldSuccessfullyDetermineTerminationSignal) {
  // given:
  WithProgram(/* language=C */ R"bibakuka(
    #include <signal.h>
    #include <unistd.h>

    int main() {
      raise(10);
      pause();
    }
  )bibakuka");

  // when:
  int runner_exit_status = ExecuteRunner();

  // then: exits successfully
  ASSERT_EQ(runner_exit_status, 0);

  // and: terminating signal is detected by runner successfully
  nlohmann::json exit_status_content = ReadJsonFile(program_exit_status_file());
  ASSERT_TRUE(exit_status_content.contains("signal"));
  ASSERT_EQ(exit_status_content["signal"], 10);
}

TEST_F(FunctionalTest, ShouldSuccessfullyPipeStdoutToFile) {
  // given:
  WithProgram(/* language=C */ R"bibakuka(
    #include <stdio.h>
    int main() {
      printf("%s\n", "Hello, world!");
      fflush(stdout);
    }
  )bibakuka");

  // when:
  int runner_exit_status = ExecuteRunner();

  // then: runner successfully finished
  ASSERT_EQ(runner_exit_status, 0);

  // and:
  std::string actual_stdout = ReadTextFile(program_stdout_file());
  ASSERT_EQ(actual_stdout, "Hello, world!\n");
}

TEST_F(FunctionalTest, ShouldSuccessfullyPipeStderrToFile) {
  // given:
  WithProgram(/* language=C */ R"bibakuka(
    #include <stdio.h>
    int main() {
      fprintf(stderr, "%s\n", "Hello, world!");
      fflush(stderr);
    }
  )bibakuka");

  // when:
  int runner_exit_status = ExecuteRunner();

  // then: runner successfully finished
  ASSERT_EQ(runner_exit_status, 0);

  // and:
  std::string actual_stdout = ReadTextFile(program_stderr_file());
  ASSERT_EQ(actual_stdout, "Hello, world!\n");
}

TEST_F(FunctionalTest, ThereShouldBeNoSurplusOpenFileDescriptorsInExecutedProgram) {
  // given:
  WithProgram(/* language=C */ R"bibakuka(
        #include <sys/types.h>
        #include <sys/stat.h>
        #include <fcntl.h>
        #include <dirent.h>
        #include <unistd.h>
        
        #include <stdio.h>
        #include <string.h>
        
        enum {
          INT_MAX_DECIMAL_DIGITS = 11
        };
        
        int main() {
          // this program prints all file descriptors open before the 'main' function is entered
        
          int dirfd = open("/dev/fd", O_RDONLY);
          DIR *dir = fdopendir(dirfd);
          
          char dirfd_text[INT_MAX_DECIMAL_DIGITS + 1];
          memset(dirfd_text, 0, sizeof(dirfd_text));
          snprintf(dirfd_text, INT_MAX_DECIMAL_DIGITS, "%d", dirfd);
        
          for (struct dirent *entry = readdir(dir); entry; entry = readdir(dir)) {
            char *entry_name = entry->d_name;
            if (0 != strcmp(".", entry_name) && 0 != strcmp("..", entry_name) && 0 != strcmp(dirfd_text, entry_name)) {
              printf("%s\n", entry_name);
            }
          }
        
          closedir(dir);
          close(dirfd);
        }
  )bibakuka");

  // when:
  int runner_exit_code = ExecuteRunner();

  // then:
  ASSERT_EQ(runner_exit_code, 0);

  // and: only stdin, stdout and stderr file descriptors are open in the executed program.
  std::vector<std::string> lines = ReadLines(program_stdout_file());
  std::unordered_set<int> actual_file_descriptors;
  for (auto &&line : lines) {
    actual_file_descriptors.insert(std::stol(line));
  }

  // When the test is executed via `ctest`, there may be other open file descriptors than 0, 1 and 2,
  // so we have to detect them and require the runner to not introduce any more file descriptors to the executed program.
  // For more information, see https://github.com/lfyuomr-gylo/kourt/issues/7
  std::unordered_set<int> expected_file_descriptors;
  DiscoverOpenFileDescriptors(&expected_file_descriptors);

  ASSERT_EQ(actual_file_descriptors, expected_file_descriptors);
}

TEST_F(FunctionalTest, ReadSizeShrinkInterceptorShouldMakeReadSyscallToReadLessBytesThanRequested) {
  // given:
  WithProgram(/* language=C */ R"bibakuka(
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>

    int main() {
      int fd = open("test_input_file.txt", O_RDONLY);
      char data[10];
      size_t bytes_read = read(fd, data, 10);
      for (ssize_t bytes_written = 0; bytes_written < bytes_read;) {
        bytes_written += write(1, data + bytes_written, bytes_read - bytes_written);
      }
      close(fd);
    }
  )bibakuka");
  WithConfig(nlohmann::json::parse(R"biba(
    {"interceptors": [{"name": "ReadSizeShrinkInterceptor"}]}
  )biba"));

  std::string input = "123";
  WithFile("test_input_file.txt", input);

  // when:
  int runner_exit_status = ExecuteRunner();

  // then:
  ASSERT_EQ(runner_exit_status, 0);

  // and: output is prefix of input
  auto output = ReadTextFile(program_stdout_file());
  EXPECT_LT(output.length(), input.length());
  EXPECT_EQ(output, input.substr(0, output.length()));
}