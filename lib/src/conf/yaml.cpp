/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Configuration parser implementation - yaml specialization
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#include <amon/Error.hpp>
#include <amon/conf/ConfigError.hpp>
#include <amon/conf/Scalar.hpp>
#include <amon/conf/yaml.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <yaml-cpp/yaml.h>
#pragma GCC diagnostic pop

namespace amon::conf::yaml {

static Node nodeFromYamlNode(const std::shared_ptr<std::string>& sourceYaml, const YAML::Node& node)
{
    Node::Value value;

    if (!node.IsDefined()) {
        value = {};

    } else if (node.IsMap()) {
        std::map<std::string, Node> map {};

        for (const auto& mapNode : node) {
            if (map.contains(mapNode.first.Scalar())) {
                throw ConfigError(
                    static_cast<unsigned>(node.Mark().line),
                    static_cast<unsigned>(node.Mark().column),
                    sourceYaml,
                    "key \"{}\" defined multiple times in dictionary",
                    mapNode.first.Scalar());
            }

            map.emplace(mapNode.first.Scalar(), nodeFromYamlNode(sourceYaml, mapNode.second));
        }
        value = std::move(map);

    } else if (node.IsSequence()) {
        std::vector<Node> sequence {};
        for (const auto& seqNode : node) {
            sequence.emplace_back(nodeFromYamlNode(sourceYaml, seqNode));
        }
        value = std::move(sequence);

    } else {
        value = std::make_shared<Scalar>(node.Scalar());
    }

    return Node {
        std::move(value),
        static_cast<unsigned>(node.Mark().line),
        static_cast<unsigned>(node.Mark().column),
        sourceYaml};
}

template <>
Node parseInto(const std::string& yamlString)
{
    auto sourceYaml = std::make_shared<std::string>(yamlString);
    YAML::Node doc;
    try {
        doc = YAML::Load(yamlString);
    } catch (const YAML::Exception& ex) {
        throw ConfigError(
            static_cast<unsigned>(ex.mark.line),
            static_cast<unsigned>(ex.mark.column),
            sourceYaml,
            "invalid syntax: {}",
            ex.msg);
    }
    return nodeFromYamlNode(sourceYaml, doc);
}

Scalar::Scalar(std::string value)
    : m_value(std::move(value))
{
}

std::string Scalar::asString() const
{
    return m_value;
}

bool Scalar::asBool() const
{
    try {
        return YAML::Load(m_value).as<bool>();
    } catch (const YAML::Exception& ex) {
        throw amon::Error("{}", ex.msg);
    }
}

uint64_t Scalar::asUInt() const
{
    try {
        return YAML::Load(m_value).as<uint64_t>();
    } catch (const YAML::Exception& ex) {
        throw amon::Error("{}", ex.msg);
    }
}

int64_t Scalar::asInt() const
{
    try {
        return YAML::Load(m_value).as<int64_t>();
    } catch (const YAML::Exception& ex) {
        throw amon::Error("{}", ex.msg);
    }
}

double Scalar::asDouble() const
{
    try {
        return YAML::Load(m_value).as<double>();
    } catch (const YAML::Exception& ex) {
        throw amon::Error("{}", ex.msg);
    }
}

} // namespace amon::conf::yaml
