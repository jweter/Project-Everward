"""No Unreal installation required: deterministic free agent stack contract tests."""
from __future__ import annotations

import importlib.util
import hashlib
import json
import pathlib
import tempfile
import unittest
from unittest.mock import patch

MODULE = pathlib.Path(__file__).with_name("unreal_free_agent_stack.py")
spec = importlib.util.spec_from_file_location("unreal_free_agent_stack", MODULE)
assert spec and spec.loader
stack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(stack)


class FreeAgentStackTests(unittest.TestCase):
    def test_all_31_skills_declared(self):
        self.assertEqual(len(stack.EXPECTED_SKILLS), 31)
        self.assertIn("ue-testing-debugging", stack.EXPECTED_SKILLS)
        self.assertIn("ue-project-context", stack.EXPECTED_SKILLS)

    def test_inventory_fails_without_skills(self):
        with tempfile.TemporaryDirectory() as tmp:
            inventory = stack.skills_inventory(pathlib.Path(tmp))
        self.assertFalse(inventory["ready"])
        self.assertEqual(len(inventory["missing"]), 31)

    def test_inventory_rejects_nonmarkdown_payloads(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            skills = root / ".agents/skills"
            skills.mkdir(parents=True)
            for name in stack.EXPECTED_SKILLS:
                folder = skills / name
                folder.mkdir()
                (folder / "SKILL.md").write_text("---\nname: " + name, encoding="utf-8")
            (skills / "QUODSOLER-MIT-LICENSE.txt").write_text("MIT", encoding="utf-8")
            files = list(skills.glob("ue-*/**/*.md")) + [skills / "QUODSOLER-MIT-LICENSE.txt"]
            manifest = {"commit": stack.PINNED_SHA, "files": {
                p.relative_to(skills).as_posix(): hashlib.sha256(p.read_bytes().replace(b"\r\n", b"\n")).hexdigest()
                for p in files}}
            (skills / "UPSTREAM_LOCK.json").write_text(json.dumps(manifest), encoding="utf-8")
            self.assertTrue(stack.skills_inventory(root)["ready"])
            (skills / "ue-testing-debugging" / "bad.py").write_text("pass", encoding="utf-8")
            self.assertFalse(stack.skills_inventory(root)["ready"])

    def test_planned_automation_is_bounded(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            paths = stack.unreal_paths(root / "UE_5.8", root / "Everward.uproject")
            plan = stack.native_command("automation", paths, root / "evidence", test="Everward.Mining")
        self.assertTrue(any("Automation RunTests Everward.Mining" in p for p in plan))
        self.assertIn("-NullRHI", plan)
        self.assertFalse(any("Aura" in p or "Claude" in p for p in plan))

    def test_no_console_injection(self):
        paths = stack.unreal_paths(pathlib.Path("/UE"), pathlib.Path("/game.uproject"))
        for name in ("Automation RunTests Foo;rm -rf", "../Test", "P; Quit", "A" * 101):
            with self.assertRaises(ValueError):
                stack.native_command("automation", paths, pathlib.Path("/evidence"), test=name)

    def test_gauntlet_requires_existing_named_test(self):
        paths = stack.unreal_paths(pathlib.Path("/UE"), pathlib.Path("/game.uproject"))
        with self.assertRaises(ValueError):
            stack.native_command("gauntlet", paths, pathlib.Path("/evidence"))
        args = stack.native_command("gauntlet", paths, pathlib.Path("/evidence"), test="Everward.Smoke")
        self.assertIn("-test=Everward.Smoke", args)

    def test_unrecognized_native_action_fails(self):
        with self.assertRaises(ValueError):
            stack.native_command("aura", {}, pathlib.Path("."))

    def test_mcp_denies_nonloopback_servers(self):
        with self.assertRaises(ValueError):
            stack.mcp_discover(url="https://mcp.remote.example/mcp")

    def test_mcp_sse_parser(self):
        output = stack._parse_mcp_body(
            b'event: message\ndata: {"jsonrpc":"2.0","id":1,"result":{"tools":[]}}\n\n',
            "text/event-stream")
        self.assertEqual(output["result"]["tools"], [])

    def test_mcp_discovery_calls_no_editor_tools(self):
        calls = []
        class FakeResponse:
            headers = {"Content-Type": "application/json", "Mcp-Session-Id": "local"}
            def __init__(self, data):
                self.data = data
            def __enter__(self):
                return self
            def __exit__(self, *_):
                return None
            def read(self, *_):
                return json.dumps(self.data).encode()
        def opener(req, *, timeout):
            payload = json.loads(req.data)
            calls.append(payload["method"])
            if payload["method"] == "initialize":
                return FakeResponse({"jsonrpc": "2.0", "id": 1, "result": {"serverInfo": {"name": "Unreal"}}})
            if payload["method"] == "tools/list":
                return FakeResponse({"jsonrpc": "2.0", "id": 2, "result": {"tools": [
                    {"name": "list_toolsets"}, {"name": "call_tool"}]}})
            return FakeResponse({})
        result = stack.mcp_discover(opener=opener)
        self.assertEqual(result["status"], "CONNECTED_READ_ONLY")
        self.assertEqual(calls, ["initialize", "notifications/initialized", "tools/list"])
        self.assertFalse(result["write_executed"])

    @patch.object(stack, "require_isolated_checkout", side_effect=ValueError("primary checkout"))
    def test_native_runner_does_not_bypass_isolation(self, _):
        with self.assertRaisesRegex(ValueError, "primary checkout"):
            stack.run_native("automation", pathlib.Path("/repo"), pathlib.Path("/UE"),
                             pathlib.Path("/repo/unreal/Everward.uproject"),
                             pathlib.Path("/tmp/evidence"), test="Everward")

    def test_windows_tree_termination_uses_only_spawned_pid(self):
        if stack.os.name != "nt":
            self.skipTest("Windows-only process-tree command")
        class FakeProcess:
            pid = 43210
            def poll(self):
                return None
            def wait(self, timeout):
                return 1
        with patch.object(stack.subprocess, "run", return_value=type(
            "Completed", (), {"returncode": 0})()) as runner:
            stack.terminate_native_tree(FakeProcess())
        self.assertEqual(runner.call_args.args[0],
                         ["taskkill", "/PID", "43210", "/T", "/F"])

    def test_timeout_spools_logs_and_calls_tree_terminator(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp) / "repo"
            root.mkdir()
            project = root / "unreal/Everward.uproject"
            project.parent.mkdir()
            project.write_text("{}", encoding="utf-8")
            engine = pathlib.Path(tmp) / "UE"
            paths = stack.unreal_paths(engine, project)
            for key in ("editor", "ubt", "uat"):
                paths[key].parent.mkdir(parents=True, exist_ok=True)
                paths[key].write_text("stub", encoding="utf-8")
            class FakeProcess:
                pid = 43210
                def poll(self):
                    return None
                def wait(self, timeout):
                    raise stack.subprocess.TimeoutExpired("native", timeout)
            with patch.object(stack, "require_isolated_checkout", return_value="a" * 40), \
                 patch.object(stack, "resource_status", return_value={"admitted": True}), \
                 patch.object(stack.subprocess, "Popen", return_value=FakeProcess()), \
                 patch.object(stack, "terminate_native_tree") as killer, \
                 patch.object(stack.tempfile, "gettempdir", return_value=tmp):
                result = stack.run_native(
                    "automation", root, engine, project, pathlib.Path(tmp) / "evidence",
                    test="Everward", timeout_seconds=1)
            killer.assert_called_once()
            self.assertEqual(result["status"], "ENVIRONMENT_FAILURE")
            self.assertFalse((pathlib.Path(tmp) / "everward-native-agent.lock").exists())

    def test_cli_is_inspect_only_by_default(self):
        self.assertEqual(stack.main(["--root", str(MODULE.parent.parent), "--mode", "inspect"]), 0)


if __name__ == "__main__":
    unittest.main()
