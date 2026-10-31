#pragma once

#include "report/ireporter.h"
#include <ostream>
#include <string>

namespace report {

/// Minimal reporter: prints only a one-line summary.
class QuietReporter : public IReporter {
public:
    void report(const compare::ComparisonResult& result,
                std::ostream& out) override;
};

} // namespace report
