from __future__ import annotations

import argparse

from runner import HeadlessSimulation


def main() -> None:
    parser = argparse.ArgumentParser(description="Run the Everward headless simulation proof")
    parser.add_argument("--years", type=int, default=10_000)
    parser.add_argument("--seed", type=int, default=847291)
    args = parser.parse_args()

    summary = HeadlessSimulation(seed=args.seed, years=args.years).run()
    print(summary.canonical_json())


if __name__ == "__main__":
    main()
