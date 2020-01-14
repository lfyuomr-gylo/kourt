#!/usr/bin/env python3

import unittest
import sys
import argparse
import tempfile
import subprocess
from collections import namedtuple
import json

RUNNER_EXECUTABLE: str = 'to be constituted in main'


class CProgramRunningTests(unittest.TestCase):
    ExecutionResults = namedtuple('ExecutionResults', ['stdout_file', 'stderr_file', 'exit_status_file'])

    def test_should_successfully_detect_exit_status_code(self):
        with tempfile.TemporaryDirectory() as working_dir:
            # language=C
            results = self._execute_program_with_runner(working_dir, r"""
                int main() {
                    return 25;
                }
            """)

            with open(results.exit_status_file, 'r') as f:
                exit_status = json.load(f)
                self.assertEqual(exit_status.get('exitCode'), 25)

    def test_should_successfully_determine_termination_signal(self):
        with tempfile.TemporaryDirectory() as working_dir:
            # language=C
            results = self._execute_program_with_runner(working_dir, r"""
                #include <signal.h>
                #include <unistd.h>

                int main() {
                    raise(10);
                    pause();
                }
            """)

            with open(results.exit_status_file, 'r') as f:
                exit_status = json.load(f)
                self.assertEqual(exit_status.get('signal'), 10)

    def test_should_successfully_pipe_stdout_to_file(self):
        with tempfile.TemporaryDirectory() as working_dir:
            # language=C
            results = self._execute_program_with_runner(working_dir, r"""
                #include <stdio.h>
                int main() {
                    printf("%s\n", "Hello, world!");
                }
            """)

            with open(results.stdout_file, 'r') as f:
                actual_text = f.read()
                expected_text = "Hello, world!\n"
                self.assertEqual(actual_text, expected_text)

    def test_should_successfully_pipe_stderr_to_file(self):
        with tempfile.TemporaryDirectory() as working_dir:
            # language=C
            results = self._execute_program_with_runner(working_dir, r"""
                #include <stdio.h>
                int main() {
                    fprintf(stderr, "%s\n", "Hello, world!");
                }
            """)

            with open(results.stderr_file, 'r') as f:
                actual_text = f.read()
                expected_text = "Hello, world!\n"
                self.assertEqual(actual_text, expected_text)

    def test_there_should_be_no_surplus_open_file_descriptors_in_executed_program(self):
        with tempfile.TemporaryDirectory() as working_dir:
            # language=C
            results = self._execute_program_with_runner(working_dir, r"""
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
            """)

            with open(results.stdout_file, 'r') as f:
                actual_descriptors = set(int(line.strip()) for line in f.readlines())
                expected_descriptors = {0, 1, 2}
                self.assertEqual(actual_descriptors, expected_descriptors)

    def test_read_size_shrink_should_make_read_syscall_to_read_less_bytes_than_requested(self):
        with tempfile.TemporaryDirectory() as working_dir:
            stdin = "123"
            # language=C
            results = self._execute_program_with_runner(
                working_dir,
                r"""
                    #include <unistd.h>
                    int main() {
                        char buf[10];
                        ssize_t bytes_read = read(0, buf, sizeof(buf));
                        write(1, buf, butes_read);
                        for (ssize_t bytes_written = 0; bytes_written < bytes_read; ) {
                            bytes_written += write(1, buf + bytes_written, bytes_read - bytes_written);
                        }
                    }
                """,
                config_file='{"interceptors": [{"name": "ReadSizeShrinkInterceptor"}]}',
                stdin=stdin
            )

            with open(results.stdout_file, 'r') as f:
                stdout = f.read()
                self.assertTrue(stdout < stdin,
                                f"Expected stdout '{stdout}' to be substring of initial stdin '{stdin}'")
                self.assertTrue(stdout == stdin[:len(stdout)],
                                f"Expected stdout '{stdout}' to be substring of initial stdin '{stdin}'")

    def _execute_program_with_runner(self, working_dir: str, program_text, config_file: str = '/dev/null', stdin=None):
        program_filename = 'program.c'
        executable_filename = 'program'

        # Compile program
        with open(f'{working_dir}/{program_filename}', 'w') as f:
            print(program_text, file=f)
        subprocess.run(
            f"g++ '{program_filename}' -o '{executable_filename}'",
            shell=True,
            check=True,
            cwd=working_dir,
            input=stdin
        )

        # Execute with runner
        subprocess.run([RUNNER_EXECUTABLE, config_file, executable_filename], cwd=working_dir, check=True)
        return self.ExecutionResults(
            stdout_file=f'{working_dir}/stdout.txt',
            stderr_file=f'{working_dir}/stderr.txt',
            exit_status_file=f'{working_dir}/exit-status.json'
        )


if __name__ == '__main__':
    print("======================== HELLO =======================")
    parser = argparse.ArgumentParser()
    parser.add_argument('runner_executable')
    parser.add_argument('unittest_args', nargs=argparse.REMAINDER)
    args = parser.parse_args()

    RUNNER_EXECUTABLE = args.runner_executable
    unittest_args = [sys.argv[0]] + args.unittest_args
    unittest.main(argv=unittest_args)
