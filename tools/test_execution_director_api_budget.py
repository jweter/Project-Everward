"""Execution Director GitHub ingest budgets and explicit API failure contracts.

Network calls are stubbed; these are deterministic tooling tests, not live
GitHub authorization or Unreal Product Reality evidence.
"""
from __future__ import annotations

import importlib.util
import os
import unittest
import urllib.error
from datetime import UTC, datetime, timedelta
from pathlib import Path
from unittest.mock import patch

DIRECTOR_PATH = Path(__file__).resolve().parents[1] / "engineering" / "execution_director.py"
SPEC = importlib.util.spec_from_file_location("everward_execution_director_api_budget", DIRECTOR_PATH)
assert SPEC is not None and SPEC.loader is not None
DIRECTOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(DIRECTOR)


class ExecutionDirectorApiBudgetTests(unittest.TestCase):
    def test_workflow_history_requests_only_recent_dates_and_filters_truth(self) -> None:
        recent = (datetime.now(UTC) - timedelta(hours=2)).isoformat()
        old = (datetime.now(UTC) - timedelta(days=28)).isoformat()
        responses = [
            {"id": 51, "name": "Native check", "head_sha": "a" * 40,
             "created_at": recent, "conclusion": "failure"},
            {"id": 52, "name": "Old job", "head_sha": "b" * 40,
             "created_at": old, "conclusion": "failure"},
            {"id": 53, "name": "Passing check", "head_sha": "c" * 40,
             "created_at": recent, "conclusion": "success"},
        ]
        with patch.object(DIRECTOR, "wrapped_pages", return_value=responses) as fetch:
            events = DIRECTOR.workflow_learning()
        self.assertEqual([event["run_id"] for event in events], [51])
        self.assertEqual(fetch.call_count, 1)
        path, key = fetch.call_args.args
        self.assertEqual(key, "workflow_runs")
        self.assertTrue(path.startswith("/actions/runs?"))
        self.assertIn("created=%3E%3D", path)
        self.assertIn("status=completed", path)

    def test_review_ingest_uses_one_repository_wide_feed_not_per_pr(self) -> None:
        recent = (datetime.now(UTC) - timedelta(hours=1)).isoformat()
        old = (datetime.now(UTC) - timedelta(days=60)).isoformat()
        comments = [
            {"id": 7, "created_at": recent, "body": "P1 Badge: broken",
             "pull_request_url": "https://api.github.com/repos/jweter/Project-Everward/pulls/418",
             "path": "unreal/Source/Everward/EverwardHUD.cpp"},
            {"id": 8, "created_at": recent, "body": "ordinary comment",
             "pull_request_url": "https://api.github.com/repos/jweter/Project-Everward/pulls/418"},
            {"id": 9, "created_at": old, "body": "P0 Badge: historical",
             "pull_request_url": "https://api.github.com/repos/jweter/Project-Everward/pulls/83"},
        ]
        with patch.object(DIRECTOR, "pages", return_value=comments) as fetch:
            events = DIRECTOR.review_learning()
        self.assertEqual(len(events), 1)
        self.assertEqual(events[0]["pull_request"], 418)
        self.assertEqual(events[0]["comment_id"], 7)
        self.assertEqual(events[0]["severity"], "P1")
        self.assertEqual(fetch.call_count, 1)
        path = fetch.call_args.args[0]
        self.assertTrue(path.startswith("/pulls/comments?"))
        self.assertIn("since=", path)

    def test_malformed_review_pointer_fails_closed(self) -> None:
        recent = datetime.now(UTC).isoformat()
        with patch.object(DIRECTOR, "pages", return_value=[{
            "created_at": recent, "body": "P0 Badge",
            "pull_request_url": "https://malicious.example/pulls/1",
        }]):
            with self.assertRaisesRegex(SystemExit, "no valid pull_request_url"):
                DIRECTOR.review_learning()

    def test_403_reports_rate_limit_and_never_fabricates_plan(self) -> None:
        error = urllib.error.HTTPError(
            "https://api.github.com/repos/jweter/Project-Everward/actions/runs",
            403, "Forbidden",
            {"X-RateLimit-Remaining": "0", "Retry-After": "60"},
            None,
        )
        with patch.dict(os.environ, {
            "GITHUB_TOKEN": "secret-test-token",
            "GITHUB_REPOSITORY": "jweter/Project-Everward",
        }), patch.object(DIRECTOR.urllib.request, "urlopen", side_effect=error):
            with self.assertRaises(SystemExit) as raised:
                DIRECTOR.github("/actions/runs?status=completed")
        self.assertIn("GitHub API 403", str(raised.exception))
        self.assertIn("rate_limit_remaining=0", str(raised.exception))
        self.assertIn("execution plan unverified", str(raised.exception))
        self.assertNotIn("secret-test-token", str(raised.exception))


if __name__ == "__main__":
    unittest.main()
