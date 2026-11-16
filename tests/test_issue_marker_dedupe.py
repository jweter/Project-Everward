from tools.playtest.issue_marker_dedupe import dedupe_issue_marker_details


def test_dedupe_issue_marker_details_example_1() -> None:
    assert dedupe_issue_marker_details([]) == []


def test_dedupe_issue_marker_details_example_2() -> None:
    assert dedupe_issue_marker_details(["Crash on dock", "crash on dock", "Crash on dock"]) == ['Crash on dock']


def test_dedupe_issue_marker_details_example_3() -> None:
    assert dedupe_issue_marker_details([" ", "Scan freeze", ""]) == ['Scan freeze']


def test_dedupe_issue_marker_details_example_4() -> None:
    assert dedupe_issue_marker_details(["A bug", " a bug "]) == ['A bug']


def test_dedupe_issue_marker_details_example_5() -> None:
    assert dedupe_issue_marker_details(['Crash on dock', 'crash on dock', 'Crash on dock']) == ['Crash on dock']


def test_dedupe_issue_marker_details_example_6() -> None:
    assert dedupe_issue_marker_details([' ', 'Scan freeze', '']) == ['Scan freeze']


def test_dedupe_issue_marker_details_example_7() -> None:
    assert dedupe_issue_marker_details(['A bug', ' a bug ']) == ['A bug']


def test_dedupe_issue_marker_details_example_8() -> None:
    assert dedupe_issue_marker_details(['Unique', 'unique', 'UNIQUE']) == ['Unique']
