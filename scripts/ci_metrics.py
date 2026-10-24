#!/usr/bin/env python3
"""Summarize real CTest results and GitHub Actions job/step durations.

Uses only the Python standard library. No personal access token or third-party
service is required; the workflow provides its own read-only GITHUB_TOKEN.
"""
import argparse
from collections import defaultdict
from datetime import datetime
import json
import os
from pathlib import Path
import sys
from urllib.request import Request, urlopen
import xml.etree.ElementTree as ET


def seconds(start, end):
    if not start or not end:
        return None
    def parse(value):
        return datetime.fromisoformat(value.replace("Z", "+00:00"))
    return max(0.0, (parse(end) - parse(start)).total_seconds())


def category(step):
    name = step.lower()
    if any(x in name for x in ("install ", "restore ", "cache ", "setup ", "dependencies", "checkout", "software renderer")):
        return "Dependencies / cache"
    if "configure" in name:
        return "CMake configuration"
    if name == "build" or "build, test and optionally publish" in name:
        return "Build / container"
    if "cpu, audio" in name or "integration tests" in name:
        return "CTest"
    if "mp4 export" in name or "stage profile" in name:
        return "Render / export smoke"
    if any(x in name for x in ("package", "artifact", "release")):
        return "Packaging / artifacts"
    return "Other"


def analyze_jobs(jobs):
    # This job is still in progress when it queries the API.
    selected = [j for j in jobs if j.get("name") != "CI metrics" and j.get("conclusion") != "skipped"]
    completed = [j for j in selected if seconds(j.get("started_at"), j.get("completed_at")) is not None]
    durations = [(j.get("name", "unknown"), seconds(j["started_at"], j["completed_at"]),
                  j.get("conclusion", "unknown")) for j in completed]
    steps, categories = [], defaultdict(float)
    for job in selected:
        for step in job.get("steps") or []:
            duration = seconds(step.get("started_at"), step.get("completed_at"))
            if duration is None or step.get("conclusion") == "skipped":
                continue
            categories[category(step.get("name", ""))] += duration
            steps.append((job.get("name", "unknown"), step.get("name", "unknown"),
                          duration, step.get("conclusion", "unknown")))
    starts = [datetime.fromisoformat(j["started_at"].replace("Z", "+00:00")) for j in completed]
    ends = [datetime.fromisoformat(j["completed_at"].replace("Z", "+00:00")) for j in completed]
    return {
        "jobs": durations, "steps": steps, "categories": dict(categories),
        "runner_seconds": sum(d for _, d, _ in durations),
        "critical_path_seconds": (max(ends) - min(starts)).total_seconds() if completed else 0.0,
        "successful_jobs": sum(j.get("conclusion") == "success" for j in selected),
        "failed_jobs": sum(j.get("conclusion") == "failure" for j in selected),
        "cancelled_jobs": sum(j.get("conclusion") == "cancelled" for j in selected),
    }


def read_junit(paths):
    results = []
    for path in sorted(set(Path(p) for p in paths)):
        root = ET.parse(path).getroot()
        suites = [root] if root.tag == "testsuite" else list(root.findall("testsuite"))
        if not suites:
            raise ValueError(f"No JUnit testsuite element in {path}")
        for suite in suites:
            cases = list(suite.iter("testcase"))
            results.append({
                "source": str(path),
                "tests": int(suite.get("tests", len(cases))),
                "failures": int(suite.get("failures", sum(c.find("failure") is not None for c in cases))),
                "errors": int(suite.get("errors", sum(c.find("error") is not None for c in cases))),
                "skipped": int(suite.get("skipped", sum(c.find("skipped") is not None for c in cases))),
            })
    return results


