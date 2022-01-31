// Copyright 2022 Xanadu Quantum Technologies Inc.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/**
 * @file
 * Defines template classes extract member function pointers for
 * metaprogramming. Also defines some utility functions that calls such
 * pointers.
 */

#pragma once
#include "GateOperation.hpp"
#include "cassert"
#include <complex>
#include <vector>

namespace Pennylane {

/**
 * @brief Return a specific member function pointer for a given gate operation.
 * See speicalized classes.
 */
template <class PrecisionT, class ParamT, class GateImplementation,
          GateOperation gate_op>
struct GateOpToMemberFuncPtr {
    // raises compile error when used
    static_assert(
        gate_op != GateOperation::Matrix,
        "GateOpToMemberFuncPtr is not defined for GateOperation::Matrix.");
    static_assert(gate_op == GateOperation::Matrix,
                  "GateOpToMemberFuncPtr is not defined for the given gate. "
                  "When you define a new GateOperation, check that you also "
                  "have added the corresponding entry in "
                  "GateOpToMemberFuncPtr.");
    constexpr static auto value = nullptr;
};

template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::PauliX> {
    constexpr static auto value =
        &GateImplementation::template applyPauliX<PrecisionT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::PauliY> {
    constexpr static auto value =
        &GateImplementation::template applyPauliY<PrecisionT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::PauliZ> {
    constexpr static auto value =
        &GateImplementation::template applyPauliZ<PrecisionT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::Hadamard> {
    constexpr static auto value =
        &GateImplementation::template applyHadamard<PrecisionT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::S> {
    constexpr static auto value =
        &GateImplementation::template applyS<PrecisionT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::T> {
    constexpr static auto value =
        &GateImplementation::template applyT<PrecisionT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::PhaseShift> {
    constexpr static auto value =
        &GateImplementation::template applyPhaseShift<PrecisionT, ParamT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::RX> {
    constexpr static auto value =
        &GateImplementation::template applyRX<PrecisionT, ParamT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::RY> {
    constexpr static auto value =
        &GateImplementation::template applyRY<PrecisionT, ParamT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::RZ> {
    constexpr static auto value =
        &GateImplementation::template applyRZ<PrecisionT, ParamT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::Rot> {
    constexpr static auto value =
        &GateImplementation::template applyRot<PrecisionT, ParamT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::CNOT> {
    constexpr static auto value =
        &GateImplementation::template applyCNOT<PrecisionT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::CY> {
    constexpr static auto value =
        &GateImplementation::template applyCY<PrecisionT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::CZ> {
    constexpr static auto value =
        &GateImplementation::template applyCZ<PrecisionT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::SWAP> {
    constexpr static auto value =
        &GateImplementation::template applySWAP<PrecisionT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::IsingXX> {
    constexpr static auto value =
        &GateImplementation::template applyIsingXX<PrecisionT, ParamT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::IsingYY> {
    constexpr static auto value =
        &GateImplementation::template applyIsingYY<PrecisionT, ParamT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::IsingZZ> {
    constexpr static auto value =
        &GateImplementation::template applyIsingZZ<PrecisionT, ParamT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::ControlledPhaseShift> {
    constexpr static auto value =
        &GateImplementation::template applyControlledPhaseShift<PrecisionT,
                                                                ParamT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::CRX> {
    constexpr static auto value =
        &GateImplementation::template applyCRX<PrecisionT, ParamT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::CRY> {
    constexpr static auto value =
        &GateImplementation::template applyCRY<PrecisionT, ParamT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::CRZ> {
    constexpr static auto value =
        &GateImplementation::template applyCRZ<PrecisionT, ParamT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::CRot> {
    constexpr static auto value =
        &GateImplementation::template applyCRot<PrecisionT, ParamT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::Toffoli> {
    constexpr static auto value =
        &GateImplementation::template applyToffoli<PrecisionT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::CSWAP> {
    constexpr static auto value =
        &GateImplementation::template applyCSWAP<PrecisionT>;
};
template <class PrecisionT, class ParamT, class GateImplementation>
struct GateOpToMemberFuncPtr<PrecisionT, ParamT, GateImplementation,
                             GateOperation::MultiRZ> {
    constexpr static auto value =
        &GateImplementation::template applyMultiRZ<PrecisionT, ParamT>;
};

/**
 * @brief Return a specific member function pointer for a given generator
 * operation. See speicalized classes.
 */
template <class PrecisionT, class GateImplementation,
          GeneratorOperation gntr_op>
struct GeneratorOpToMemberFuncPtr {
    // raises compile error when used
    static_assert(
        sizeof(GateImplementation) == -1,
        "GeneratorOpToMemberFuncPtr is not defined for the given generator. "
        "When you define a new GeneratorOperation, check that you also "
        "have added the corresponding entry in GeneratorOpToMemberFuncPtr.");
};

template <class PrecisionT, class GateImplementation>
struct GeneratorOpToMemberFuncPtr<PrecisionT, GateImplementation,
                                  GeneratorOperation::RX> {
    constexpr static auto value =
        &GateImplementation::template applyGeneratorRX<PrecisionT>;
};
template <class PrecisionT, class GateImplementation>
struct GeneratorOpToMemberFuncPtr<PrecisionT, GateImplementation,
                                  GeneratorOperation::RY> {
    constexpr static auto value =
        &GateImplementation::template applyGeneratorRY<PrecisionT>;
};
template <class PrecisionT, class GateImplementation>
struct GeneratorOpToMemberFuncPtr<PrecisionT, GateImplementation,
                                  GeneratorOperation::RZ> {
    constexpr static auto value =
        &GateImplementation::template applyGeneratorRZ<PrecisionT>;
};
template <class PrecisionT, class GateImplementation>
struct GeneratorOpToMemberFuncPtr<PrecisionT, GateImplementation,
                                  GeneratorOperation::PhaseShift> {
    constexpr static auto value =
        &GateImplementation::template applyGeneratorPhaseShift<PrecisionT>;
};
template <class PrecisionT, class GateImplementation>
struct GeneratorOpToMemberFuncPtr<PrecisionT, GateImplementation,
                                  GeneratorOperation::IsingXX> {
    constexpr static auto value =
        &GateImplementation::template applyGeneratorIsingXX<PrecisionT>;
};
template <class PrecisionT, class GateImplementation>
struct GeneratorOpToMemberFuncPtr<PrecisionT, GateImplementation,
                                  GeneratorOperation::IsingYY> {
    constexpr static auto value =
        &GateImplementation::template applyGeneratorIsingYY<PrecisionT>;
};
template <class PrecisionT, class GateImplementation>
struct GeneratorOpToMemberFuncPtr<PrecisionT, GateImplementation,
                                  GeneratorOperation::IsingZZ> {
    constexpr static auto value =
        &GateImplementation::template applyGeneratorIsingZZ<PrecisionT>;
};
template <class PrecisionT, class GateImplementation>
struct GeneratorOpToMemberFuncPtr<PrecisionT, GateImplementation,
                                  GeneratorOperation::CRX> {
    constexpr static auto value =
        &GateImplementation::template applyGeneratorCRX<PrecisionT>;
};
template <class PrecisionT, class GateImplementation>
struct GeneratorOpToMemberFuncPtr<PrecisionT, GateImplementation,
                                  GeneratorOperation::CRY> {
    constexpr static auto value =
        &GateImplementation::template applyGeneratorCRY<PrecisionT>;
};
template <class PrecisionT, class GateImplementation>
struct GeneratorOpToMemberFuncPtr<PrecisionT, GateImplementation,
                                  GeneratorOperation::CRZ> {
    constexpr static auto value =
        &GateImplementation::template applyGeneratorCRZ<PrecisionT>;
};
template <class PrecisionT, class GateImplementation>
struct GeneratorOpToMemberFuncPtr<PrecisionT, GateImplementation,
                                  GeneratorOperation::ControlledPhaseShift> {
    constexpr static auto value =
        &GateImplementation::template applyGeneratorControlledPhaseShift<
            PrecisionT>;
};
template <class PrecisionT, class GateImplementation>
struct GeneratorOpToMemberFuncPtr<PrecisionT, GateImplementation,
                                  GeneratorOperation::MultiRZ> {
    constexpr static auto value =
        &GateImplementation::template applyGeneratorMultiRZ<PrecisionT>;
};

//
namespace Internal {
/**
 * @brief Gate operation pointer type for a statevector. See all specialized
 * types.
 */
template <class SVType, class ParamT, size_t num_params> struct GateMemFuncPtr {
    static_assert(num_params < 2 || num_params == 3,
                  "The given num_params is not supported.");
};
/**
 * @brief Function pointer type for a gate operation without parameters.
 */
template <class SVType, class ParamT> struct GateMemFuncPtr<SVType, ParamT, 0> {
    using Type = void (SVType::*)(const std::vector<size_t> &, bool);
};
/**
 * @brief Function pointer type for a gate operation with a single parameter.
 */
template <class SVType, class ParamT> struct GateMemFuncPtr<SVType, ParamT, 1> {
    using Type = void (SVType::*)(const std::vector<size_t> &, bool, ParamT);
};
/**
 * @brief Function pointer type for a gate operation with three parameters.
 */
template <class SVType, class ParamT> struct GateMemFuncPtr<SVType, ParamT, 3> {
    using Type = void (SVType::*)(const std::vector<size_t> &, bool, ParamT,
                                  ParamT, ParamT);
};

template <class SVType, class ParamT, size_t num_params>
using GateMemFuncPtrT =
    typename GateMemFuncPtr<SVType, ParamT, num_params>::Type;

/**
 * @brief Gate operation pointer type. See all specialized types.
 */
template <class PrecisionT, class ParamT, size_t num_params>
struct GateFuncPtr {
    static_assert(num_params < 2 || num_params == 3,
                  "The given num_params is not supported.");
};

/**
 * @brief Pointer type for a gate operation without parameters.
 */
template <class PrecisionT, class ParamT>
struct GateFuncPtr<PrecisionT, ParamT, 0> {
    using Type = void (*)(std::complex<PrecisionT> *, size_t,
                          const std::vector<size_t> &, bool);
};
/**
 * @brief Pointer type for a gate operation with a single parameter
 */
template <class PrecisionT, class ParamT>
struct GateFuncPtr<PrecisionT, ParamT, 1> {
    using Type = void (*)(std::complex<PrecisionT> *, size_t,
                          const std::vector<size_t> &, bool, ParamT);
};
/**
 * @brief Pointer type for a gate operation with three paramters
 */
template <class PrecisionT, class ParamT>
struct GateFuncPtr<PrecisionT, ParamT, 3> {
    using Type = void (*)(std::complex<PrecisionT> *, size_t,
                          const std::vector<size_t> &, bool, ParamT, ParamT,
                          ParamT);
};

/**
 * @brief Pointer type for a generator operation
 */
template <class PrecisionT> struct GeneratorFuncPtr {
    using Type = PrecisionT (*)(std::complex<PrecisionT> *, size_t,
                                const std::vector<size_t> &, bool);
};
} // namespace Internal

/**
 * @brief Convinient type alias for GateFuncPtr. See GateFuncPtr for details.
 */
template <class PrecisionT, class ParamT, size_t num_params>
using GateFuncPtrT =
    typename Internal::GateFuncPtr<PrecisionT, ParamT, num_params>::Type;

template <class PrecisionT>
using GeneratorFuncPtrT = typename Internal::GeneratorFuncPtr<PrecisionT>::Type;

/**
 * @defgroup Call gate operation with provided arguments
 *
 * @tparam PrecisionT Floating point type for the state-vector.
 * @tparam ParamT Floating point type for the gate paramters.
 * @param func Function pointer for the gate operation.
 * @param num_qubits The number of qubits of the state-vector.
 * @param wires Wires the gate applies to.
 * @param inverse If true, we apply the inverse of the gate.
 * @param params The list of gate paramters.
 */
/// @{
/**
 * @brief Overload for a gate operation without parameters
 */
template <class PrecisionT, class ParamT>
inline void callGateOps(GateFuncPtrT<PrecisionT, ParamT, 0> func,
                        std::complex<PrecisionT> *data, size_t num_qubits,
                        const std::vector<size_t> &wires, bool inverse,
                        [[maybe_unused]] const std::vector<ParamT> &params) {
    assert(params.empty());
    func(data, num_qubits, wires, inverse);
}

/**
 * @brief Overload for a gate operation for a single paramter
 */
template <class PrecisionT, class ParamT>
inline void callGateOps(GateFuncPtrT<PrecisionT, ParamT, 1> func,
                        std::complex<PrecisionT> *data, size_t num_qubits,
                        const std::vector<size_t> &wires, bool inverse,
                        const std::vector<ParamT> &params) {
    assert(params.size() == 1);
    func(data, num_qubits, wires, inverse, params[0]);
}

/**
 * @brief Overload for a gate operation for three paramters
 */
template <class PrecisionT, class ParamT>
inline void callGateOps(GateFuncPtrT<PrecisionT, ParamT, 3> func,
                        std::complex<PrecisionT> *data, size_t num_qubits,
                        const std::vector<size_t> &wires, bool inverse,
                        const std::vector<ParamT> &params) {
    assert(params.size() == 3);
    func(data, num_qubits, wires, inverse, params[0], params[1], params[2]);
}
/// @}
/**
 * @brief Call a generator operation.
 *
 * @tparam PrecisionT Floating point type for the state-vector.
 * @return Scaling factor
 */
template <class PrecisionT>
inline PrecisionT callGeneratorOps(GeneratorFuncPtrT<PrecisionT> func,
                                   std::complex<PrecisionT> *data,
                                   size_t num_qubits,
                                   const std::vector<size_t> &wires, bool adj) {
    return func(data, num_qubits, wires, adj);
}
} // namespace Pennylane