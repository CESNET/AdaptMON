/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Configuration parser - config node representation
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#include <amon/conf/Node.hpp>

#include <utility>

namespace amon::conf {

std::string_view Node::typeDescriptionHuman() const
{
    if (isDict()) {
        return "a key-value dictionary";
    }
    if (isList()) {
        return "a list of values";
    }
    if (isScalar()) {
        return "a scalar value";
    }
    if (isDefined()) {
        return "a null value";
    }
    std::unreachable();
}

} // namespace amon::conf
