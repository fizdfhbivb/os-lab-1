import argparse
import pathlib
import random
import shutil
import subprocess
import uuid
from contextlib import contextmanager


@contextmanager
def test_directory():
    base = pathlib.Path.cwd().resolve()
    path = base / ('os-lab1-' + uuid.uuid4().hex)
    path.mkdir()
    try:
        yield path
    finally:
        assert path.resolve().parent == base and path.name.startswith('os-lab1-')
        shutil.rmtree(path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bin-dir', type=pathlib.Path,
                        default=pathlib.Path(__file__).resolve().parents[1] / 'build')
    args = parser.parse_args()
    binary = (args.bin_dir / 'parent').resolve()
    count = 0
    with test_directory() as directory:
        root = pathlib.Path(directory)

        def case(name, commands, expected, code=0, filename='result.txt', initial=None):
            nonlocal count
            output = root / filename
            if output.exists():
                output.unlink()
            if initial is not None:
                output.write_bytes(initial)
            data = (str(output) + '\n' + commands).encode('utf-8')
            run = subprocess.run([str(binary)], input=data, capture_output=True,
                                 timeout=20, cwd=root)
            assert run.returncode == code, (name, run.returncode, run.stderr)
            actual = output.read_bytes() if output.exists() else None
            assert actual == expected, (name, actual, expected, run.stderr)
            assert run.stdout == b'', (name, 'unexpected console output')
            count += 1
            print(f'PASS {name}')

        case('composites followed by prime', '4\n6\n9\n7\n8\n', b'4\n6\n9\n')
        case('negative stops immediately', '-1\n4\n', b'')
        case('prime 2 stops immediately', '2\n4\n', b'')
        case('zero and one are skipped', '0\n1\n4\n', b'4\n')
        case('signed INT_MIN', '-2147483648\n4\n', b'')
        case('INT_MAX is prime', '2147483646\n2147483647\n4\n', b'2147483646\n')
        case('final line without newline', '25', b'25\n')
        case('CRLF spaces and plus sign', '  +12 \r\n\t49\t\r\n', b'12\n49\n')
        case('empty command stream', '', b'')
        case('manual exit', '4\nexit\n6\n', b'4\n')
        case('malformed input', '4\nhello\n6\n', b'4\n', 1)
        case('multiple integers rejected', '4 6\n', b'', 1)
        case('positive overflow', '2147483648\n', b'', 1)
        case('negative overflow', '-2147483649\n', b'', 1)
        case('embedded NUL rejected', '4\x00\n', b'', 1)
        case('empty line rejected', '\n', b'', 1)
        case('too long line rejected', ' ' * 100000 + '4\n', b'', 1)
        case('UTF-8 path with spaces', '81\n', b'81\n', filename='тест файл.txt')
        case('existing file is truncated', '4\n', b'4\n', initial=b'old contents\n')
        case('unavailable output directory', '4\n', None, 1, filename='missing/out.txt')
        case('quotation in Linux filename', '4\n', b'4\n', filename='bad"name.txt')
        random.seed(5)
        numbers = [random.randrange(2, 50000) * 2 for _ in range(1000)]
        case('1000 composite numbers', ''.join(f'{n}\n' for n in numbers),
             ''.join(f'{n}\n' for n in numbers).encode())
        alone = root / 'alone'
        alone.mkdir()
        shutil.copy2(binary, alone / 'parent')
        run = subprocess.run([str(alone / 'parent')], input=b'out.txt\n4\n',
                             capture_output=True, timeout=10, cwd=root)
        assert run.returncode == 1 and b'execv' in run.stderr
        count += 1
        print('PASS missing child executable')
    print(f'{count}/{count} integration tests passed')


if __name__ == '__main__':
    main()
