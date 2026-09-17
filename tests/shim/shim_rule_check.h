// The one shared rule-loop judgement for fixture-driven shim programs.
#pragma once

#include "shim_rule_table.h"
#include "shim_run_guard.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace ShimTest {

inline const char* verdictName(Verdict verdict) {
  switch (verdict) {
    case Verdict::Absent: return "absent";
    case Verdict::OnlyOracle: return "only-oracle";
    case Verdict::OnlyConverted: return "only-converted";
    case Verdict::Same: return "same";
    case Verdict::NoFlip: return "no-flip";
    case Verdict::Diff: return "different";
    case Verdict::Shape: return "shape";
  }
  return "unknown";
}

class RuleChecker {
 public:
  RuleChecker(const char* marker, const ShimRuleTable::Rule* rules, std::size_t ruleCount)
      : marker_(marker), rules_(rules), ruleCount_(ruleCount), checkedRules_(ruleCount, false) {}

  void check(const char* id, Verdict actual) {
    const std::size_t index = findIndex(id);
    if (index == ruleCount_) {
      fail("rule id is not present in the rule table");
      return;
    }
    if (checkedRules_[index]) {
      fail("rule id was checked more than once");
      return;
    }
    checkedRules_[index] = true;
    ++checked_;
    const ShimRuleTable::Rule* rule = &rules_[index];
    const Verdict expected = ShimRuleTable::expectedVerdict(rule->kind);
    if (actual == expected) return;
    ++failures_;
    std::printf("%s: rule %s (%s) citation=%s verdict=%s expected=%s\n", marker_, rule->id,
                ShimRuleTable::kindName(rule->kind), rule->citation, verdictName(actual),
                verdictName(expected));
  }

  void fail(const char* detail) {
    ++failures_;
    std::printf("%s: %s\n", marker_, detail);
  }

  void expect(bool condition, const char* detail) {
    ++assertions_;
    if (!condition) fail(detail);
  }

  void assertPreconditionCount(int expected) {
    assertRanCount(marker_, "structural preconditions checked", assertions_, expected, failures_);
  }

  void assertEveryRuleChecked() {
    assertRanCount(marker_, "rule table entries checked", checked_, static_cast<int>(ruleCount_),
                   failures_);
  }

  int failures() const { return failures_; }

 private:
  std::size_t findIndex(const char* id) const {
    for (std::size_t i = 0; i < ruleCount_; ++i) {
      if (std::strcmp(rules_[i].id, id) == 0) return i;
    }
    return ruleCount_;
  }

  const char* marker_;
  const ShimRuleTable::Rule* rules_;
  std::size_t ruleCount_;
  std::vector<bool> checkedRules_;
  int checked_ = 0;
  int assertions_ = 0;
  int failures_ = 0;
};

}  // namespace ShimTest
