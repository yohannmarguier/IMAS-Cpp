// Family 4.1 (docs/SHIM_SUITE_CONVENTION.md S5.4): structural conversion
// rules read exclusively through the public C++ HLI.  The DD 4.1.1 fixture
// is the same-version oracle; DD 3.39.0 is read through the shim.
#include "ALClasses.h"
#include "shim_rule_check.h"
#include "shim_rule_table.h"

#include <array>
#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr const char* kFailureMarker = "STRUCTURAL-FAILURE";
using Equilibrium = IdsNs::IDS::equilibrium;

std::string hdf5Uri(const char* fixtureRoot, const char* dictionaryDirectory) {
  return std::string("imas:hdf5?path=") + fixtureRoot + "/" + dictionaryDirectory;
}

std::vector<double> scalarReading(double value) {
  return value == EMPTY_DOUBLE ? std::vector<double>{} : std::vector<double>{value};
}

template <int Rank>
std::vector<double> arrayReading(const IMASArray<double, Rank>& values) {
  const std::size_t count = static_cast<std::size_t>(values.numElements());
  return count == 0 ? std::vector<double>{} : std::vector<double>(values.data(), values.data() + count);
}

ShimTest::Verdict compareReadings(const std::vector<double>& oracle,
                                  const std::vector<double>& converted) {
  return ShimTest::Compare(/*oracle=*/ ShimTest::OracleReading(oracle),
                           /*converted=*/ ShimTest::ConvertedReading(converted));
}

ShimTest::Verdict scalarVerdict(double oracle, double converted) {
  return compareReadings(scalarReading(oracle), scalarReading(converted));
}

template <int Rank>
ShimTest::Verdict arrayVerdict(const IMASArray<double, Rank>& oracle,
                               const IMASArray<double, Rank>& converted) {
  return compareReadings(arrayReading(oracle), arrayReading(converted));
}

ShimTest::Verdict firstNonAgreeing(const ShimRuleTable::Rule& rule, ShimTest::Verdict first,
                                   ShimTest::Verdict second) {
  return first == ShimRuleTable::expectedVerdict(rule.kind) ? second : first;
}

bool hasRequiredContainers(const Equilibrium& equilibrium) {
  if (equilibrium.time.extent(0) == 0 || equilibrium.time_slice.extent(0) == 0) return false;
  const auto& slice = equilibrium.time_slice(0);
  return slice.constraints.b_field_pol_probe.extent(0) > 0 &&
         slice.constraints.mse_polarization_angle.extent(0) > 0 &&
         slice.constraints.iron_core_segment.extent(0) > 0 && slice.boundary.gap.extent(0) > 0 &&
         slice.profiles_2d.extent(0) > 0 && slice.constraints.j_phi.extent(0) > 0 &&
         slice.ggd.extent(0) > 0 && slice.ggd(0).j_phi.extent(0) > 0 &&
         slice.ggd(0).b_field_phi.extent(0) > 0;
}

using RuleEvaluator = ShimTest::Verdict (*)(const ShimRuleTable::Rule&, const Equilibrium&,
                                             const Equilibrium&);

struct StructuralCheck {
  const char* id;
  RuleEvaluator evaluate;
};

ShimTest::Verdict evaluateVacuumR0(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                   const Equilibrium& converted) {
  return scalarVerdict(oracle.vacuum_toroidal_field.r0, converted.vacuum_toroidal_field.r0);
}

ShimTest::Verdict evaluateTime(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                               const Equilibrium& converted) {
  return arrayVerdict(oracle.time, converted.time);
}

ShimTest::Verdict evaluateBetaPol(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                  const Equilibrium& converted) {
  return scalarVerdict(oracle.time_slice(0).global_quantities.beta_pol,
                       converted.time_slice(0).global_quantities.beta_pol);
}

ShimTest::Verdict evaluateBetaTorNorm(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                      const Equilibrium& converted) {
  return scalarVerdict(oracle.time_slice(0).global_quantities.beta_tor_norm,
                       converted.time_slice(0).global_quantities.beta_tor_norm);
}

ShimTest::Verdict evaluateBpolProbe(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                    const Equilibrium& converted) {
  return scalarVerdict(oracle.time_slice(0).constraints.b_field_pol_probe(0).measured,
                       converted.time_slice(0).constraints.b_field_pol_probe(0).measured);
}

ShimTest::Verdict evaluateMsePolarization(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                          const Equilibrium& converted) {
  return scalarVerdict(oracle.time_slice(0).constraints.mse_polarization_angle(0).measured,
                       converted.time_slice(0).constraints.mse_polarization_angle(0).measured);
}

