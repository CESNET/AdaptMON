/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Configuration parser implementation
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#include <amon/conf/Struct.hpp>

namespace amon::conf {

template <typename Out>
static void printSchemaHelper(Out out, const conf::Struct& schema, int indentLevel)
{
    for (const auto& field : schema.fields()) {
        for (int i = 0; i < indentLevel; i++) {
            std::format_to(out, "    ");
        }

        int width = 50 - (indentLevel * 4);
        std::format_to(out, "{:<{}}", field->name(), width);
        std::format_to(out, ": ");
        std::format_to(out, "{}", field->description());
        if (field->isVector()) {
            std::format_to(out, " [list]");
        }
        if (field->isMap()) {
            std::format_to(out, " [dict]");
        }
        if (!field->required()) {
            std::format_to(out, " [optional]");
        }
        std::format_to(out, "\n");

        if (field->hasStructSchema()) {
            printSchemaHelper(out, field->structSchema(), indentLevel + 1);
        }
    }
}

std::string Struct::toString() const
{
    std::string s;
    auto out = std::back_inserter(s);
    printSchemaHelper(out, *this, 0);
    return s;
}

} // namespace amon::conf
