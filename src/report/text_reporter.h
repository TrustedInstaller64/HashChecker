#pragma once

#include "report/ireporter.h"
#include <ostream>
#include <string>

namespace report {

/// Coloured terminal report with per-difference details and summary.
class TextReporter : public IReporter {
public:
    void report(const compare::ComparisonResult& result,
                std::ostream& out) override;

    std::string name() const override { return "text"; }

private:
    static bool is_terminal(const std::ostream& out);
};

} // namespace report
