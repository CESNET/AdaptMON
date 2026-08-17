/**
 * @file
 * @author Michal Sedlak <sedlakm@cesnet.cz>
 * @brief Pipeline blueprint declaration.
 * @copyright Copyright (c) 2026 CESNET, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <amon/dpdk/pipeline/Stage.hpp>

#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

namespace amon::dpdk::pipeline {

/**
 * @brief Describes the stages from which a pipeline is instantiated.
 *
 * A blueprint owns deferred stage constructors. It can therefore be assembled before pipeline
 * runtime resources are acquired and later transferred to a Pipeline.
 */
class Blueprint {
public:
    /// @brief Default ctor.
    Blueprint() = default;
    /// @brief Default dtor.
    ~Blueprint() = default;
    /// @brief Deleted copy ctor.
    Blueprint(const Blueprint&) = delete;
    /// @brief Deleted copy assignment.
    Blueprint& operator=(const Blueprint&) = delete;
    /// @brief Default move ctor.
    Blueprint(Blueprint&&) = default;
    /// @brief Default move assignment.
    /// @return Self.
    Blueprint& operator=(Blueprint&&) = default;

    /**
     * @brief Append a stage definition to the pipeline.
     *
     * The supplied arguments are retained until the stage is instantiated. Lvalues are retained
     * by reference and rvalues are owned by the blueprint.
     *
     * @tparam StageType Stage implementation to instantiate. Must derive from Stage.
     * @tparam StageArgs Constructor argument types.
     * @param stageArgs Arguments passed to the stage constructor.
     * @return Self to allow chaining.
     *
     * @warning References to the supplied arguments must be valid until the blueprint is consumed
     * by the Pipeline constructor!
     */
    template <typename StageType, typename... StageArgs, typename Self>
        requires std::is_base_of_v<Stage, StageType>
    decltype(auto) add(this Self&& self, StageArgs&&... stageArgs)
    {
        self.m_stageFactories.emplace_back(
            [args = std::tuple<CaptureArg<StageArgs>...>(
                 std::forward<StageArgs>(stageArgs)...)]() mutable -> std::unique_ptr<Stage> {
                return std::apply(
                    [](auto&&... args) -> std::unique_ptr<Stage> {
                        return std::make_unique<StageType>(std::forward<decltype(args)>(args)...);
                    },
                    std::move(args));
            });
        return std::forward<Self>(self);
    }

private:
    template <class T>
    using CaptureArg = std::conditional_t<
        std::is_lvalue_reference_v<T>,
        T, // lvalues: store reference
        std::decay_t<T> // rvalues: own value
        >;

    friend class Pipeline;
    std::vector<std::function<std::unique_ptr<Stage>()>> m_stageFactories;
};

} // namespace amon::dpdk::pipeline
