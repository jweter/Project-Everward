import importlib.util
import pathlib
import unittest

MODULE_PATH = pathlib.Path(__file__).with_name("coordinates.py")
spec = importlib.util.spec_from_file_location("coordinates", MODULE_PATH)
coordinates = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(coordinates)

CELL_SIZE_MM = coordinates.CELL_SIZE_MM
SpatialPosition = coordinates.SpatialPosition


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


if __name__ == "__main__":
    unittest.main()
