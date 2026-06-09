/**
 * @file
 * @author Petr Macha <petr.macha@cesnet.cz>
 * @brief Single node of the protocol layer tree
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 */

#pragma once

#include <amon/layers/Layer.hpp>

#include <vector>

/**
 * @brief Single node of the protocol layer tree.
 */
struct LayerNode {
    /** Type of layer for the current node. */
    amon::layers::LayerType layerType;
    /** Error from parser, NONE if parsed cleanly. */
    amon::layers::LayerError error = amon::layers::LayerError::NONE;
    /** Number of times this layer has occurred. */
    uint64_t count = 0;
    /** Stores layers below this one. */
    std::vector<LayerNode> nodes;
};
