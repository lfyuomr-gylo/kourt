import pathlib
import shutil
import subprocess
import tempfile

import munch


def prepare_for_execution(preparation_config: munch.Munch, solution_file: pathlib.Path) -> tempfile.TemporaryDirectory:
    execution_dir_cm = tempfile.TemporaryDirectory()
    execution_dir = execution_dir_cm.name

    dst_path = pathlib.Path(execution_dir) / preparation_config.solutionFileName
    shutil.copy2(solution_file, dst_path)

    subprocess.run(preparation_config.command, shell=True, cwd=execution_dir)
    return execution_dir_cm
