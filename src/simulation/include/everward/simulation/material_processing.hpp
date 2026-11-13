#pragma once

#include "everward/simulation/impact_damage.hpp"
#include "everward/simulation/manipulator.hpp"
#include "everward/simulation/material_consumption.hpp"
#include "everward/simulation/types.hpp"

#include <array>
#include <cmath>
#include <map>
#include <stdexcept>
#include <string>

namespace everward::simulation {

// Issue #356 (Slice 13): after collecting real named materials, the probe
// must be able to process them into a refined feedstock and fabricate one
// functional component from that feedstock, deterministically and without
// creating mass. This bridges mining/storage into Fix_It's replacement path
// (fix_it.hpp's FixItResources::fabrication_available), which has existed
// as an inert flag with no real system behind it until now.
//
// Processing and fabrication are modeled as the same two-stage mechanic:
// consume a named input material, hold stored energy and manipulator-tool
// hardware for a required duration, then produce a named output material
// (a refined feedstock, or a fabricated component) through the existing
// authoritative SimulationCore material/energy mutators -- the same
// boundary mining.hpp and fix_it.hpp already use. Output mass per recipe is
// validated at registration to never exceed input mass.
enum class MaterialProcessingStage {
    Processing,
    Fabrication
};

struct MaterialProcessingRecipe {
    std::string recipe_id;
    std::string display_name;
    MaterialProcessingStage stage{MaterialProcessingStage::Processing};
    std::string input_material_id;
    double input_kg{0.0};
    std::string output_material_id;
    double output_kg{0.0};
    double energy_required_j{0.0};
    double time_required_s{0.0};
    // Fabrication issue #356's own acceptance text: "no fabrication succeeds
    // with ... invalid tool position". Processing (internal refining) does
    // not need the manipulator; fabrication (assembling a physical
    // component) does.
    bool requires_deployed_tool{false};
};

enum class MaterialProcessingJobState {
    Idle,
    Running,
    Completed,
    Interrupted
};

struct MaterialProcessingJobStatus {
    MaterialProcessingJobState state{MaterialProcessingJobState::Idle};
    std::string recipe_id;
    MaterialProcessingStage stage{MaterialProcessingStage::Processing};
    double elapsed_s{0.0};
    double material_consumed_kg{0.0};
    double energy_consumed_j{0.0};
    std::string detail;
};

struct MaterialProcessingStartResult {
    bool accepted{false};
    std::string detail;
};

// Engine-independent; Unreal only presents commands/telemetry, matching the
// convention established by mining.hpp and fix_it.hpp.
class MaterialProcessingSystem {
public:
    [[nodiscard]] static std::array<MaterialProcessingRecipe, 2> canonical_generation1_recipes() {
        return {{
            {
                "process_regolith_to_alloy",
                "Smelt raw regolith into structural alloy",
                MaterialProcessingStage::Processing,
                "raw_regolith", 10.0,
                "structural_alloy", 6.0,
                3.0e6, 300.0,
                false,
            },
            {
                "fabricate_spare_part_mk1",
                "Fabricate Generation-1 spare component",
                MaterialProcessingStage::Fabrication,
                "structural_alloy", 5.0,
                "fabricated_spare_part_mk1", 4.5,
                1.5e6, 240.0,
                true,
            },
        }};
    }

    void register_recipe(MaterialProcessingRecipe recipe) {
        if (recipe.recipe_id.empty() || recipe.display_name.empty()) {
            throw std::invalid_argument("material processing recipe id/name must not be empty");
        }
        if (recipe.input_material_id.empty() || recipe.output_material_id.empty()) {
            throw std::invalid_argument("material processing recipe material identity must not be empty");
        }
        if (!std::isfinite(recipe.input_kg) || recipe.input_kg <= 0.0) {
            throw std::invalid_argument("material processing recipe input mass must be finite and positive");
        }
        if (!std::isfinite(recipe.output_kg) || recipe.output_kg <= 0.0) {
            throw std::invalid_argument("material processing recipe output mass must be finite and positive");
        }
        // Conservation of mass: a recipe may destroy mass (waste heat, slag,
        // machining losses) but must never create it.
        if (recipe.output_kg > recipe.input_kg + 1e-9) {
            throw std::invalid_argument(
                "material processing recipe would create mass: " + recipe.recipe_id);
        }
        if (!std::isfinite(recipe.energy_required_j) || recipe.energy_required_j < 0.0) {
            throw std::invalid_argument("material processing recipe energy requirement must be finite and non-negative");
        }
        if (!std::isfinite(recipe.time_required_s) || recipe.time_required_s <= 0.0) {
            throw std::invalid_argument("material processing recipe time requirement must be finite and positive");
        }
        if (recipes_.contains(recipe.recipe_id)) {
            throw std::invalid_argument("material processing recipe already registered: " + recipe.recipe_id);
        }
        recipes_.emplace(recipe.recipe_id, std::move(recipe));
    }

    [[nodiscard]] const MaterialProcessingRecipe* find_recipe(const std::string& recipe_id) const noexcept {
        const auto it = recipes_.find(recipe_id);
        return it != recipes_.end() ? &it->second : nullptr;
    }

    // Backs fix_it.hpp's FixItResources::fabrication_available: Fix_It's
    // replacement stage is a real capability only once at least one
    // fabrication recipe is known, rather than a caller-supplied constant.
    [[nodiscard]] bool has_fabrication_capability() const noexcept {
        for (const auto& [id, recipe] : recipes_) {
            if (recipe.stage == MaterialProcessingStage::Fabrication) return true;
        }
        return false;
    }

