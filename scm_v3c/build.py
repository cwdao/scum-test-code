"""Builds the SCuM firmware for the given project.

For macOS users, Wine is required, which can be installed from
https://wiki.winehq.org/MacOS.

The ARMCC toolchain can be downloaded from
https://drive.google.com/file/d/13K1JNKu-wprHqGxFRZL4OgcY5WSSTBiG/view?usp=sharing.

Usage:
    python3 build.py \
        --toolchain_dir ~/ARM \
        --project hello_world

Example commands:
wine64 /ARM/ARMCC/bin/armcc.exe --c99 -c --cpu Cortex-M0 -D__EVAL -g -O0 --apcs=interwork -D__UVISION_VERSION="525" -DARMCM0 -I ../../ -o hello_world.o --omf_browse hello_world.crf --depend hello_world.d hello_world.c
wine64 /ARM/ARMCC/bin/armasm.exe --cpu Cortex-M0 --pd "__EVAL SETA 1" -g --16 --apcs=interwork --pd "__UVISION_VERSION SETA 525" --pd "ARMCM0 SETA 1" --xref --list cm0dsasm.lst -o cm0dsasm.o --depend cm0dsasm.d ../../common/cm0dsasm.s
wine64 /ARM/ARMCC/bin/armlink.exe --cpu Cortex-M0 --strict --scatter ../../build/linker.sct --summary_stderr --info summarysizes --map --xref --callgraph --symbols --info sizes --info totals --info unused --info veneers --list hello_world.map -o hello_world.axf *.o
"""

import os
import subprocess
from pathlib import Path

from absl import app, flags, logging

FLAGS = flags.FLAGS


def _is_windows():
    """Returns true if running on Windows."""
    return os.name == "nt"


# Current path.
PATH = Path(__file__).parent

# ARM license file environment variable.
ARM_LICENSE_ENV_VAR = "ARMLMD_LICENSE_FILE"
LICENSE_FILE = "TOOLS.INI"

# Directory names.
APPLICATIONS_DIR = "applications"
COMMON_DIR = "."
BUILD_DIR = "build"
OUTPUT_DIR = "bin"

# Scatter file for the linker.
SCATTER_FILE = "scum_linker.sct"

# Toolchain configuration.
WINE = "" if _is_windows() else "wine64"
CC = "ARM/ARMCC/bin/armcc.exe"
CC_FLAGS = (
    "--c99",
    "-c",
    "--cpu Cortex-M0",
    "-D__EVAL",
    "-g",
    "-O0",
    "--apcs=interwork",
    "-D__UVISION_VERSION=\"525\"",
    "-DARMCMO",
)
ASM = "ARM/ARMCC/bin/armasm.exe"
ASM_FLAGS = (
    "--cpu Cortex-M0",
    "--pd \"__EVAL SETA 1\"",
    "-g",
    "--16",
    "--apcs=interwork",
    "--pd \"__UVISION_VERSION SETA 525\"",
    "--pd \"ARMCM0 SETA 1\"",
    "--xref",
)
LD = "ARM/ARMCC/bin/armlink.exe"
LD_FLAGS = (
    "--cpu Cortex-M0",
    "--strict",
    f"--scatter {PATH / BUILD_DIR / SCATTER_FILE}",
    "--summary_stderr",
    "--info summarysizes",
    "--map",
    "--xref",
    "--callgraph",
    "--symbols",
    "--info sizes",
    "--info totals",
    "--info unused",
    "--info veneers",
)
OBJCOPY = "ARM/ARMCC/bin/fromelf.exe"
DISASM = "ARM/ARMCC/bin/fromelf.exe"

# All SCuM applications.
APPLICATIONS = tuple(
    dir.stem for dir in (PATH / APPLICATIONS_DIR).iterdir() if dir.is_dir())


def _run_cmd(cmd: str) -> None:
    """Runs the given command.

    Args:
        cmd: Command to run.
    """
    logging.info("Running: %s", cmd)
    subprocess.run(
        cmd,
        check=True,
        shell=True,
    )


def _compile(toolchain_dir: Path, project_dir: Path, common_dir: Path,
             dir: Path) -> list[Path]:
    """Compiles all .c files in the given directory.

    Args:
        toolchain_dir: Toolchain directory.
        project_dir: Project directory.
        common_dir: Common directory.
        dir: Directory to compile.

    Returns:
        List of paths to object files.
    """
    output_dir = project_dir / OUTPUT_DIR
    object_files = []
    for file in dir.glob("*.c"):
        logging.info("Compiling %s.", file.name)
        stem = file.stem
        args = (
            f"-I {project_dir}",
            f"-I {common_dir}",
            f"-o {output_dir}/{stem}.o",
            f"--omf_browse {output_dir}/{stem}.crf",
            f"--depend {output_dir}/{stem}.d",
        )
        cmd = f"{WINE} {toolchain_dir / CC} {' '.join(CC_FLAGS)} {' '.join(args)} {file}"
        _run_cmd(cmd)
        object_files.append(output_dir / f"{stem}.o")
    return object_files


