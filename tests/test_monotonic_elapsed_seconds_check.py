from tools.playtest.monotonic_elapsed_seconds_check import find_first_nonmonotonic_elapsed_seconds


def test_find_first_nonmonotonic_elapsed_seconds_example_1() -> None:
    assert find_first_nonmonotonic_elapsed_seconds([]) is None


def test_find_first_nonmonotonic_elapsed_seconds_example_2() -> None:
    assert find_first_nonmonotonic_elapsed_seconds([5.0]) is None


def test_find_first_nonmonotonic_elapsed_seconds_example_3() -> None:
    assert find_first_nonmonotonic_elapsed_seconds([0.0, 1.5, 1.5, 3.0]) is None


def test_find_first_nonmonotonic_elapsed_seconds_example_4() -> None:
    assert find_first_nonmonotonic_elapsed_seconds([0.0, 2.0, 1.0, 5.0]) == 2


def test_find_first_nonmonotonic_elapsed_seconds_example_5() -> None:
    assert find_first_nonmonotonic_elapsed_seconds([1.0, 1.0, 1.0, 1.0]) is None
