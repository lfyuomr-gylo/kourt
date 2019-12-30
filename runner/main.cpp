#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <nlohmann/json.hpp>

void pipeStdoutAndStderrToFiles(const nlohmann::json &config) {
    // TODO: handle syscall errors
    std::string stdoutFileName = config.contains("stdoutFile")
            ? config["stdoutFile"]
            : "stdout.txt";
    int stdoutFile = creat(stdoutFileName.c_str(), 0644);
    dup2(stdoutFile, 1);
    close(stdoutFile);

    std::string stderrFileName = config.contains("stderrFile")
            ? config["stderrFile"]
            : "stderr.txt";
    int stderrFile = creat(stderrFileName.c_str(), 0644);
    dup2(stderrFile, 2);
    close(stderrFile);
}

void printExitStatus(int exitStatus, const nlohmann::json &config) {
    std::string outFileName = config.contains("exitStatusFile")
            ? config["exitStatusFile"]
            : "exit-status.json";
    std::ofstream out(outFileName, std::ofstream::out | std::ofstream::trunc);

    nlohmann::json json;
    if (WIFEXITED(exitStatus)) {
        json["exitCode"] = WEXITSTATUS(exitStatus);
    } else if (WIFSIGNALED(exitStatus)) {
        json["signal"] = WTERMSIG(exitStatus);
    } else {
        std::cerr << "Unexpected child exit status: " << exitStatus << std::endl;
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
        std::ifstream configStream(argv[1]);
        if (configStream.peek() != std::ifstream::traits_type::eof()) {
            // read only if the config is non-empty
            configStream >> config;
        }
    }
    if (0 == pid) {
        // child
        pipeStdoutAndStderrToFiles(config);
        execv(argv[2], argv + 2);
        perror("execv");
        exit(1);
    } else if (pid > 0) {
        // parent
        int childStatus;
        waitpid(pid, &childStatus, WEXITED);
        printExitStatus(childStatus, config);
    } else {
        // TODO: handle fork failure correctly
        exit(1);
    }
}
