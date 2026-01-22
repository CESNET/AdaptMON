/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Configuration parser implementation - yaml specialization
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#include <amon-internal/Test.hpp>
#include <amon/Error.hpp>
#include <amon/conf/ConfigError.hpp>
#include <amon/conf/Scalar.hpp>
#include <amon/conf/Struct.hpp>
#include <amon/conf/yaml.hpp>
#include <amon/util/numeric.hpp>

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

#ifdef AMON_ENABLE_TESTS

/// @cond
namespace {

struct SimpleScalarDefault {
    int a;
    std::string b;
    static const amon::conf::Struct SCHEMA;
};

const amon::conf::Struct SimpleScalarDefault::SCHEMA {
    amon::conf::Field {
        .field = &SimpleScalarDefault::a,
        .name = "a",
        .description = "an int",
        .required = true},
    amon::conf::Field {
        .field = &SimpleScalarDefault::b,
        .name = "b",
        .description = "a string",
        .required = false,
        .defaultValue = "hello"},
};

TEST_CASE("YamlConf simple scalar and default")
{
    const std::string yaml = "a: 42\n";
    auto s = amon::conf::yaml::parseInto<SimpleScalarDefault>(yaml);
    CHECK(s.a == 42);
    CHECK(s.b == "hello");
}

struct SimpleMissingRequired {
    int a;
    static const amon::conf::Struct SCHEMA;
};

const amon::conf::Struct SimpleMissingRequired::SCHEMA {
    amon::conf::Field {
        .field = &SimpleMissingRequired::a,
        .name = "a",
        .description = "an int",
        .required = true},
};

TEST_CASE("YamlConf missing required field throws")
{
    const std::string yaml = "b: 1\n";
    CHECK_THROWS_AS(
        amon::conf::yaml::parseInto<SimpleMissingRequired>(yaml),
        amon::conf::ConfigError);
}

struct SimpleExtraKey {
    int a;
    static const amon::conf::Struct SCHEMA;
};

const amon::conf::Struct SimpleExtraKey::SCHEMA {
    amon::conf::Field {
        .field = &SimpleExtraKey::a,
        .name = "a",
        .description = "an int",
        .required = true},
};

TEST_CASE("YamlConf extra key throws")
{
    const std::string yaml = "a: 1\nc: 2\n";
    CHECK_THROWS_AS(amon::conf::yaml::parseInto<SimpleExtraKey>(yaml), amon::conf::ConfigError);
}

struct Inner {
    int x;
    static const amon::conf::Struct SCHEMA;
};

const amon::conf::Struct Inner::SCHEMA {
    amon::conf::Field {.field = &Inner::x, .name = "x", .description = "inner x", .required = true},
};

struct Outer {
    std::string name;
    std::vector<Inner> items;
    std::map<std::string, int> dict;
    std::optional<int> opt;
    static const amon::conf::Struct SCHEMA;
};

const amon::conf::Struct Outer::SCHEMA {
    amon::conf::Field {
        .field = &Outer::name,
        .name = "name",
        .description = "name",
        .required = true},
    amon::conf::Field {
        .field = &Outer::items,
        .name = "items",
        .description = "list of inner",
        .required = false},
    amon::conf::Field {
        .field = &Outer::dict,
        .name = "dict",
        .description = "dict",
        .required = false},
    amon::conf::Field {
        .field = &Outer::opt,
        .name = "opt",
        .description = "optional int",
        .required = false},
};

TEST_CASE("YamlConf nested struct and list parsing")
{
    const std::string yaml = R"(
name: test
items:
  - x: 1
  - x: 2
dict:
  foo: 10
  bar: 20
opt: 99
)";
    auto o = amon::conf::yaml::parseInto<Outer>(yaml);
    CHECK(o.name == "test");
    REQUIRE(o.items.size() == 2);
    CHECK(o.items[0].x == 1);
    CHECK(o.items[1].x == 2);
    CHECK(o.dict.at("foo") == 10);
    CHECK(o.dict.at("bar") == 20);
    REQUIRE(o.opt.has_value());
    CHECK(o.opt.value() == 99); // NOLINT(bugprone-unchecked-optional-access)
}

TEST_CASE("YamlConf optional field default")
{
    const std::string yaml = R"(
name: test
)";
    auto o = amon::conf::yaml::parseInto<Outer>(yaml);
    CHECK(o.opt == std::nullopt);
    CHECK(o.items.empty());
    CHECK(o.dict.empty());
}

struct DuplicateFieldStruct {
    int a;
    int b;
    static const amon::conf::Struct SCHEMA;
};

TEST_CASE("YamlConf duplicate field names throws")
{
    CHECK_THROWS_AS(
        amon::conf::Struct(
            amon::conf::Field {
                .field = &DuplicateFieldStruct::a,
                .name = "dup",
                .description = "first field",
                .required = true},
            amon::conf::Field {
                .field = &DuplicateFieldStruct::b,
                .name = "dup",
                .description = "second field",
                .required = true}),
        amon::Error);
}

} // namespace
/// @endcond
#endif

} // namespace amon::conf::yaml
