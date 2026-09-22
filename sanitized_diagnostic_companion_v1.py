import os
import subprocess
import sys

def run_command(command):
    try:
        result = subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        return result.stdout
    except subprocess.CalledProcessError as e:
        return f"Error: {e.stderr}"

def verify_repository_state():
    print("Verifying repository state...")
    # Add commands to verify repository state
    # Example: Check for uncommitted changes, open PRs, etc.
    return run_command(["git", "status"])

def run_targeted_verification():
    print("Running targeted verification...")
    # Add commands to run targeted verification
    # Example: Run specific tests, check for regressions, etc.
    return run_command(["python", "tests/targeted_verification.py"])

def run_full_preflight():
    print("Running full preflight...")
    # Add commands to run full preflight
    # Example: Run all tests, check for build issues, etc.
    return run_command(["python", "tests/full_preflight.py"])

def main():
    print("Starting sanitized-diagnostic-companion-v1...")
    repository_state = verify_repository_state()
    print("Repository State:\n", repository_state)

    targeted_verification = run_targeted_verification()
    print("Targeted Verification:\n", targeted_verification)

    full_preflight = run_full_preflight()
    print("Full Preflight:\n", full_preflight)

if __name__ == "__main__":
    main()
