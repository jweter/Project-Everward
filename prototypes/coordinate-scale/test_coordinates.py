import unittest

from coordinates import AXIS_COUNT, CELL_SIZE_MM, VECTOR_FIELDS, SpatialPosition

# Pinned expectations for the declared shape contract. Kept as literals here so
# a change to the module constants is a visible test failure rather than a
# silently co-varying assertion.
EXPECTED_AXIS_COUNT = 3
EXPECTED_VECTOR_FIELDS = ("cell", "offset_mm")

VALID_VECTOR_VALUES = {"cell": (0, 0, 0), "offset_mm": (0, 0, 0)}
SHAPE_ERROR = "cell and offset_mm must be 3D"
NORMALIZATION_ERROR = "offset_mm must be normalized into the containing cell"

# Values that must be rejected as an offset on any axis: below the cell, exactly
# at the exclusive upper bound, and above it.
NON_NORMALIZED_OFFSET_VALUES = (-1, -CELL_SIZE_MM, CELL_SIZE_MM, CELL_SIZE_MM + 1)

# Axis counts that are not AXIS_COUNT and must be rejected wherever a 3-vector
# is required.
WRONG_AXIS_COUNTS = (0, 1, 2, 4)


def _vector_of_length(length: int) -> tuple:
    return tuple(range(length))


class SpatialPositionTests(unittest.TestCase):
    def test_round_trip_across_interstellar_scale(self):
        light_year_mm = 9_460_730_472_580_800 * 1000
        source = (light_year_mm * 250, -light_year_mm * 125, light_year_mm * 3)
        position = SpatialPosition.from_total_mm(source)
        self.assertEqual(position.total_mm(), source)

    def test_negative_coordinates_normalize(self):
        position = SpatialPosition.from_total_mm((-1, -CELL_SIZE_MM - 1, 0))
        self.assertEqual(position.total_mm(), (-1, -CELL_SIZE_MM - 1, 0))
        self.assertTrue(all(0 <= value < CELL_SIZE_MM for value in position.offset_mm))

    def test_millimetre_translation_survives_huge_absolute_position(self):
        huge = 9_460_730_472_580_800 * 1000 * 100_000
        start = SpatialPosition.from_total_mm((huge, -huge, huge))
        moved = start.translated_mm((1, -2, 3))
        self.assertEqual(start.delta_mm_to(moved), (1, -2, 3))

    def test_render_conversion_subtracts_before_float_conversion(self):
        huge = 9_460_730_472_580_800 * 1000 * 1_000_000
        origin = SpatialPosition.from_total_mm((huge, huge, huge))
        target = origin.translated_mm((1250, -2500, 1))
        self.assertEqual(target.render_relative_m(origin), (1.25, -2.5, 0.001))

    def test_cell_boundary_translation_is_continuous(self):
        start = SpatialPosition.from_total_mm((CELL_SIZE_MM - 2, 0, 0))
        moved = start.translated_mm((5, 0, 0))
        self.assertEqual(moved.cell[0], 1)
        self.assertEqual(moved.offset_mm[0], 3)
        self.assertEqual(start.delta_mm_to(moved), (5, 0, 0))

    def test_invalid_offset_is_rejected(self):
        with self.assertRaises(ValueError):
            SpatialPosition((0, 0, 0), (CELL_SIZE_MM, 0, 0))


class ShapeContractTests(unittest.TestCase):
    """Cover the declared axis/vector shape rules for every axis and field.

    `PROOF.md` lists "rejection of non-normalized offsets" as a property under
    test, but the only pre-existing negative test placed the bad value on the x
    axis and only at the exclusive upper bound. The 3D shape checks in
    `__post_init__`, `from_total_mm`, and `translated_mm` had no coverage at
    all. These tests exercise every declared axis and every declared vector
    field independently, and pin the declared constants themselves.
    """

    def test_axis_count_is_exactly_three(self):
        self.assertEqual(AXIS_COUNT, EXPECTED_AXIS_COUNT)

    def test_vector_fields_are_exactly_the_two_expected_fields(self):
        self.assertEqual(tuple(VECTOR_FIELDS), EXPECTED_VECTOR_FIELDS)

    def test_every_axis_rejects_a_non_normalized_offset(self):
        for axis in range(AXIS_COUNT):
            for bad_value in NON_NORMALIZED_OFFSET_VALUES:
                with self.subTest(axis=axis, value=bad_value):
                    offset = [0] * AXIS_COUNT
                    offset[axis] = bad_value
                    with self.assertRaises(ValueError) as context:
                        SpatialPosition((0, 0, 0), tuple(offset))
                    self.assertEqual(str(context.exception), NORMALIZATION_ERROR)

    def test_every_axis_accepts_the_largest_normalized_offset(self):
        for axis in range(AXIS_COUNT):
            with self.subTest(axis=axis):
                offset = [0] * AXIS_COUNT
                offset[axis] = CELL_SIZE_MM - 1
                position = SpatialPosition((0, 0, 0), tuple(offset))
                self.assertEqual(position.offset_mm[axis], CELL_SIZE_MM - 1)

    def test_every_declared_vector_field_rejects_a_wrong_axis_count(self):
        for field in VECTOR_FIELDS:
            for length in WRONG_AXIS_COUNTS:
                with self.subTest(field=field, length=length):
                    values = dict(VALID_VECTOR_VALUES)
                    values[field] = _vector_of_length(length)
                    with self.assertRaises(ValueError) as context:
                        SpatialPosition(**values)
                    self.assertEqual(str(context.exception), SHAPE_ERROR)

    def test_from_total_mm_rejects_a_wrong_axis_count(self):
        for length in WRONG_AXIS_COUNTS:
            with self.subTest(length=length):
                with self.assertRaises(ValueError) as context:
                    SpatialPosition.from_total_mm(_vector_of_length(length))
                self.assertEqual(
                    str(context.exception), "xyz_mm must contain exactly three axes"
                )

    def test_translated_mm_rejects_a_wrong_axis_count(self):
        start = SpatialPosition.from_total_mm((0, 0, 0))
        for length in WRONG_AXIS_COUNTS:
            with self.subTest(length=length):
                with self.assertRaises(ValueError) as context:
                    start.translated_mm(_vector_of_length(length))
                self.assertEqual(
                    str(context.exception), "delta_mm must contain exactly three axes"
                )


if __name__ == "__main__":
    unittest.main()
