/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Configuration parser - config node representation
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#pragma once

#include <amon/Error.hpp>
#include <amon/conf/Scalar.hpp>

#include <cassert>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace amon::conf {

/**
 * @brief Represents a node in the configuration tree.
 *
 * A Node can be a scalar value, a list, a dictionary, or undefined (monostate).
 * It stores its value, position in the source file, and a reference to the source file string.
 */
class Node {
public:
    /**
     * @brief Variant type to hold possible node values.
     *
     * Value can be one of the following types:
     *   - std::monostate: Represents an empty or uninitialized value.
     *   - std::shared_ptr<Scalar>: Holds a scalar value.
     *   - std::vector<Node>: Holds a list of child nodes.
     *   - std::map<std::string, Node>: Holds a mapping from string keys to nodes.
     */
    using Value = std::variant<
        std::monostate,
        std::shared_ptr<Scalar>,
        std::vector<Node>,
        std::map<std::string, Node>>;

    /**
     * @brief Construct a Node with a value, line, column, and source file reference.
     * @param value The value to assign to the node.
     * @param line The line number in the source.
     * @param col The column number in the source.
     * @param source Shared pointer to the source string.
     */
    Node(Value value, unsigned line, unsigned col, std::shared_ptr<std::string> source)
        : m_value(std::move(value))
        , m_line(line)
        , m_col(col)
        , m_source(std::move(source))
    {
    }

    /**
     * @brief Parse this Node into a struct of type StructT.
     * @tparam StructT The struct type to parse into.
     * @return An instance of StructT populated from this Node.
     */
    template <typename StructT>
    StructT parseInto();

    /**
     * @brief Checks if the node holds a dictionary (map) value.
     * @return True if the node is a dictionary, false otherwise.
     */
    bool isDict() const { return std::holds_alternative<std::map<std::string, Node>>(m_value); }

    /**
     * @brief Checks if the node holds a list (vector) value.
     * @return True if the node is a list, false otherwise.
     */
    bool isList() const { return std::holds_alternative<std::vector<Node>>(m_value); }

    /**
     * @brief Checks if the node holds a scalar (string) value.
     * @return True if the node is a scalar, false otherwise.
     */
    bool isScalar() const { return std::holds_alternative<std::shared_ptr<Scalar>>(m_value); }

    /**
     * @brief Checks if the node is defined (not a monostate).
     * @return True if the node is defined, false otherwise.
     */
    bool isDefined() const { return !std::holds_alternative<std::monostate>(m_value); }

    /**
     * @brief Gets the dictionary value held by the node.
     * @return Const reference to the dictionary.
     * @throws std::bad_variant_access if the node does not hold a dictionary.
     */
    const std::map<std::string, Node>& dict() const
    {
        return std::get<std::map<std::string, Node>>(m_value);
    }

    /**
     * @brief Gets the list value held by the node.
     * @return Const reference to the list.
     * @throws std::bad_variant_access if the node does not hold a list.
     */
    const std::vector<Node>& list() const { return std::get<std::vector<Node>>(m_value); }

    /**
     * @brief Gets the scalar value held by the node.
     * @return Const reference to the scalar string.
     * @throws std::bad_variant_access if the node does not hold a scalar.
     */
    const Scalar& scalar() const { return *std::get<std::shared_ptr<Scalar>>(m_value); }

    /**
     * @brief Gets the line number where the node was parsed from.
     * @return The line number.
     */
    unsigned line() const { return m_line; }

    /**
     * @brief Gets the column number where the node was parsed from.
     * @return The column number.
     */
    unsigned col() const { return m_col; }

    /**
     * @brief Gets the source string from which the node was parsed.
     * @return Const reference to the source string.
     */
    const std::shared_ptr<std::string>& source() const { return m_source; }

    /**
     * @brief Returns a human-readable description of the type.
     *
     * This function inspects the type and returns a string view describing
     * whether it is a dictionary, list, scalar, or null value.
     *
     * @return std::string_view A description of the type.
     */
    std::string_view typeDescriptionHuman() const;

private:
    Value m_value;
    unsigned m_line = 0;
    unsigned m_col = 0;
    std::shared_ptr<std::string> m_source;
};

/**
 * @brief Parse this Node into a struct of type StructT.
 * @tparam StructT The struct type to parse into.
 * @return An instance of StructT populated from this Node.
 */
template <typename StructT>
StructT Node::parseInto()
{
    StructT result;
    StructT::SCHEMA.parseFields(*this, reinterpret_cast<void*>(&result));
    return result;
}

} // namespace amon::conf
