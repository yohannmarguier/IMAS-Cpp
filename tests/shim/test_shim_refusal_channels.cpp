// Family 4.4 (docs/SHIM_SUITE_CONVENTION.md S5.4): a correctly refused path
// must be reported on all three channels -- value, refused-path record, and
// status -- not merely survived. `grids_ggd/grid/space/coordinates_type` is
// the map's one `retyped` rule (int array in DD 3.39.0, array of identifier
// structures in DD 4.1.1); no transformation reshapes one into the other, so
// the shim refuses it and the traversal must tolerate that refusal rather
// than abort.
//
// The two reads run as separate calls, converted before oracle, on two
// independent IdsNs::IDS objects (as F4.1's test_shim_structural_rules.cpp
// does), so the oracle read cannot reach convertedIds' own record. This
// program still copies the converted read's record out before running the
// oracle read, matching docs/SHIM_SUITE_CONVENTION.md S5.4's stated ordering:
// the record resets at the start of each root operation, so the same
// ordering keeps this assertion correct if a future revision reads both
// pulses through one shared object instead of two.
#include "ALClasses.h"
#include "shim_refusal_match.h"
#include "shim_rule_table.h"
#include "shim_run_guard.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr const char* kFailureMarker = "REFUSAL-CHANNELS-FAILURE";
constexpr int kExpectedAssertions = 11;
using Equilibrium = IdsNs::IDS::equilibrium;

const ShimRuleTable::Rule* findRule(const char* id) {
  for (const ShimRuleTable::Rule& rule : ShimRuleTable::refusalRules()) {
    if (std::strcmp(rule.id, id) == 0) return &rule;
  }
  return nullptr;
}

std::string hdf5Uri(const char* fixtureRoot, const char* dictionaryDirectory) {
  return std::string("imas:hdf5?path=") + fixtureRoot + "/" + dictionaryDirectory;
}

bool hasSpaceContainer(const Equilibrium& equilibrium) {
  return equilibrium.grids_ggd.extent(0) > 0 && equilibrium.grids_ggd(0).grid.extent(0) > 0 &&
         equilibrium.grids_ggd(0).grid(0).space.extent(0) > 0;
}

void expect(bool condition, const char* detail, int& assertions, int& failures) {
  ++assertions;
  if (!condition) {
    ++failures;
    std::printf("%s: %s\n", kFailureMarker, detail);
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::printf("%s: usage: <fixture-root>\n", kFailureMarker);
    return 1;
  }

  const ShimRuleTable::Rule* rule = findRule("retype-coordinates-type");
  if (rule == nullptr) {
    std::printf("%s: retype-coordinates-type is not present in the refusal-rule table\n",
                kFailureMarker);
    return 1;
  }

  int assertions = 0;
  int failures = 0;

  IdsNs::IDS convertedIds;
  const int convertedOpen = convertedIds.open(hdf5Uri(argv[1], "dd-3.39.0"), OPEN_PULSE);
  const int convertedStatus = convertedOpen == 0 ? convertedIds._equilibrium.get() : convertedOpen;
  const std::vector<IdsNs::SkippedPath> convertedSkippedPaths =
      convertedIds._equilibrium.getSkippedPaths();
  const Equilibrium& converted = convertedIds._equilibrium;

  IdsNs::IDS oracleIds;
  const int oracleOpen = oracleIds.open(hdf5Uri(argv[1], "dd-4.1.1"), OPEN_PULSE);
  const int oracleStatus = oracleOpen == 0 ? oracleIds._equilibrium.get() : oracleOpen;
  const Equilibrium& oracle = oracleIds._equilibrium;

  const bool oracleUsable = oracleOpen == 0 && oracleStatus == 0;
  const bool convertedUsable =
      convertedOpen == 0 && (convertedStatus == 0 || convertedStatus == IdsNs::PARTIAL_READ);
  expect(oracleUsable, "the same-version oracle read did not succeed cleanly", assertions, failures);
  expect(convertedUsable, "the cross-version read did not succeed or report a partial read", assertions,
         failures);
  expect(oracleUsable && convertedUsable && hasSpaceContainer(oracle) && hasSpaceContainer(converted),
         "a read did not reach the grids_ggd/grid/space container the refusal indexes into", assertions,
         failures);

  // Channel 3: status.
  expect(convertedStatus == IdsNs::PARTIAL_READ,
         "the cross-version read of the retyped path did not report a partial outcome", assertions,
         failures);

  // Channel 1: value. Left absent, not converted and not defaulted.
  expect(converted.grids_ggd(0).grid(0).space(0).coordinates_type.extent(0) == 0,
         "the retyped path was not left absent in the returned IDS", assertions, failures);

  // Channel 2: refused-path record. Match the operation, the full DD path
  // (on its tail, docs/SHIM_INTEGRATION_CONTRACT.md S8.1), the reason, and
  // the status band -- three things and no fewer.
  const IdsNs::SkippedPath* refusal = ShimTest::findRefusalByPath(
      convertedSkippedPaths, IdsNs::SkippedPath::Operation::Read, rule->hliPath);
  expect(refusal != nullptr, "the retyped path's refusal is not present in the skipped-path record",
         assertions, failures);
  expect(refusal != nullptr && ShimTest::refusalNamesReason(*refusal, ShimRuleTable::kRetypedRefusalReason),
         "the refusal record's message does not name the retyped-container reason", assertions, failures);
  expect(refusal != nullptr && ShimTest::refusalInBand(*refusal),
         "the refusal record's status is not in the refusal band", assertions, failures);

  // The refusal is absorbed at the field, not by truncating the traversal
  // around it: the containers directly enclosing it survived, a field read
  // after it within the same "space" structure arrived, and a field read
  // later still (equilibrium's root "time", after the whole grids_ggd and
  // time_slice traversal) still agrees with the oracle.
  expect(hasSpaceContainer(converted), "the containers around the refused path did not survive",
         assertions, failures);
  expect(converted.grids_ggd(0).grid(0).space(0).objects_per_dimension.extent(0) > 0,
         "a field read after the refusal within the same structure did not arrive", assertions, failures);

  const std::vector<double> oracleTime(oracle.time.data(), oracle.time.data() + oracle.time.numElements());
  const std::vector<double> convertedTime(converted.time.data(),
                                           converted.time.data() + converted.time.numElements());
  const ShimTest::Verdict laterFieldVerdict =
      ShimTest::Compare(/*oracle=*/ ShimTest::OracleReading(oracleTime),
                        /*converted=*/ ShimTest::ConvertedReading(convertedTime));
  expect(laterFieldVerdict == ShimTest::Verdict::Same,
         "a served field from later in the traversal no longer agrees with the oracle", assertions,
         failures);

  convertedIds.close();
  oracleIds.close();

  ShimTest::assertRanCount(kFailureMarker, "F4.4 refusal-channel assertions", assertions,
                           kExpectedAssertions, failures);
  if (failures != 0) {
    std::printf("%s: rule %s (%s) citation=%s: %d assertion(s) failed\n", kFailureMarker, rule->id,
                ShimRuleTable::kindName(rule->kind), rule->citation, failures);
    return 1;
  }
  return 0;
}
