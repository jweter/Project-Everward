from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "run_unattended_product_reality.ps1"


class PortfolioActorBootstrapTests(unittest.TestCase):
    def test_existing_worker_bootstraps_portfolio_actor(self) -> None:
        source = RUNNER.read_text(encoding="utf-8")
        self.assertIn("Ensure-PortfolioActor", source)
        self.assertIn("jweter/project-orchestrator.git", source)
        self.assertIn("ProjectOrchestrator\\actor", source)
        self.assertIn("fetch --prune origin main", source)
        self.assertIn("checkout --detach origin/main", source)
        self.assertIn("scripts\\install_laptop_actor.ps1", source)

    def test_bootstrap_fails_closed_without_disabling_everward(self) -> None:
        source = RUNNER.read_text(encoding="utf-8")
        self.assertIn("bootstrap failed closed", source)
        bootstrap = source.index("Ensure-PortfolioActor")
        worker = source.index("& $Python $Worker --repo-root $RepoRoot")
        self.assertLess(bootstrap, worker)
        self.assertIn("Everward verification will continue normally", source)

    def test_bootstrap_revalidates_origin_and_auth(self) -> None:
        source = RUNNER.read_text(encoding="utf-8")
        self.assertIn("auth status -h github.com", source)
        self.assertIn("remote get-url origin", source)
        self.assertIn("https://github.com/jweter/project-orchestrator", source)


if __name__ == "__main__":
    unittest.main()
