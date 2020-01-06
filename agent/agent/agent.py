import argparse
from argparse import ArgumentParser, Namespace
from functools import partial
from pathlib import Path
from typing import Union, Text, Sequence, Any, Optional

from agent.config import load_config
from agent.execution import run_execution
from agent.preparation import prepare_for_execution
from agent.validation import validate_execution_results


class _StorePathAction(argparse.Action):

    def __call__(
            self,
            parser: ArgumentParser,
            namespace: Namespace,
            values: Union[Text, Sequence[Any], None],
            option_string: Optional[Text] = ...) -> None:
        setattr(namespace, self.dest, Path(values).absolute())


def _parse_args():
    parser = argparse.ArgumentParser()
    add_path_argument = partial(parser.add_argument, required=True, action=_StorePathAction)
    add_path_argument('-r', '--runner', help='Path to runner executable')
    add_path_argument('-c', '--config', help='Path to test suite configuration file')
    add_path_argument('-s', '--solution', help='Path to solution file')
    return parser.parse_args()


if __name__ == '__main__':
    cli_args = _parse_args()
    test_suites = load_config(Path(cli_args.config))
    for test_suite in test_suites:
        with prepare_for_execution(test_suite.preparation, cli_args.solution) as execution_dir_name:
            execution_dir = Path(execution_dir_name)
            with run_execution(execution_dir, test_suite.execution, Path(cli_args.runner)) as execution_status_dir_name:
                validate_execution_results(Path(execution_status_dir_name), test_suite.validation)
