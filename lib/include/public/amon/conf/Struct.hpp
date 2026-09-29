/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Configuration parser implementation
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#pragma once

#include <amon/Error.hpp>
#include <amon/conf/ConfigError.hpp>
#include <amon/conf/Node.hpp>
#include <amon/conf/Parser.hpp>
#include <amon/conf/Scalar.hpp>
#include <amon/util/numeric.hpp>

#include <algorithm>
#include <cassert>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace amon::conf {

/// @cond
namespace detail {

// Helper to deduce the class type from a member pointer
template <typename T>
struct MemberPointerTraits;

// Specialization for pointer-to-member types
template <typename ClassType, typename MemberType>
struct MemberPointerTraits<MemberType ClassType::*> {
    using Class = ClassType;
    using Member = MemberType;
};

template <typename T>
struct IsStdVector : std::false_type {};

// Specialization for std::vector
template <typename T, typename Alloc>
struct IsStdVector<std::vector<T, Alloc>> : std::true_type {};

template <typename T>
struct IsStdMap : std::false_type {};

// Specialization for std::map
template <typename KeyT, typename ValueT, typename Compare, typename Alloc>
struct IsStdMap<std::map<KeyT, ValueT, Compare, Alloc>> : std::true_type {};

// Primary template: Assumes T does not have the static member.
template <typename T, typename = void>
struct HasSchemaAttribute : std::false_type {};

// Specialization: If T has a static member named `example`.
template <typename T>
struct HasSchemaAttribute<T, std::void_t<decltype(T::SCHEMA)>> : std::true_type {};

template <typename T>
struct IsStdOptional : std::false_type {};

// Specialization for std::vector
template <typename T>
struct IsStdOptional<std::optional<T>> : std::true_type {};

template <typename T>
struct NestedTypeExtract;

template <typename T, typename Alloc>
struct NestedTypeExtract<std::vector<T, Alloc>> {
    using Value = T;
};
template <typename KeyT, typename ValueT, typename Compare, typename Alloc>
struct NestedTypeExtract<std::map<KeyT, ValueT, Compare, Alloc>> {
    using Value = ValueT;
};
template <typename T>
struct NestedTypeExtract<std::optional<T>> {
    using Value = T;
};
template <typename T>
struct NestedTypeExtract {
    using Value = T;
};

} // namespace detail
/// @endcond

/**
 * @brief Describes a field in a struct for configuration parsing.
 * @tparam MemberPtrT_ Pointer-to-member type for the struct field.
 */
template <typename MemberPtrT_>
struct Field {
    /**
     * @brief Alias for the member pointer type.
     */
    using MemberPtrT = MemberPtrT_;
    /**
     * @brief The type of the member pointed to by MemberPtrT.
     */
    using ValueT = detail::MemberPointerTraits<MemberPtrT>::Member;
    /**
     * @brief The nested value type extracted from ValueT.
     */
    using NestedT = detail::NestedTypeExtract<ValueT>::Value;

    /// Pointer to the struct member.
    MemberPtrT field;
    /// Name of the field in the configuration.
    std::string name;
    /// Description of the field.
    std::string description;
    /// Whether the field is required.
    bool required = true;
    /// Default value if the field is not required.
    std::optional<ValueT> defaultValue = std::nullopt;
    /// Optional custom parser for the field.
    std::optional<std::function<NestedT(const Scalar&)>> parser = std::nullopt;
};

class Struct;

/**
 * @brief Abstract base class for field schemas.
 */
class FieldDef {
public:
    /**
     * @brief Get the name of the field.
     * @return The name as a constant reference to std::string.
     */
    virtual const std::string& name() const = 0;

    /**
     * @brief Get the description of the field.
     * @return The description as a constant reference to std::string.
     */
    virtual const std::string& description() const = 0;

    /**
     * @brief Check if the field is required.
     * @return True if the field is required, false otherwise.
     */
    virtual bool required() const = 0;

    /**
     * @brief Parse the value from a Node and store it in the destination struct.
     * @param node The input node containing the value.
     * @param destinationStruct Pointer to the destination struct where the value will be
     * stored.
     */
    virtual void parseValue(const Node& node, void* destinationStruct) const = 0;

    /**
     * @brief Set the default value in the destination struct.
     * @param destinationStruct Pointer to the destination struct where the default value will
     * be set.
     */
    virtual void setDefaultValue(void* destinationStruct) const = 0;

    /**
     * @brief Check if the field is a vector type.
     * @return True if the field is a vector, false otherwise.
     */
    virtual bool isVector() const = 0;

    /**
     * @brief Check if the field is a map type.
     * @return True if the field is a map, false otherwise.
     */
    virtual bool isMap() const = 0;

