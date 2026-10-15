#!/usr/bin/env python3
"""Extract a host-compatible Qt SDK locally on Debian/Ubuntu; never install system packages."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys

PROJECT = Path(__file__).resolve().parents[1]
SEEDS = ['qt6-base-dev', 'qt6-base-dev-tools', 'qt6-svg-dev', 'qt6-multimedia-dev']

def command(*args):
    return subprocess.check_output(args, text=True).strip()

def installed(package):
    process = subprocess.run(['dpkg-query', '-W', '-f=${Status}', package], capture_output=True, text=True)
    return process.returncode == 0 and process.stdout == 'install ok installed'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=PROJECT / '.deps' / 'qt')
    args = parser.parse_args()
    root = args.directory.resolve()
    packages_dir = root / 'packages'
    packages_dir.mkdir(parents=True, exist_ok=True)
    queue, seen, packages = list(SEEDS), set(), []
    while queue:
        package = queue.pop(0)
        if package in seen:
            continue
        seen.add(package)
        if installed(package):
            continue
        packages.append(package)
        dependencies = command('apt-cache', 'depends', package)
        queue.extend(re.findall(r'^\s*(?:\|)?(?:Pre)?Depends:\s+([^\s<>]+)', dependencies, re.M))
    if packages:
        print(f'Downloading {len(packages)} packages into {packages_dir}', flush=True)
        subprocess.run(['apt-get', '-o', 'Acquire::Retries=1', 'download', *packages], cwd=packages_dir, check=True)
        for archive in packages_dir.glob('*.deb'):
            subprocess.run(['dpkg-deb', '-x', str(archive), str(root)], check=True)
    triplet = command('dpkg-architecture', '-qDEB_HOST_MULTIARCH')
    system_lib = Path('/usr/lib') / triplet
    local_lib = root / 'usr' / 'lib' / triplet
    local_lib.mkdir(parents=True, exist_ok=True)
    # Relocated CMake targets expect runtime files beside their development symlinks.
    # Reuse compatible host libraries rather than modifying /usr or duplicating them.
    for pattern in ('libQt6*', 'libGL*', 'libOpenGL*'):
        for source in system_lib.glob(pattern):
            destination = local_lib / source.name
            if not destination.exists() and not destination.is_symlink():
                destination.symlink_to(source)
    for relative in ('qt6/plugins', 'qt6/libexec', 'qt6/bin'):
        source = system_lib / relative
        destination = local_lib / relative
        if source.is_dir():
            destination.mkdir(parents=True, exist_ok=True)
            for item in source.iterdir():
                target = destination / item.name
                if not target.exists() and not target.is_symlink():
                    target.symlink_to(item)
    config = local_lib / 'cmake' / 'Qt6' / 'Qt6Config.cmake'
    system_config = system_lib / 'cmake' / 'Qt6' / 'Qt6Config.cmake'
    if not config.exists() and not system_config.exists():
        raise RuntimeError('Qt configuration was not found after extraction.')
    prefix = root / 'usr' if config.exists() else Path('/usr')
    manifest = {'prefix': str(prefix), 'multiarch': triplet, 'packages': packages,
                'systemRuntimeReused': True}
    (root / 'sdk.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Qt SDK ready: {prefix}\nBuild with ./scripts/build.sh', flush=True)

if __name__ == '__main__':
    try:
        main()
    except (subprocess.CalledProcessError, OSError, RuntimeError) as error:
        print(f'Qt setup failed: {error}', file=sys.stderr)
        sys.exit(1)