ShimTest::Verdict evaluateMagnetizationR(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                         const Equilibrium& converted) {
  return scalarVerdict(
      oracle.time_slice(0).constraints.iron_core_segment(0).magnetization_r.measured,
      converted.time_slice(0).constraints.iron_core_segment(0).magnetization_r.measured);
}

ShimTest::Verdict evaluateMagnetizationZ(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                         const Equilibrium& converted) {
  return scalarVerdict(
      oracle.time_slice(0).constraints.iron_core_segment(0).magnetization_z.measured,
      converted.time_slice(0).constraints.iron_core_segment(0).magnetization_z.measured);
}

ShimTest::Verdict evaluateClosestWallPoint(const ShimRuleTable::Rule& rule, const Equilibrium& oracle,
                                           const Equilibrium& converted) {
  return firstNonAgreeing(rule,
      scalarVerdict(oracle.time_slice(0).boundary.closest_wall_point.r,
                    converted.time_slice(0).boundary.closest_wall_point.r),
      scalarVerdict(oracle.time_slice(0).boundary.closest_wall_point.z,
                    converted.time_slice(0).boundary.closest_wall_point.z));
}

ShimTest::Verdict evaluateDrDzZeroPoint(const ShimRuleTable::Rule& rule, const Equilibrium& oracle,
                                        const Equilibrium& converted) {
  return firstNonAgreeing(rule,
      scalarVerdict(oracle.time_slice(0).boundary.dr_dz_zero_point.r,
                    converted.time_slice(0).boundary.dr_dz_zero_point.r),
      scalarVerdict(oracle.time_slice(0).boundary.dr_dz_zero_point.z,
                    converted.time_slice(0).boundary.dr_dz_zero_point.z));
}

ShimTest::Verdict evaluateGap(const ShimRuleTable::Rule& rule, const Equilibrium& oracle,
                              const Equilibrium& converted) {
  return firstNonAgreeing(rule,
      scalarVerdict(oracle.time_slice(0).boundary.gap(0).r,
                    converted.time_slice(0).boundary.gap(0).r),
      scalarVerdict(oracle.time_slice(0).boundary.gap(0).z,
                    converted.time_slice(0).boundary.gap(0).z));
}

ShimTest::Verdict evaluateP2dBFieldR(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                     const Equilibrium& converted) {
  return arrayVerdict(oracle.time_slice(0).profiles_2d(0).b_field_r,
                      converted.time_slice(0).profiles_2d(0).b_field_r);
}

ShimTest::Verdict evaluateP2dBFieldZ(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                     const Equilibrium& converted) {
  return arrayVerdict(oracle.time_slice(0).profiles_2d(0).b_field_z,
                      converted.time_slice(0).profiles_2d(0).b_field_z);
}

ShimTest::Verdict evaluateP2dBFieldPhi(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                       const Equilibrium& converted) {
  return arrayVerdict(oracle.time_slice(0).profiles_2d(0).b_field_phi,
                      converted.time_slice(0).profiles_2d(0).b_field_phi);
}

ShimTest::Verdict evaluateAxisBFieldPhi(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                        const Equilibrium& converted) {
  return scalarVerdict(oracle.time_slice(0).global_quantities.magnetic_axis.b_field_phi,
                       converted.time_slice(0).global_quantities.magnetic_axis.b_field_phi);
}

ShimTest::Verdict evaluateP1dBFieldAverage(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                           const Equilibrium& converted) {
  return arrayVerdict(oracle.time_slice(0).profiles_1d.b_field_average,
                      converted.time_slice(0).profiles_1d.b_field_average);
}

ShimTest::Verdict evaluateP1dBFieldMax(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                       const Equilibrium& converted) {
  return arrayVerdict(oracle.time_slice(0).profiles_1d.b_field_max,
                      converted.time_slice(0).profiles_1d.b_field_max);
}

ShimTest::Verdict evaluateP1dBFieldMin(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                       const Equilibrium& converted) {
  return arrayVerdict(oracle.time_slice(0).profiles_1d.b_field_min,
                      converted.time_slice(0).profiles_1d.b_field_min);
}

ShimTest::Verdict evaluateEnergyMhd(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                    const Equilibrium& converted) {
  return scalarVerdict(oracle.time_slice(0).global_quantities.energy_mhd,
                       converted.time_slice(0).global_quantities.energy_mhd);
}

ShimTest::Verdict evaluateConstraintsJPhi(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                          const Equilibrium& converted) {
  return scalarVerdict(oracle.time_slice(0).constraints.j_phi(0).measured,
                       converted.time_slice(0).constraints.j_phi(0).measured);
}

ShimTest::Verdict evaluateGgdJPhi(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                  const Equilibrium& converted) {
  return arrayVerdict(oracle.time_slice(0).ggd(0).j_phi(0).values,
                      converted.time_slice(0).ggd(0).j_phi(0).values);
}

