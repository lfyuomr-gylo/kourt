import json
import subprocess
from pathlib import Path
from tempfile import TemporaryDirectory

from munch import Munch

# === Names of files in execution status directory
STDOUT_FILE = Path('stdout.txt')
STDERR_FILE = Path('stderr.txt')
EXIT_STATUS_FILE = Path('exit-status.json')
RUNNER_CONFIG_FILE = Path('runnner-config.json')


def run_execution(execution_dir: Path, execution_config: Munch, path_to_runner: Path) -> TemporaryDirectory:
    status_dir_cm = TemporaryDirectory()
    status_dir = Path(status_dir_cm.name)

    _prepare_runner_configuration(execution_dir, status_dir)
    _execute_runner(execution_dir, execution_config, path_to_runner, status_dir)
    return status_dir_cm


def _prepare_runner_configuration(execution_dir: Path, status_dir: Path):
    config = {
        "stdoutFile": str(status_dir / STDOUT_FILE),
        "stderrFile": str(status_dir / STDERR_FILE),
        "exitStatusFile": str(status_dir / EXIT_STATUS_FILE)
    }
    with (status_dir / RUNNER_CONFIG_FILE).open('w') as f:
        json.dump(config, f)


def _execute_runner(execution_dir: Path, execution_config: Munch, path_to_runner: Path, status_dir: Path):
    runner_config_file = str(status_dir / RUNNER_CONFIG_FILE)
    solution_executable_path = str(execution_dir / execution_config.executable)
    solution_cmd_args = execution_config.get('cmdArgs', [])
    subprocess.run(
        [str(path_to_runner), runner_config_file, solution_executable_path, *solution_cmd_args],
        cwd=str(execution_dir),
        # TODO: support other stdin options
        text=True,
        input=execution_config.stdin.text
        # TODO: capture runner's stdout and stderr
    )
