from __future__ import annotations

def find_first_nonmonotonic_elapsed_seconds(elapsed_seconds: list[float]) -> int | None:
    """Return the index of the first element less than the element immediately before it in a list of elapsed-second numbers, or None if every element is greater than or equal to the one before it. An empty list or a single-element list returns None. Equal consecutive values are not a violation; non-decreasing order is required, not strictly increasing. Comparison uses native numeric ordering. Supports verifying that a recorded first-loop playtest's event timeline never goes backward. Keep it a pure function with a docstring."""
    if len(elapsed_seconds) < 2:
        return None
    for i in range(1, len(elapsed_seconds)):
        if elapsed_seconds[i] < elapsed_seconds[i - 1]:
            return i
    return None
