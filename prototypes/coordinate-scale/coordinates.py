from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable, Tuple

Vec3i = Tuple[int, int, int]
Vec3f = Tuple[float, float, float]

# Large enough to keep local scenes far from cell boundaries while remaining
# much smaller than interstellar distances.
CELL_SIZE_M = 1_000_000_000_000  # 1e12 m
MM_PER_M = 1_000
CELL_SIZE_MM = CELL_SIZE_M * MM_PER_M

# Number of spatial axes in an authoritative position. Named so that the
# per-axis shape and normalization rules below can be covered independently for
# every axis instead of only the first one.
AXIS_COUNT = 3

# The integer vector fields of SpatialPosition that must each carry exactly
# AXIS_COUNT axes. Declared as a module-level constant (matching the
# NUMERIC_NONNEGATIVE_FIELDS / HARDWARE_FIELDS / ANIMATION_PERIOD_FIELDS
# convention used elsewhere in the prototypes) so the declared set itself can be
# pinned by test.
VECTOR_FIELDS = ("cell", "offset_mm")


def _split_axis(total_mm: int) -> tuple[int, int]:
    cell, offset = divmod(total_mm, CELL_SIZE_MM)
    return cell, offset


@dataclass(frozen=True, slots=True)
class SpatialPosition:
    """Deterministic authoritative position across astronomical scales.

    Absolute position is represented as integer cell coordinates plus integer
    millimetre offsets. Presentation code converts only relative deltas to
    floating point after rebasing around a nearby origin.
    """

    cell: Vec3i
    offset_mm: Vec3i

    def __post_init__(self) -> None:
        if any(len(getattr(self, name)) != AXIS_COUNT for name in VECTOR_FIELDS):
            raise ValueError("cell and offset_mm must be 3D")
        if any(not 0 <= value < CELL_SIZE_MM for value in self.offset_mm):
            raise ValueError("offset_mm must be normalized into the containing cell")

    @classmethod
    def from_total_mm(cls, xyz_mm: Iterable[int]) -> "SpatialPosition":
        values = tuple(int(v) for v in xyz_mm)
        if len(values) != AXIS_COUNT:
            raise ValueError("xyz_mm must contain exactly three axes")
        pairs = tuple(_split_axis(v) for v in values)
        return cls(
            cell=(pairs[0][0], pairs[1][0], pairs[2][0]),
            offset_mm=(pairs[0][1], pairs[1][1], pairs[2][1]),
        )

    def total_mm(self) -> Vec3i:
        values = tuple(
            self.cell[i] * CELL_SIZE_MM + self.offset_mm[i] for i in range(AXIS_COUNT)
        )
        return values  # type: ignore[return-value]

    def translated_mm(self, delta_mm: Iterable[int]) -> "SpatialPosition":
        delta = tuple(int(v) for v in delta_mm)
        if len(delta) != AXIS_COUNT:
            raise ValueError("delta_mm must contain exactly three axes")
        total = self.total_mm()
        return SpatialPosition.from_total_mm(
            total[i] + delta[i] for i in range(AXIS_COUNT)
        )

    def delta_mm_to(self, other: "SpatialPosition") -> Vec3i:
        here = self.total_mm()
        there = other.total_mm()
        values = tuple(there[i] - here[i] for i in range(AXIS_COUNT))
        return values  # type: ignore[return-value]

    def render_relative_m(self, origin: "SpatialPosition") -> Vec3f:
        """Map authoritative position to local presentation coordinates.

        Exact integer subtraction occurs before conversion to floating point,
        preventing a huge absolute coordinate from erasing local precision.
        """

        delta_mm = origin.delta_mm_to(self)
        values = tuple(value / MM_PER_M for value in delta_mm)
        return values  # type: ignore[return-value]