    [[nodiscard]] const MaterialProcessingJobStatus& status() const noexcept {
        return status_;
    }

    [[nodiscard]] MaterialProcessingStartResult start(
            const std::string& recipe_id,
            const DamageAwareProbeRuntime& runtime,
            const ManipulatorRig& rig) {
        MaterialProcessingStartResult result;
        if (status_.state == MaterialProcessingJobState::Running) {
            result.detail = "a material processing job is already running";
            return result;
        }
        const MaterialProcessingRecipe* recipe = find_recipe(recipe_id);
        if (recipe == nullptr) {
            result.detail = "unknown material processing recipe: " + recipe_id;
            return result;
        }
        if (recipe->requires_deployed_tool && !arm_ready(rig)) {
            result.detail = "deploy a manipulator arm and attach its tool before fabrication";
            return result;
        }

        const auto plan = plan_material_consumption(
            runtime.material_inventory_kg(), recipe->input_material_id, recipe->input_kg);
        if (!plan.can_consume) {
            result.detail = "insufficient stored " + recipe->input_material_id +
                " (" + std::to_string(plan.available_kg) + "/" + std::to_string(recipe->input_kg) + " kg)";
            return result;
        }
        if (runtime.snapshot().stored_energy_j + 1e-6 < recipe->energy_required_j) {
            result.detail = "insufficient stored energy for " + recipe->display_name;
            return result;
        }

        status_ = {};
        status_.state = MaterialProcessingJobState::Running;
        status_.recipe_id = recipe_id;
        status_.stage = recipe->stage;
        status_.detail = "material processing running: " + recipe->display_name;
        result.accepted = true;
        result.detail = status_.detail;
        return result;
    }

    void interrupt(std::string reason) {
        if (status_.state != MaterialProcessingJobState::Running) return;
        status_.state = MaterialProcessingJobState::Interrupted;
        status_.detail = reason.empty() ? "material processing interrupted" : std::move(reason);
    }

    // Fails closed before either debit, exactly as fix_it.hpp's executors
    // do, so interruption can never consume one resource while failing the
    // other half of the same step.
    void advance(DamageAwareProbeRuntime& runtime, double elapsed_s) {
        if (status_.state != MaterialProcessingJobState::Running) {
            throw std::logic_error("material processing job is not running");
        }
        if (!std::isfinite(elapsed_s) || elapsed_s < 0.0) {
            throw std::invalid_argument("material processing elapsed time must be finite and non-negative");
        }
        if (elapsed_s == 0.0) return;

        const MaterialProcessingRecipe& recipe = recipes_.at(status_.recipe_id);
        const double remaining_s = std::max(0.0, recipe.time_required_s - status_.elapsed_s);
        const double applied_s = std::min(elapsed_s, remaining_s);
        if (applied_s <= 0.0) {
            complete(runtime, recipe);
            return;
        }

        const double fraction = applied_s / recipe.time_required_s;
        const double material_delta = recipe.input_kg * fraction;
        const double energy_delta = recipe.energy_required_j * fraction;

        const auto plan = plan_material_consumption(
            runtime.material_inventory_kg(), recipe.input_material_id, material_delta);
        if (!plan.can_consume) {
            interrupt("material processing interrupted: insufficient stored " + recipe.input_material_id);
            return;
        }
        if (runtime.snapshot().stored_energy_j + 1e-6 < energy_delta) {
            interrupt("material processing interrupted: insufficient stored energy");
            return;
        }

        runtime.consume_stored_material_kg(recipe.input_material_id, material_delta);
        runtime.consume_stored_energy_j(energy_delta);
        status_.material_consumed_kg += material_delta;
        status_.energy_consumed_j += energy_delta;
        status_.elapsed_s += applied_s;

        if (status_.elapsed_s + 1e-9 >= recipe.time_required_s) {
            complete(runtime, recipe);
        } else {
            status_.detail = "material processing progressing: " + recipe.display_name;
        }
    }

    // Bridges persisted save data (see save_data.hpp) back into a running
    // system that has already re-registered its canonical recipes. Mirrors
    // the restore_from_snapshot convention used throughout this module
    // (manipulator.hpp, impact_damage.hpp): a direct, validated assignment
    // rather than replaying start()/advance().
    void restore_status(MaterialProcessingJobStatus status) {
        status_ = std::move(status);
    }

private:
    [[nodiscard]] static bool arm_ready(const ManipulatorRig& rig) noexcept {
        for (ManipulatorArmId arm_id : {ManipulatorArmId::Port, ManipulatorArmId::Starboard}) {
            const ManipulatorArmState& arm = rig.arm(arm_id);
            if (arm.is_deployed && !arm.is_deploying && !arm.is_stowing && arm.tool_attached) {
                return true;
            }
        }
        return false;
    }

    void complete(DamageAwareProbeRuntime& runtime, const MaterialProcessingRecipe& recipe) {
        status_.elapsed_s = recipe.time_required_s;
        status_.material_consumed_kg = recipe.input_kg;
        status_.energy_consumed_j = recipe.energy_required_j;
        status_.state = MaterialProcessingJobState::Completed;
        status_.detail = "material processing completed: " + recipe.display_name;
        runtime.add_stored_material_kg(recipe.output_kg, recipe.output_material_id);
    }

    std::map<std::string, MaterialProcessingRecipe> recipes_{};
    MaterialProcessingJobStatus status_{};
};

} // namespace everward::simulation
