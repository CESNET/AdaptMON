/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Configuration parser implementation - yaml specialization
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * Example usage:
 *
 * @code{.cpp}
 * #include <amon/Logger.hpp>
 * #include <amon/conf/Struct.hpp>
 * #include <amon/conf/yaml.hpp>
 *
 * #include <iostream>
 * #include <string>
 *
 * namespace config {
 *
 * enum class TelemetryState {
 *     ON,
 *     OFF,
 *     AUTO,
 * };
 *
 * struct Ring {
 *     std::string name;
 *     int capacity;
 *
 *     const static amon::conf::Struct SCHEMA;
 * };
 *
 * struct Pipeline {
 *     std::vector<std::string> inputFrom;
 *     std::string outputTo;
 *     std::string outputToPipeline;
 *     std::optional<Ring> outputToRing;
 *
 *     const static amon::conf::Struct SCHEMA;
 * };
 *
 * struct PipelineParams {
 *     int burstSize;
 *     int packetMtu;
 *     int packetPool;
 *     int workers;
 *     int rxDescriptors;
 *     int txDescriptors;
 *
 *     const static amon::conf::Struct SCHEMA;
 * };
 *
 * struct Global {
 *     std::vector<std::string> inputs;
 *     std::vector<std::string> outputs;
 *     TelemetryState telemetrState;
 *     std::string telemetryDir;
 *     PipelineParams pipelineParams;
 *     std::map<std::string, Pipeline> pipeline;
 *
 *     const static amon::conf::Struct SCHEMA;
 * };
 *
 * const amon::conf::Struct Global::SCHEMA {
 *     amon::conf::Field {
 *         .field = &Global::inputs,
 *         .name = "inputs",
 *         .description = "The input cards",
 *         .required = true},
 *     amon::conf::Field {
 *         .field = &Global::outputs,
 *         .name = "outputs",
 *         .description = "The output cards",
 *         .required = true},
 *     amon::conf::Field {
 *         .field = &Global::telemetryDir,
 *         .name = "telemetry_dir",
 *         .description = "The telemetry directory path",
 *         .required = false,
 *         .defaultValue = "/var/run/distributor"},
 *     amon::conf::Field {
 *         .field = &Global::telemetrState,
 *         .name = "telemetry_state",
 *         .description = "The telemetry behavior",
 *         .required = false,
 *         .defaultValue = TelemetryState::AUTO},
 *     amon::conf::Field {
 *         .field = &Global::pipelineParams,
 *         .name = "pipeline_params",
 *         .description = "The common pipeline options",
 *         .required = true},
 *     amon::conf::Field {
 *         .field = &Global::pipeline,
 *         .name = "pipeline",
 *         .description = "The pipeline definition",
 *         .required = true},
 * };
 *
 * const amon::conf::Struct PipelineParams::SCHEMA {
 *     amon::conf::Field {
 *         .field = &PipelineParams::burstSize,
 *         .name = "burst_size",
 *         .description = "The burst size",
 *         .required = false,
 *         .defaultValue = 64},
 *     amon::conf::Field {
 *         .field = &PipelineParams::packetMtu,
 *         .name = "packet_mtu",
 *         .description = "The max size of the packet",
 *         .required = true},
 *     amon::conf::Field {
 *         .field = &PipelineParams::packetPool,
 *         .name = "packet_pool",
 *         .description = "The packet pool size",
 *         .required = true},
 *     amon::conf::Field {
 *         .field = &PipelineParams::workers,
 *         .name = "workers",
 *         .description = "The worker count",
 *         .required = true},
 *     amon::conf::Field {
 *         .field = &PipelineParams::rxDescriptors,
 *         .name = "rx_descriptors",
 *         .description = "The rx descriptor count",
 *         .required = true},
 *     amon::conf::Field {
 *         .field = &PipelineParams::txDescriptors,
 *         .name = "tx_descriptors",
 *         .description = "The tx descriptor count",
 *         .required = true},
 * };
 *
 * const amon::conf::Struct Pipeline::SCHEMA {
 *     amon::conf::Field {
 *         .field = &Pipeline::inputFrom,
 *         .name = "input_from",
 *         .description = "The input ports",
 *         .required = false,
 *         .defaultValue = {}},
 *     amon::conf::Field {
 *         .field = &Pipeline::outputTo,
 *         .name = "output_to",
 *         .description = "The output port",
 *         .required = false,
 *         .defaultValue = ""},
 *     amon::conf::Field {
 *         .field = &Pipeline::outputToPipeline,
 *         .name = "output_to_pipeline",
 *         .description = "The output pipeline",
 *         .required = false,
 *         .defaultValue = ""},
 *     amon::conf::Field {
 *         .field = &Pipeline::outputToRing,
 *         .name = "output_to_ring",
 *         .description = "The output rings",
 *         .required = false,
 *         .defaultValue = {}},
 * };
 *
 * const amon::conf::Struct Ring::SCHEMA {
 *     amon::conf::Field {
 *         .field = &Ring::name,
 *         .name = "name",
 *         .description = "The ring name pattern",
 *         .required = true},
 *     amon::conf::Field {
 *         .field = &Ring::capacity,
 *         .name = "capacity",
 *         .description = "The ring capacity",
 *         .required = false,
 *         .defaultValue = 8192,
 *         .parser = [](const amon::conf::Scalar &scalar) {
 *             auto val = scalar.asUInt();
 *             if (val > 32768) { throw std::runtime_error("bad value"); }
 *             return val;
 *          }},
 * };
 *
 * } // namespace config
 *
 * template <>
 * struct amon::conf::Parser<config::TelemetryState> {
 *     static config::TelemetryState parse(const amon::conf::Scalar& scalar)
 *     {
 *         auto value = scalar.asString();
 *         if (value == "on") {
 *             return config::TelemetryState::ON;
 *         }
 *         if (value == "off") {
 *             return config::TelemetryState::OFF;
 *         }
 *         if (value == "auto") {
 *             return config::TelemetryState::AUTO;
 *         }
 *         throw std::runtime_error(R"(value must be one of "on", "off", "auto")");
 *     }
 * };
 *
 * const std::string yaml = R"(
 * inputs:
 *   - 0000:17:00.0
 *   - 0000:ca:00.0
 * outputs:
 *   - 0000:98:00.0
 * telemetry_state: on
 * telemetry_dir: /var/run/amon-telemetry
 * pipeline_params:
 *   burst_size: 64
 *   packet_mtu: 9978
 *   packet_pool: 65536
 *   workers: 8
 *   rx_descriptors: 2048
 *   tx_descriptors: 2048
 * pipeline:
 *   alpha:
 *     input_from:
 *       - 0000:17:00.0
 *     output_to_pipeline: beta
 *     output_to_ring:
 *       name: alpha_ring_%d
 *       capacity: 32768
 *   beta:
 *     input_from:
 *       - 0000:ca:00.0
 *     output_to: 0000:98:00.0
 *     output_to_ring:
 *       name: beta_ring_%d
 *       capacity: 32768
 * )";
 *
 * int main()
 * {
 *     try {
 *         const auto config = amon::conf::yaml::parseInto<config::Global>(yaml);
 *
 *         std::string schema;
 *         config::Global::SCHEMA.toString(std::back_inserter(schema));
 *         std::cout << schema << '\n';
 *
 *     } catch (const amon::conf::ConfigError& err) {
 *         std::cerr << err.toString() << '\n';
 *     }
 * }
 * @endcode
 */

