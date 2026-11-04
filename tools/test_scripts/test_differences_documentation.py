"""Check documented PicoNIX capacities against the source constants."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]


def value(pattern, path):
    match = re.search(pattern, (ROOT / path).read_text())
    assert match, f"missing constant in {path}: {pattern}"
    return int(match.group(1))


class DifferencesDocumentationTest(unittest.TestCase):
    def test_capacities(self):
        kernel = "kernel/kernel.c"
        self.assertEqual(value(r"#define OS_MAX_TASKS\s+(\d+)", "include/piconix.h"), 6)
        self.assertEqual(value(r"#define OS_FD_MAX\s+(\d+)", kernel), 16)
        self.assertEqual(value(r"#define PIPE_COUNT\s+(\d+)", kernel), 2)
        self.assertEqual(value(r"#define PIPE_BYTES\s+(\d+)", kernel), 256)
        self.assertEqual(value(r"#define TMP_FILES\s+(\d+)", kernel), 11)
        self.assertEqual(value(r"#define TMP_BYTES\s+(\d+)", kernel), 2048)
        self.assertEqual(value(r"#define NET_SOCKET_TCP_SLOTS\s+(\d+)", "kernel/net_socket.h"), 4)
        for lang in ("en", "ja"):
            doc = (ROOT / f"docs/{lang}/reference/DIFFERENCES.md").read_text()
            for number in ("6", "24 KiB", "4 KiB", "16", "256", "11", "2,048", "64"):
                self.assertIn(number, doc)
            self.assertIn("/dev/ttyN0", doc)
            self.assertIn("/proc", doc)


if __name__ == "__main__":
    unittest.main()
