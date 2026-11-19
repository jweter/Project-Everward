from tools.playtest.classification_tally import tally_playtest_classifications


def test_tally_playtest_classifications_example_1() -> None:
    assert tally_playtest_classifications([]) == {'BLOCKER': 0, 'FAIL': 0, 'PARTIAL': 0, 'PASS': 0, 'UNCLASSIFIED': 0}


def test_tally_playtest_classifications_example_2() -> None:
    assert tally_playtest_classifications(["blocker", "BLOCKER", "fail"]) == {'BLOCKER': 2, 'FAIL': 1, 'PARTIAL': 0, 'PASS': 0, 'UNCLASSIFIED': 0}


def test_tally_playtest_classifications_example_3() -> None:
    assert tally_playtest_classifications(["weird", "", " "]) == {'BLOCKER': 0, 'FAIL': 0, 'PARTIAL': 0, 'PASS': 0, 'UNCLASSIFIED': 3}


def test_tally_playtest_classifications_example_4() -> None:
    assert tally_playtest_classifications([" pass ", "Partial"]) == {'BLOCKER': 0, 'FAIL': 0, 'PARTIAL': 1, 'PASS': 1, 'UNCLASSIFIED': 0}


def test_tally_playtest_classifications_example_5() -> None:
    assert tally_playtest_classifications(['blocker', 'BLOCKER', 'fail']) == {'BLOCKER': 2, 'FAIL': 1, 'PARTIAL': 0, 'PASS': 0, 'UNCLASSIFIED': 0}


def test_tally_playtest_classifications_example_6() -> None:
    assert tally_playtest_classifications(['weird', '', ' ']) == {'BLOCKER': 0, 'FAIL': 0, 'PARTIAL': 0, 'PASS': 0, 'UNCLASSIFIED': 3}


def test_tally_playtest_classifications_example_7() -> None:
    assert tally_playtest_classifications([' pass ', 'Partial']) == {'BLOCKER': 0, 'FAIL': 0, 'PARTIAL': 1, 'PASS': 1, 'UNCLASSIFIED': 0}


def test_tally_playtest_classifications_example_8() -> None:
    assert tally_playtest_classifications(['BLOCKER', 'FAIL', 'PARTIAL', 'PASS', 'UNCLASSIFIED']) == {'BLOCKER': 1, 'FAIL': 1, 'PARTIAL': 1, 'PASS': 1, 'UNCLASSIFIED': 1}


def test_non_string_entries_are_counted_as_unclassified() -> None:
    assert tally_playtest_classifications([None, 0, False, {}, [], 'PASS']) == {
        'BLOCKER': 0, 'FAIL': 0, 'PARTIAL': 0, 'PASS': 1, 'UNCLASSIFIED': 5
    }