#pragma once

#include <amon/conf/Node.hpp>
#include <amon/conf/Scalar.hpp>

namespace amon::conf::yaml {

/**
 * @brief Parses a YAML string into a struct of type StructT.
 *
 * @tparam StructT The type of struct to parse into.
 * @param yamlString The YAML string to parse.
 * @return StructT The parsed struct of type StructT.
 */
template <typename StructT>
StructT parseInto(const std::string& yamlString)
{
    return parseInto<Node>(yamlString).parseInto<StructT>();
}

/**
 * @brief Parses a YAML string into a Node.
 *
 * @param yamlString The YAML string to parse.
 * @return Node The parsed Node object.
 */
template <>
Node parseInto(const std::string& yamlString);

/**
 * @class Scalar
 * @brief Represents a scalar value in a YAML document.
 *
 * Inherits from Scalar and provides YAML-specific scalar functionality.
 */
class Scalar : public amon::conf::Scalar {
public:
    /**
     * @brief Construct a Scalar from a string value.
     * @param value The string value to initialize the scalar with.
     */
    Scalar(std::string value);

    /**
     * @brief Get the scalar value as a string.
     * @return The value as a std::string.
     */
    std::string asString() const override;

    /**
     * @brief Get the scalar value as a boolean.
     * @return The value as a bool.
     * @throws amon::Error if the value cannot be converted to bool.
     */
    bool asBool() const override;

    /**
     * @brief Get the scalar value as an unsigned integer.
     * @return The value as a uint64_t.
     * @throws amon::Error if the value cannot be converted to uint64_t.
     */
    uint64_t asUInt() const override;

    /**
     * @brief Get the scalar value as a signed integer.
     * @return The value as an int64_t.
     * @throws amon::Error if the value cannot be converted to int64_t.
     */
    int64_t asInt() const override;

    /**
     * @brief Get the scalar value as a double.
     * @return The value as a double.
     * @throws amon::Error if the value cannot be converted to double.
     */
    double asDouble() const override;

private:
    std::string m_value;
};

} // namespace amon::conf::yaml
