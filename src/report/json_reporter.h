#pragma once

#include "report/ireporter.h"
#include <ostream>
#include <string>

namespace report {

/// Machine-readable JSON report.
class JsonReporter : public IReporter {
public:
    void report(const compare::ComparisonResult& result,
                std::ostream& out) override;

private:
    static std::string escape(const std::string& s);
};

} // namespace report
