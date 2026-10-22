"""Compare an independent fresh-history receipt with a continuous C2 receipt."""
import argparse
import json
from pathlib import Path


def check(fresh_path, continuous_path):
    fresh = json.loads(Path(fresh_path).read_text())
    continuous = json.loads(Path(continuous_path).read_text())
    assert fresh['method'] == 'fresh' and continuous['method'] == 'continuous'
    for key in ('profile', 'first', 'last', 'frontiers', 'terminal'):
        assert fresh[key] == continuous[key], 'receipt differs: ' + key
    for path, digest in fresh['frozen'].items():
        assert continuous['frozen'][str(Path(path).resolve())] == digest, 'different frozen build: ' + path
    expected = [(row['count'], row['logical']) for row in fresh['results']]
    assert len(expected) == fresh['last'] - fresh['first']
    assert len(continuous['sides']) == 2
    for side in continuous['sides']:
        actual = [(event['index'], event['name'])
                  for event in side['events'] if event['event'] == 'frontier']
        assert actual == expected, 'fresh and continuous entry sequences differ'
        terminal = [event['name'] for event in side['events'] if event['event'] == 'terminal']
        assert terminal == [fresh['terminal']], 'terminal differs'
        assert side['events'][-1]['event'] == 'pass'
    print('%s: %d fresh/continuous entries and terminal %s match on the same frozen build' %
          (fresh['profile'], len(expected), fresh['terminal']))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('fresh')
    parser.add_argument('continuous')
    args = parser.parse_args()
    check(args.fresh, args.continuous)
