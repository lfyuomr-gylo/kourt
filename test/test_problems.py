import pytest
from pathlib import Path
import subprocess
from dataclasses import dataclass
from itertools import product
import json


@pytest.fixture
def project_root_dir():
    return Path(__file__).absolute().parent.parent


@pytest.fixture
def runner_executable(project_root_dir) -> Path:
    runner_dir = project_root_dir / 'runner'
    subprocess.run('mkdir -p build', shell=True, cwd=runner_dir, check=True)
    build_dir = runner_dir / 'build'
    subprocess.run("cmake .. && make", shell=True, cwd=build_dir, check=True)
    return build_dir / 'runner'


@dataclass
class CorrectSolution:
    solution_file: Path
    config_file: Path


@pytest.fixture
def correct_solutions(project_root_dir):
    problems_dir = project_root_dir / 'problems'
    solutions = []
    for problem_dir in filter(Path.is_dir, problems_dir.iterdir()):
        correct_solutions_dir = problem_dir / 'correct-solutions'
        test_suites_dir = problem_dir / 'test-suites'
        if correct_solutions_dir.exists() and test_suites_dir.exists():
            solutions.extend(product(correct_solutions_dir.iterdir(), test_suites_dir.iterdir()))
    return [CorrectSolution(*solution) for solution in solutions]


def test_correct_solutions_successfully_pass(project_root_dir, correct_solutions, runner_executable):
    for solution in correct_solutions:
        exec_status = subprocess.run([
            "python", "-m", "agent.agent",
            "--runner", str(runner_executable),
            "--config", str(solution.config_file),
            "--solution", str(solution.solution_file)
        ],
            cwd=project_root_dir,
        )
        assert exec_status.returncode == 0