def make_markdown(data, junit, url, event):
    lines = ["## CI performance and test coverage", "",
             f"[View workflow run]({url}) · Event: `{event}`", "",
             "| Metric | Measured result |", "|---|---:|",
             f"| Jobs passed / failed / cancelled | {data['successful_jobs']} / {data['failed_jobs']} / {data['cancelled_jobs']} |",
             f"| Critical path (excluding metrics job) | {data['critical_path_seconds']:.1f} s |",
             f"| Sum of job execution time (parallel runners) | {data['runner_seconds']:.1f} s |"]
    if junit:
        lines.extend([
            f"| CTest cases | {sum(x['tests'] for x in junit)} |",
            f"| CTest failures + errors | {sum(x['failures'] + x['errors'] for x in junit)} |",
            f"| CTest skipped | {sum(x['skipped'] for x in junit)} |",
        ])
    else:
        lines.append("| CTest cases | No JUnit artifacts available (check build/test failures) |")
    lines.extend(["", "### Jobs (longest first)", "",
                  "| Job | Elapsed | Result |", "|---|---:|---|"])
    for name, duration, result in sorted(data["jobs"], key=lambda x: x[1], reverse=True):
        lines.append(f"| {name} | {duration:.1f} s | {result} |")
    lines.extend(["", "### Where runner time is spent", "",
                  "| Step group | Sum across jobs |", "|---|---:|"])
    for name, duration in sorted(data["categories"].items(), key=lambda x: x[1], reverse=True):
        lines.append(f"| {name} | {duration:.1f} s |")
    lines.extend(["", "### Slowest individual steps", "",
                  "| Job | Step | Duration |", "|---|---|---:|"])
    for job, step, duration, _ in sorted(data["steps"], key=lambda x: x[2], reverse=True)[:10]:
        lines.append(f"| {job} | {step} | {duration:.1f} s |")
    lines.extend(["", "Step group totals include parallel jobs and are not wall-clock time. "
                  "JUnit totals count uploaded reports only: a build failing before CTest "
                  "cannot provide test cases. No FPS regression threshold is enforced "
                  "on shared CI runners."])
    return "\n".join(lines) + "\n"


def fetch_jobs(repo, run_id, token):
    if not token:
        raise RuntimeError("GITHUB_TOKEN is required to retrieve job timings")
    if repo.count("/") != 1 or not run_id.isdigit():
        raise ValueError("Invalid repository or workflow run ID")
    jobs = []
    for page in range(1, 11):
        url = f"https://api.github.com/repos/{repo}/actions/runs/{run_id}/jobs?per_page=100&page={page}"
        request = Request(url, headers={
            "Authorization": f"Bearer {token}",
            "Accept": "application/vnd.github+json",
            "X-GitHub-Api-Version": "2022-11-28",
            "User-Agent": "project-opengl-ci-metrics"})
        with urlopen(request, timeout=25) as response:
            batch = json.load(response)["jobs"]
        jobs.extend(batch)
        if len(batch) < 100:
            break
    return jobs


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--jobs-json", type=Path, help="Offline GitHub jobs API fixture")
    parser.add_argument("--junit-dir", type=Path, default=Path("build/test-reports"))
    parser.add_argument("--output", type=Path, default=Path("ci-metrics.json"))
    args = parser.parse_args(argv)
    if args.jobs_json:
        blob = json.loads(args.jobs_json.read_text(encoding="utf-8"))
        jobs = blob["jobs"] if isinstance(blob, dict) else blob
    else:
        jobs = fetch_jobs(os.environ["GITHUB_REPOSITORY"], os.environ["GITHUB_RUN_ID"],
                          os.getenv("GITHUB_TOKEN"))
    junit = read_junit(args.junit_dir.rglob("*.xml")) if args.junit_dir.is_dir() else []
    data = analyze_jobs(jobs)
    data["junit"] = junit
    args.output.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    summary = make_markdown(data, junit, os.getenv("GITHUB_RUN_URL", "https://github.com"),
                            os.getenv("GITHUB_EVENT_NAME", "offline"))
    print(summary)
    if os.getenv("GITHUB_STEP_SUMMARY"):
        with open(os.environ["GITHUB_STEP_SUMMARY"], "a", encoding="utf-8") as stream:
            stream.write(summary)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, RuntimeError, ET.ParseError) as exc:
        print(f"CI metrics report failed: {exc}", file=sys.stderr)
        sys.exit(1)
