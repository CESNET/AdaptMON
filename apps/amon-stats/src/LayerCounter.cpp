/**
 * @file
 * @author Petr Macha <petr.macha@cesnet.cz>
 * @brief Layer counting class
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#include "LayerCounter.hpp"

#include <amon/Error.hpp>
#include <amon/amon.hpp>
#include <amon/util/Pcap.hpp>
#include <amon/util/ansi.hpp>
#include <amon/util/io.hpp>

#include <print>

std::pair<uint32_t, std::string>
LayerCounter::printNodes(std::string_view prefix, const LayerNode& root)
{
    std::string fullText;
    uint64_t lineCount = 0;
    if (root.count == 0) {
        return {lineCount, fullText};
    }

    for (size_t i = 0; i < root.nodes.size(); i++) {
        const LayerNode& node = root.nodes[i];
        auto count = static_cast<double>(node.count);

        double relativePercentage = (count / static_cast<double>(root.count)) * 100.0;
        double absolutePercentage = (count / static_cast<double>(m_rootNode.count)) * 100.0;

        std::string layerError;
        if (node.error != amon::layers::LayerError::NONE) {
            layerError = std::format(" [{}]", node.error);
        }

        fullText += std::format(
            AMON_ANSI_CLEAR_LINE "{}+- {}{}: {:.2f}% ({:.2f}% ({}))\n",
            prefix,
            node.layerType,
            layerError,
            relativePercentage,
            absolutePercentage,
            node.count);

        std::string newPrefix;
        if (i == root.nodes.size() - 1) {
            newPrefix = "  ";
        } else {
            newPrefix = "| ";
        }

        lineCount++;
        auto [newLines, newText] = printNodes(std::format("{}{}", prefix, newPrefix), node);
        lineCount += newLines;
        fullText += newText;
    }

    return {lineCount, fullText};
}

uint32_t LayerCounter::printNodes()
{
    if (m_rootNode.count == 0) {
        return 0;
    }

    std::string fullText;
    uint32_t lineCount;
    const std::string prefix;
    {
        const std::lock_guard<std::mutex> lock(m_memberMutex);
        std::tie(lineCount, fullText) = printNodes(prefix, m_rootNode);
    }

    std::print("{}", fullText);

    std::flush(std::cout);

    return lineCount;
}

static LayerNode& count(const amon::Packet& packet, LayerNode& parentNode, size_t layerIndex)
{
    auto error = amon::layers::LayerError::NONE;
    const amon::layers::LayerType layer = amon::match(
        packet.layers[layerIndex],
        [&](const amon::PacketLayer& packetLayer) { return packetLayer.type; },
        [&](const amon::ErrorLayer& errorLayer) {
            error = errorLayer.error;
            return errorLayer.type;
        });

    for (auto& node : parentNode.nodes) {
        if (node.layerType == layer && node.error == error) {
            node.count++;
            return node;
        }
    }

    return parentNode.nodes.emplace_back(layer, error, 1);
}

void LayerCounter::countAll(amon::Packet& packet)
{
    const std::lock_guard<std::mutex> lock(m_memberMutex);

    m_rootNode.count++;
    LayerNode* currentNode = &m_rootNode;
    for (size_t i = 0; i < packet.layers.size(); i++) {
        currentNode = &count(packet, *currentNode, i);
    }
}

static auto g_cmp = [](const LayerNode& x, const LayerNode& y) { return x.count > y.count; };

static void sortNodes(LayerNode& root)
{
    std::ranges::sort(root.nodes, g_cmp);

    for (auto& node : root.nodes) {
        sortNodes(node);
    }
}

void LayerCounter::sort()
{
    const std::lock_guard<std::mutex> lock(m_memberMutex);
    sortNodes(m_rootNode);
}

static std::string writeNodes(const LayerNode& root, std::string prefix = "")
{
    std::string text;
    for (const auto& node : root.nodes) {
        text += std::format("{0}- layer: {1}\n", prefix, node.layerType);
        text += std::format("{0}  count: {1}\n", prefix, node.count);

        if (node.error != amon::layers::LayerError::NONE) {
            text += std::format("{0}  error: {1}\n", prefix, node.error);
        }

        if (!node.nodes.empty()) {
            text += std::format("{0}  nodes:\n", prefix);
            text += writeNodes(node, prefix + "  ");
        }
    }

    return text;
}

void LayerCounter::writeToFile(std::string_view path)
{
    AMON_ASSERT(!path.empty());

    if (m_rootNode.count == 0) {
        return;
    }

    std::string fullText;
    {
        const std::lock_guard<std::mutex> lock(m_memberMutex);
        fullText = writeNodes(m_rootNode);
    }

    try {
        amon::util::io::writeAll(path, fullText);
    } catch (amon::Error& error) {
        m_logger.error("Failed to write to file: {}", error.what());
    }
}
