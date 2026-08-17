from __future__ import annotations

from generator import generate_system


def main() -> None:
    system = generate_system(847291, (18, -4, 71))
    print(f"system_id={system.system_id}")
    print(f"fingerprint={system.fingerprint()}")
    print(
        "star="
        f"{system.star.spectral_class} "
        f"mass={system.star.mass_milli_solar / 1000:.3f} M_sun "
        f"temp={system.star.temperature_k} K"
    )
    print(f"planets={len(system.planets)} belts={len(system.belts)}")
    for planet in system.planets:
        print(
            f"planet {planet.ordinal}: {planet.planet_type} "
            f"a={planet.semi_major_axis_milli_au / 1000:.3f} AU "
            f"mass={planet.mass_milli_earth / 1000:.3f} M_earth "
            f"temp={planet.equilibrium_temp_k} K moons={len(planet.moons)}"
        )


if __name__ == "__main__":
    main()
