#!/usr/bin/env python3
"""Historical PCH oracle: original execution is retired; see README.txt."""
import argparse


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args()
    parser.exit(2, 'Historical PCH capture is retired: its frozen PCH/IR embeds the former absolute cwd.\n'
        'Use decomp/vc1.0/c1/scripts/capture-frontier.py with a current C1-owned PCH manifest.\n'
        'See decomp/vc1.0/c1/cases/pch/ and the script --help.\n'
        'Frozen historical inputs/oracles remain unchanged; no Wine process was started.\n')


if __name__ == '__main__':
    main()