ShimTest::Verdict evaluateGgdBFieldPhi(const ShimRuleTable::Rule&, const Equilibrium& oracle,
                                       const Equilibrium& converted) {
  return arrayVerdict(oracle.time_slice(0).ggd(0).b_field_phi(0).values,
                      converted.time_slice(0).ggd(0).b_field_phi(0).values);
}

ShimTest::Verdict evaluateSplitPsiAxis(const ShimRuleTable::Rule& rule, const Equilibrium& oracle,
                                       const Equilibrium& converted) {
  return firstNonAgreeing(rule,
      scalarVerdict(oracle.time_slice(0).global_quantities.psi_axis,
                    converted.time_slice(0).global_quantities.psi_axis),
      scalarVerdict(oracle.time_slice(0).global_quantities.psi_magnetic_axis,
                    converted.time_slice(0).global_quantities.psi_magnetic_axis));
}

constexpr std::array<StructuralCheck, 23> kStructuralChecks{{
    {"identical-vacuum-r0", evaluateVacuumR0}, {"identical-time", evaluateTime},
    {"identical-beta-pol", evaluateBetaPol}, {"rename-beta-normal", evaluateBetaTorNorm},
    {"rename-bpol-probe", evaluateBpolProbe},
    {"rename-mse-polarisation-angle", evaluateMsePolarization},
    {"rename-magnetisation-r", evaluateMagnetizationR},
    {"rename-magnetisation-z", evaluateMagnetizationZ},
    {"move-closest-wall-point", evaluateClosestWallPoint},
    {"move-dr-dz-zero-point", evaluateDrDzZeroPoint}, {"move-gap", evaluateGap},
    {"fold-p2d-br", evaluateP2dBFieldR}, {"fold-p2d-bz", evaluateP2dBFieldZ},
    {"fold-p2d-bphi", evaluateP2dBFieldPhi}, {"fold-axis-bphi", evaluateAxisBFieldPhi},
    {"fold-p1d-baverage", evaluateP1dBFieldAverage},
    {"fold-p1d-bmax", evaluateP1dBFieldMax}, {"fold-p1d-bmin", evaluateP1dBFieldMin},
    {"fold-energy-mhd", evaluateEnergyMhd}, {"fold-constraints-j", evaluateConstraintsJPhi},
    {"fold-ggd-j", evaluateGgdJPhi}, {"fold-ggd-bfield", evaluateGgdBFieldPhi},
    {"split-psi-axis", evaluateSplitPsiAxis},
}};

}  // namespace

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::printf("%s: usage: <fixture-root>\n", kFailureMarker);
    return 1;
  }

  const auto& rules = ShimRuleTable::structuralRules();
  ShimTest::RuleChecker checker(kFailureMarker, rules.data(), rules.size());
  if (kStructuralChecks.size() != rules.size()) {
    checker.fail("the structural check loop and rule table have different sizes");
  }

  IdsNs::IDS convertedIds;
  IdsNs::IDS oracleIds;
  const int convertedOpen = convertedIds.open(hdf5Uri(argv[1], "dd-3.39.0"), OPEN_PULSE);
  const int convertedStatus = convertedOpen == 0 ? convertedIds._equilibrium.get() : convertedOpen;
  const int oracleOpen = oracleIds.open(hdf5Uri(argv[1], "dd-4.1.1"), OPEN_PULSE);
  const int oracleStatus = oracleOpen == 0 ? oracleIds._equilibrium.get() : oracleOpen;

  const bool oracleUsable = oracleOpen == 0 && oracleStatus == 0;
  const bool convertedUsable =
      convertedOpen == 0 && (convertedStatus == 0 || convertedStatus == IdsNs::PARTIAL_READ);
  if (!oracleUsable) {
    checker.fail("the same-version oracle read did not succeed cleanly");
  }
  if (!convertedUsable) {
    checker.fail("the cross-version read did not succeed or report a partial read");
  }
  if (oracleUsable && convertedUsable &&
      (!hasRequiredContainers(oracleIds._equilibrium) ||
       !hasRequiredContainers(convertedIds._equilibrium))) {
    checker.fail("a read did not reach every container the structural rules index into");
  }

  if (checker.failures() == 0) {
    for (const StructuralCheck& check : kStructuralChecks) {
      for (const ShimRuleTable::Rule& rule : rules) {
        if (std::string(rule.id) == check.id) {
          checker.check(check.id, check.evaluate(rule, oracleIds._equilibrium,
                                                  convertedIds._equilibrium));
          break;
        }
      }
    }
  }

  convertedIds.close();
  oracleIds.close();
  checker.assertEveryRuleChecked();
  if (checker.failures() != 0) {
    std::printf("%s: %d structural rule assertion(s) failed\n", kFailureMarker,
                checker.failures());
    return 1;
  }
  return 0;
}
