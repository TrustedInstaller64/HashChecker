#pragma once

#include "compare/comparison_result.h"
#include <ostream>
#include <string>

namespace report {

/// Abstract interface for report output formats.
class IReporter {
public:
    virtual ~IReporter() = default;

    /// Write the comparison result to the given output stream.
    virtual void report(const compare::ComparisonResult& result,
                        std::ostream& out) = 0;

    /// Human-readable name of this reporter (e.g. "text", "json").
    virtual std::string name() const = 0;
};

} // namespace report