    /**
     * @brief Check if the field has an associated struct schema.
     * @return True if the field has a struct schema, false otherwise.
     */
    virtual bool hasStructSchema() const = 0;

    /**
     * @brief Get the struct schema associated with the field.
     * @return The struct schema as a constant reference to Struct.
     */
    virtual const Struct& structSchema() const = 0;

    /**
     * @brief Virtual destructor.
     */
    virtual ~FieldDef() = default;
};

/**
 * @brief Concrete implementation of FieldDef for a specific field type.
 * @tparam FieldMemberPtrT Pointer-to-member type for the struct field.
 */
template <typename FieldMemberPtrT>
class ConcreteFieldDef : public FieldDef {
private:
    using ClassT = detail::MemberPointerTraits<FieldMemberPtrT>::Class;
    using ValueT = detail::MemberPointerTraits<FieldMemberPtrT>::Member;
    using NestedT = detail::NestedTypeExtract<ValueT>::Value;

public:
    /**
     * @brief Construct a ConcreteFieldDef from a Field.
     * @param field The field metadata to initialize from.
     * @throws amon::Error if the field is required and has a default value set.
     */
    ConcreteFieldDef(const Field<FieldMemberPtrT>& field)
        : m_fieldPtr(field.field)
        , m_name(field.name)
        , m_description(field.description)
        , m_required(field.required)
        , m_defaultValue(field.defaultValue)
        , m_parser(field.parser)
    {
        if (m_name.empty()) {
            throw amon::Error("field definition is missing name");
        }

        if (!m_fieldPtr) {
            throw amon::Error("field \"{}\" is missing fieldPtr", m_name);
        }

        if (m_description.empty()) {
            throw amon::Error("field \"{}\" is missing description", m_name);
        }

        if (m_required && m_defaultValue) {
            throw amon::Error(
                "field \"{}\" is marked as required but has default value set",
                m_name);
        }

        if (!m_required && !m_defaultValue) {
            if constexpr (std::is_default_constructible_v<ValueT>) {
                m_defaultValue = ValueT {};
            } else {
                throw amon::Error(
                    "field \"{}\" is marked as optional but is missing default value",
                    m_name);
            }
        }
    }

    /**
     * @brief Get the name of the field.
     * @return The field name as a string reference.
     */
    const std::string& name() const override { return m_name; }

    /**
     * @brief Get the description of the field.
     * @return The field description as a string reference.
     */
    const std::string& description() const override { return m_description; }

    /**
     * @brief Check if the field is required.
     * @return True if the field is required, false otherwise.
     */
    bool required() const override { return m_required; }

    /**
     * @brief Parse the value from the node and assign it to the struct field.
     * @param node The input node to parse the value from.
     * @param destinationStruct Pointer to the struct where the value will be assigned.
     */
    void parseValue(const Node& node, void* destinationStruct) const override
    {
        accessMemberInStruct(destinationStruct) = parseHelper<ValueT>(node);
    }

    /**
     * @brief Set the default value for the struct field.
     * @param destinationStruct Pointer to the struct where the default value will be set.
     * @note Asserts that a default value is present.
     */
    void setDefaultValue(void* destinationStruct) const override
    {
        if (!m_defaultValue) {
            throw std::logic_error("field has no default value");
        }
        accessMemberInStruct(destinationStruct) = m_defaultValue.value();
    }

    /**
     * @brief Check if the field type is a std::map.
     * @return True if the field is a map, false otherwise.
     */
    bool isMap() const override { return static_cast<bool>(detail::IsStdMap<ValueT>::value); }

    /**
     * @brief Check if the field type is a std::vector.
     * @return True if the field is a vector, false otherwise.
     */
    bool isVector() const override { return static_cast<bool>(detail::IsStdVector<ValueT>::value); }

    /**
     * @brief Check if the field type has a struct schema attribute.
     * @return True if the field type has a schema attribute, false otherwise.
     */
    bool hasStructSchema() const override
    {
        return static_cast<bool>(detail::HasSchemaAttribute<NestedT>::value);
    }

    /**
     * @brief Get the struct schema for the field type.
     * @return Reference to the struct schema.
     * @throws std::logic_error if the field type does not have a schema attribute.
     */
    const Struct& structSchema() const override
    {
        if constexpr (detail::HasSchemaAttribute<NestedT>::value) {
            return NestedT::SCHEMA;
        } else {
            throw std::logic_error("not a struct");
        }
    }

private:
    FieldMemberPtrT m_fieldPtr;
    std::string m_name;
    std::string m_description;
    bool m_required;
    std::optional<ValueT> m_defaultValue;
    std::optional<std::function<NestedT(const Scalar&)>> m_parser;

