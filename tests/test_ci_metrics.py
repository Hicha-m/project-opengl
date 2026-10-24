"""Unit tests for CI metrics; no GitHub API or runner is required."""
import sys
from pathlib import Path
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from ci_metrics import analyze_jobs, category, make_markdown, read_junit, seconds


class CIMetricsTests(unittest.TestCase):
    def test_parallel_time_and_failures(self):
        jobs = [
            {"name": "Linux", "conclusion": "success",
             "started_at": "2026-10-22T11:00:00Z", "completed_at": "2026-10-22T11:02:00Z",
             "steps": [{"name": "Install SDL3", "conclusion": "success",
                        "started_at": "2026-10-22T11:00:00Z", "completed_at": "2026-10-22T11:00:50Z"},
                       {"name": "Build", "conclusion": "success",
                        "started_at": "2026-10-22T11:00:50Z", "completed_at": "2026-10-22T11:01:20Z"}]},
            {"name": "Windows", "conclusion": "failure",
             "started_at": "2026-10-22T11:01:00Z", "completed_at": "2026-10-22T11:03:00Z",
             "steps": []},
            {"name": "Release", "conclusion": "skipped"},
            {"name": "CI metrics", "conclusion": None},
        ]
        data = analyze_jobs(jobs)
        self.assertEqual((data["successful_jobs"], data["failed_jobs"]), (1, 1))
        self.assertEqual(data["critical_path_seconds"], 180)
        self.assertEqual(data["runner_seconds"], 240)
        self.assertEqual(data["categories"]["Dependencies / cache"], 50)
        self.assertEqual(data["categories"]["Build / container"], 30)
        self.assertIn("No JUnit artifacts available",
                      make_markdown(data, [], "https://github.com", "pull_request"))

    def test_junit_counts(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "ctest.xml"
            path.write_text('<testsuite tests="3" failures="1" errors="0" skipped="1">'
                            '<testcase name="ok"/><testcase name="fail"><failure/></testcase>'
                            '<testcase name="skip"><skipped/></testcase></testsuite>',
                            encoding="utf-8")
            cases = read_junit([path])
            self.assertEqual([cases[0][x] for x in ("tests", "failures", "skipped")], [3, 1, 1])
            self.assertIn("CTest cases | 3",
                          make_markdown(analyze_jobs([]), cases, "https://github.com", "offline"))

    def test_timestamp_and_classification(self):
        self.assertIsNone(seconds(None, "2026-10-22T11:00:00Z"))
        self.assertEqual(seconds("2026-10-22T11:00:01Z", "2026-10-22T11:00:02Z"), 1)
        self.assertEqual(category("Full OpenGL integration tests"), "CTest")


if __name__ == "__main__":
    unittest.main()
