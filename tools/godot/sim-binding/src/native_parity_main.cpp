// Native reference run of the parity scenario (no Godot). Prints one result per line.
#include "parity_scenario.hpp"

#include <cstdio>

int main() {
    for (const std::string& l : everward::binding::run_native_parity()) std::printf("%s\n", l.c_str());
    return 0;
}
