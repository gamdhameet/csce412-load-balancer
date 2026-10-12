"""Check the accounting and scaling of the repeatable assignment run."""

import re
import subprocess


result = subprocess.run(
    ["./load-balancer", "--servers", "20", "--cycles", "100000", "--seed", "412", "--mode", "quiet"],
    check=True,
    capture_output=True,
    text=True,
)
output = result.stdout


def value(pattern):
    match = re.search(pattern, output)
    assert match, f"missing summary field: {pattern}"
    return int(match.group(1))


assert value(r"Starting queue size: (\d+)") == 2000
generated = value(r"Generated requests: (\d+)")
blocked = value(r"IP-filter rejected: (\d+)")
discarded = value(r"Queue-full discarded: (\d+)")
completed = value(r"Completed requests: (\d+)")
busy = value(r"Active servers at end: (\d+)")
waiting = value(r"Ending queue size: (\d+)")

assert blocked > 0
assert "scale=add" not in output  # quiet mode reports a history, not event lines
assert ": add ->" in output and ": remove ->" in output
assert generated == blocked + discarded + completed + busy + waiting
print("summary accounting and scaling: OK")