    /**
     * @brief Access the member in the struct instance.
     */
    ValueT& accessMemberInStruct(void* instance) const
    {
        return (*reinterpret_cast<ClassT*>(instance)).*m_fieldPtr;
    }

    /**
     * @brief Helper to recursively parse the node into the appropriate type.
     * Handles maps, vectors, nested structs, optionals, and scalars.
     */
    template <typename ResultT>
    ResultT parseHelper(const Node& node) const
    {
        if constexpr (detail::IsStdMap<ResultT>::value) {
            if (!node.isDict()) {
                throw ConfigError(
                    node,
                    "expected node to be a key-value dictionary, not {}",
                    node.typeDescriptionHuman());
            }

            ResultT result;
            for (const auto& [key, valueNode] : node.dict()) {
                auto value = parseHelper<typename ValueT::mapped_type>(valueNode);
                result.emplace(key, std::move(value));
            }
            return result;

        } else if constexpr (detail::IsStdVector<ResultT>::value) {
            if (!node.isList()) {
                throw ConfigError(
                    node,
                    "expected node to be a list of values, not {}",
                    node.typeDescriptionHuman());
            }

            ResultT result;
            for (const auto& valueNode : node.list()) {
                auto value = parseHelper<typename ResultT::value_type>(valueNode);
                result.emplace_back(std::move(value));
            }
            return result;

        } else if constexpr (detail::HasSchemaAttribute<ResultT>::value) {
            ResultT result;
            ResultT::SCHEMA.parseFields(node, &result);
            return result;

        } else if constexpr (detail::IsStdOptional<ResultT>::value) {
            return !node.isDefined() ? std::nullopt
                                     : ResultT {parseHelper<typename ResultT::value_type>(node)};

        } else if constexpr (std::is_same_v<ResultT, Node>) {
            return node;

        } else {
            try {
                return m_parser.has_value() ? (*m_parser)(node.scalar())
                                            : Parser<ResultT>::parse(node.scalar());
            } catch (const std::exception& ex) {
                throw ConfigError(node, "{}", ex.what());
            }
        }
    }
};

/**
 * @brief Represents the schema of a struct for configuration parsing.
 * Holds a list of field schemas and provides parsing logic.
 */
class Struct {
public:
    /**
     * @brief Construct a Struct from a list of fields.
     * @tparam ...Ts Field types.
     * @param ...fields Field schemas.
     */
    template <typename... Ts>
    Struct(Field<Ts>&&... fields)
    {
        m_fields = {[](auto&& field) {
            return std::shared_ptr<FieldDef>(new ConcreteFieldDef(field));
        }(std::move(fields))...};

        for (const auto& field : m_fields) {
            const auto& name = field->name();
            const auto count = std::ranges::count_if(m_fields, [&](const auto& field) {
                return field->name() == name;
            });
            if (count > 1) {
                throw amon::Error("multiple fields defined with the same name '{}'", name);
            }
        }
    }

    /**
     * @brief Get the list of field schemas.
     * @return const reference to a vector of shared pointers to FieldDef objects.
     */
    const std::vector<std::shared_ptr<FieldDef>>& fields() const { return m_fields; }

    /**
     * @brief Convert the schema of a struct type to string.
     * @return the string with nicely formatted representation of the structure.
     */
    std::string toString() const;

private:
    friend class Node;
    template <typename T>
    friend class ConcreteFieldDef;

    std::vector<std::shared_ptr<FieldDef>> m_fields;

    /**
     * @brief Parse all fields from the node into the destination struct.
     * Throws if required fields are missing or extra keys are present.
     */
    void parseFields(const Node& node, void* destinationStruct) const
    {
        if (!node.isDict()) {
            throw ConfigError(
                node,
                "expected node to be a key-value dictionary, not {}",
                node.typeDescriptionHuman());
        }

        for (const auto& field : m_fields) {
            auto it = node.dict().find(field->name());
            if (it == node.dict().end()) {
                if (!field->required()) {
                    field->setDefaultValue(destinationStruct);
                } else {
                    throw ConfigError(
                        node,
                        "dictionary is missing a required key \"{}\"",
                        field->name());
                }
            } else {
                field->parseValue(it->second, destinationStruct);
            }
        }

        for (const auto& [key, _] : node.dict()) {
            if (!std::ranges::any_of(m_fields, [&](const auto& field) {
                    return field->name() == key;
                })) {
                throw ConfigError(node, "unexpected key \"{}\" in dictionary", key);
            }
        }
    }
};

} // namespace amon::conf
