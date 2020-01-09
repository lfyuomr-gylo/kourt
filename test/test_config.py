import contextlib
import tempfile
from pathlib import Path

from agent.config import load_config


@contextlib.contextmanager
def _load_config(file_content: str) -> tempfile.NamedTemporaryFile:
    with tempfile.NamedTemporaryFile(mode='w') as file:
        file.write(file_content)
        file.flush()
        yield load_config(Path(file.name))


def test_default_exit_code_is_zero():
    # language=yaml
    with _load_config(r"""
            tests:
              - execution: 
                  executable: /bin/echo
            """) as tests:
        assert len(tests) == 1
        assert tests[0].validation.returnStatus.expectedValue == 0


def test_stdout_should_not_be_validated_by_default():
    # language=yaml
    with _load_config(r"""
            tests:
              - execution:
                  executable: /bin/echo
            """) as tests:
        assert len(tests) == 1
        assert 'stdout' not in tests[0].validation


def test_stdout_should_be_validated_ignoring_trailing_spaces_by_default():
    # language=yaml
    with _load_config(r"""
            tests:
              - execution:
                  executable: /bin/echo
                  cmdArgs: ['Hello, world!']
                validation:
                  stdout:
                    expectedContent:
                      text: 'Hello, world!'
            """) as tests:
        assert len(tests) == 1
        assert tests[0].validation.stdout.ignoreTrailingSpaces is True


def test_should_apply_template_to_tests():
    # language=yaml
    with _load_config(r"""
            testTemplate:
              preparation:
                command: 'cp /bin/echo my_echo'
              execution:
                executable: my_echo
              validation:
                stdout:
                  ignoreTrailingSpaces: false
            tests:
              - execution:
                  cmdArgs: ['biba']
                validation:
                  stdout: 
                    expectedContent:
                      text: "biba\n"
              - execution:
                  cmdArgs: ['kuka']
                validation:
                  stdout: 
                    expectedContent:
                      text: "kuka\n"
            """) as tests:
        assert len(tests) == 2
        assert tests[0].preparation == {"command": "cp /bin/echo my_echo"}
        assert tests[0].execution == {"executable": "my_echo", "cmdArgs": ["biba"]}
        assert tests[0].validation.stdout == {"expectedContent": {"text": "biba\n"}, "ignoreTrailingSpaces": False}

        assert tests[1].preparation == {"command": "cp /bin/echo my_echo"}
        assert tests[1].execution == {"executable": "my_echo", "cmdArgs": ["kuka"]}
        assert tests[1].validation.stdout == {"expectedContent": {"text": "kuka\n"}, "ignoreTrailingSpaces": False}
