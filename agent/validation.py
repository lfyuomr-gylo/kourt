import json
from pathlib import Path

from munch import Munch

from agent.execution import STDOUT_FILE, EXIT_STATUS_FILE


def validate_execution_results(execution_status_dir: Path, validation_config: Munch):
    if 'stdout' in validation_config:
        _validate_stdout(execution_status_dir / STDOUT_FILE, validation_config.stdout)
    if 'returnStatus' in validation_config:
        _validate_return_status(execution_status_dir / EXIT_STATUS_FILE, validation_config.returnStatus)


def _validate_stdout(stdout_file: Path, requirements: Munch):
    with stdout_file.open('r') as f:
        actual_stdout = f.read()
        # TODO: после того, как будет реализовано проставление дефолтов из схемы, дефолт отсюда нужно убрать
        if requirements.get('ignoreTrailingSpaces', True):
            actual_stdout = actual_stdout.rstrip()
    # TODO: поддерживать также другие форматы ожидаемого контента
    expected_stdout = requirements.expectedContent.text.rstrip()
    if actual_stdout != expected_stdout:
        raise AssertionError(f"Expected '{expected_stdout}' but got '{actual_stdout}'")


def _validate_return_status(exit_status_file: Path, requirements: Munch):
    with exit_status_file.open('r') as f:
        exit_status = json.load(f)
        actual_exit_code = exit_status.get('exitCode', None)
        expected_exit_code = requirements.expectedValue
        if actual_exit_code != expected_exit_code:
            raise AssertionError(f"Expected exit code {expected_exit_code} but got {actual_exit_code}")