def _assemble(toolchain_dir: Path, project_dir: Path, dir: Path) -> list[Path]:
    """Assembles all .s files in the given directory.

    Args:
        toolchain_dir: Toolchain directory.
        project_dir: Project directory.
        dir: Directory to assemble.

    Returns:
        List of paths to object files.
    """
    output_dir = project_dir / OUTPUT_DIR
    object_files = []
    for file in dir.glob("*.s"):
        logging.info("Assembling %s.", file.name)
        stem = file.stem
        args = (
            f"--list {output_dir}/{stem}.lst",
            f"-o {output_dir}/{stem}.o",
            f"--depend {output_dir}/{stem}.d",
        )
        cmd = f"{WINE} {toolchain_dir / ASM} {' '.join(ASM_FLAGS)} {' '.join(args)} {file}"
        _run_cmd(cmd)
        object_files.append(output_dir / f"{stem}.o")
    return object_files


def _link(toolchain_dir: Path, project_dir: Path,
          object_files: list[Path]) -> None:
    """Links the given object files in given project directory.

    Args:
        toolchain_dir: Toolchain directory.
        project_dir: Project directory.
        object_files: List of paths to object files to link.
    """
    project = project_dir.stem
    logging.info("Linking %s project.", project)
    output_dir = project_dir / OUTPUT_DIR
    args = (
        f"--list {output_dir}/{project}.map",
        f"-o {output_dir}/{project}.axf",
        *(f"{file}" for file in object_files),
    )
    cmd = f"{WINE} {toolchain_dir / LD} {' '.join(LD_FLAGS)} {' '.join(args)}"
    _run_cmd(cmd)


def _gen_binary(toolchain_dir: Path, project_dir: Path) -> Path:
    """Generates a binary file for the project.

    Args:
        toolchain_dir: Toolchain directory.
        project_dir: Project directory.

    Returns:
        Path to the binary file.
    """
    project = project_dir.stem
    logging.info("Generating %s.bin.", project_dir)
    output_dir = project_dir / OUTPUT_DIR
    args = (
        f"--bin {output_dir}/{project}.axf",
        f"-o {output_dir}/{project}.bin",
    )
    cmd = f"{WINE} {toolchain_dir / OBJCOPY} {' '.join(args)}"
    _run_cmd(cmd)
    return output_dir / f"{project}.bin"


def _disassemble(toolchain_dir: Path, project_dir: Path) -> None:
    """Generates a disassembly file for the project.

    Args:
        toolchain_dir: Toolchain directory.
        project_dir: Project directory.
    """
    project = project_dir.stem
    logging.info("Disassembling %s.", project_dir)
    output_dir = project_dir / OUTPUT_DIR
    args = (
        f"-cvf {output_dir}/{project}.axf",
        f"-o {output_dir}/{project}_disasm.txt",
    )
    cmd = f"{WINE} {toolchain_dir / DISASM} {' '.join(args)}"
    _run_cmd(cmd)


def main(argv):
    assert len(argv) == 1

    toolchain_dir = Path(FLAGS.toolchain_dir)

    # Set the license file.
    os.environ[ARM_LICENSE_ENV_VAR] = str(toolchain_dir / LICENSE_FILE)

    project_dir = PATH / APPLICATIONS_DIR / FLAGS.project
    common_dir = PATH / COMMON_DIR
    output_dir = project_dir / OUTPUT_DIR
    output_dir.mkdir(exist_ok=True)

    # Compile and assemble all files in the common and the project directories.
    object_files = []
    for dir in (project_dir, common_dir):
        object_files.extend(
            _compile(toolchain_dir, project_dir, common_dir, dir))
        object_files.extend(_assemble(toolchain_dir, project_dir, dir))

    # Link all object files in the project directory.
    _link(toolchain_dir, project_dir, object_files)

    # Generate a binary and the dissembly.
    bin_path = _gen_binary(toolchain_dir, project_dir)
    _disassemble(toolchain_dir, project_dir)

    logging.info("Successfully generated %s.", bin_path)


if __name__ == "__main__":
    flags.DEFINE_string("toolchain_dir", "~/Documents/ARM",
                        "Path to the ARMCC toolchain directory.")
    flags.DEFINE_enum("project", None, APPLICATIONS, "Project to build.")
    flags.mark_flag_as_required("project")

    app.run(main)
