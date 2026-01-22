/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Configuration parser error type
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#include <amon/conf/ConfigError.hpp>
#include <amon/util/numeric.hpp>

#include <iomanip>
#include <sstream>

namespace amon::conf {

std::string ConfigError::toString() const
{
    std::stringstream ss(*m_source);
    std::stringstream out;
    std::string line;

    static constexpr int linesAround = 5;
    const unsigned errLineno = m_line;
    unsigned lineno = 0;
    while (std::getline(ss, line)) {
        if (amon::util::numeric::absDiff(lineno, errLineno) <= linesAround) {
            if (lineno == errLineno) {
                out << " >>> ";
            } else {
                out << "     ";
            }
            out << std::setw(4) << (lineno + 1) << ". ";
            out << line;
            out << '\n';

            if (lineno == errLineno) {
                out << "           ";
                out << std::string(m_col, ' ') << '^' << '\n';
            }
        }
        lineno++;
    }

    out << '\n';
    out << "Config error on line " << (m_line + 1) << ", column " << (m_col + 1) << ": " << what()
        << '\n';

    return out.str();
}

} // namespace amon::conf
