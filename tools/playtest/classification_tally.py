from typing import List, Dict

def tally_playtest_classifications(classifications: List[str]) -> Dict[str, int]:
    """Return a dict with keys BLOCKER, FAIL, PARTIAL, PASS, UNCLASSIFIED mapped to counts of classifications in each, after stripping whitespace and uppercasing for comparison. Entries not matching BLOCKER, FAIL, PARTIAL, or PASS -- including empty, whitespace-only, non-string, or unrecognized text -- count as UNCLASSIFIED. An empty list returns every key as 0. Comparison is case-insensitive."""
    counts = {
        'BLOCKER': 0,
        'FAIL': 0,
        'PARTIAL': 0,
        'PASS': 0,
        'UNCLASSIFIED': 0
    }
    
    for classification in classifications:
        if isinstance(classification, str):
            stripped = classification.strip().upper()
            if stripped in counts:
                counts[stripped] += 1
            else:
                counts['UNCLASSIFIED'] += 1
        else:
            counts['UNCLASSIFIED'] += 1
    
    return counts
