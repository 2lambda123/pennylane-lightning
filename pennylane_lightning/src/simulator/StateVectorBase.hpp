// Copyright 2021 Xanadu Quantum Technologies Inc.

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
 * @file StateVectorBase.hpp
 * Defines the class representation for quantum state vectors.
 */

#pragma once

#include "Constant.hpp"
#include "DefaultKernels.hpp"
#include "DynamicDispatcher.hpp"
#include "Error.hpp"
#include "SelectKernel.hpp"
#include "Util.hpp"

/// @cond DEV
// Required for compilation with MSVC
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES // for C++
#endif
/// @endcond

#include <cmath>
#include <complex>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

/**
 * @brief This macro defines methods for State-vector class. The kernel template
 * argument choose the kernel to run.
 */
#define PENNYLANE_STATEVECTOR_DEFINE_GATE(GATE_NAME)                           \
    template <Gates::KernelType kernel, typename... Ts>                        \
    inline void apply##GATE_NAME##_(const std::vector<size_t> &wires,          \
                                    bool inverse, Ts &&...args) {              \
        auto *arr = getData();                                                 \
        static_assert(Util::static_lookup<Gates::GateOperation::GATE_NAME>(    \
                          Gates::Constant::gate_num_params) == sizeof...(Ts),  \
                      "The provided number of parameters for gate " #GATE_NAME \
                      " is wrong.");                                           \
        static_assert(Util::array_has_elt(                                     \
                          Gates::SelectKernel<kernel>::implemented_gates,      \
                          Gates::GateOperation::GATE_NAME),                    \
                      "The kernel does not implement the gate.");              \
        Gates::SelectKernel<kernel>::apply##GATE_NAME(                         \
            arr, num_qubits_, wires, inverse, std::forward<Ts>(args)...);      \
    }

#define PENNYLANE_STATEVECTOR_DEFINE_GENERATOR(GENERATOR_NAME)                 \
    template <KernelType kernel, typename... Ts>                               \
    inline void applyGenerator##GENERATOR_NAME##_(                             \
        const std::vector<size_t> &wires, bool adj) {                          \
        auto *arr = getData();                                                 \
        static_assert(Util::array_has_elt(                                     \
                          Gates::SelectKernel<PrecisionT,                      \
                                              kernel>::implemented_generators, \
                          Gates::GeneratorOperation::GENERATOR_NAME),          \
                      "The kernel does not implement the gate generator.");    \
        SelectKernel<kernel>::applyGenerator##GENERATOR_NAME(arr, num_qubits_, \
                                                             wires, adj);      \
    }

namespace Pennylane {

/**
 * @brief State-vector base class.
 *
 * This class combines a data array managed by a derived class (CRTP) and
 * implementations of gate operations. The bound data is assumed to be complex,
 * and is required to be in either 32-bit (64-bit `complex<float>`) or
 * 64-bit (128-bit `complex<double>`) floating point representation.
 * As this is the base class, we do not add default template arguments.
 *
 * @tparam PrecisionT Floating point precision of underlying statevector data.
 * @tparam Derived Type of a derived class
 */
template <class PrecisionT, class Derived> class StateVectorBase {
  public:
    /**
     * @brief StateVector complex precision type.
     */
    using ComplexPrecisionT = std::complex<PrecisionT>;

  private:
    size_t num_qubits_{0};

  protected:
    explicit StateVectorBase(size_t num_qubits) : num_qubits_{num_qubits} {}

    /**
     * @brief Redefine the number of qubits in the statevector and number of
     * elements.
     *
     * @param qubits New number of qubits represented by statevector.
     */
    void setNumQubits(size_t qubits) { num_qubits_ = qubits; }

  public:
    /**
     * @brief Get the number of qubits represented by the statevector data.
     *
     * @return std::size_t
     */
    [[nodiscard]] auto getNumQubits() const -> std::size_t {
        return num_qubits_;
    }

    [[nodiscard]] size_t getLength() const {
        return static_cast<size_t>(Util::exp2(num_qubits_));
    }

    [[nodiscard]] inline auto getData() -> decltype(auto) {
        return static_cast<Derived *>(this)->getData();
    }

    [[nodiscard]] inline auto getData() const -> decltype(auto) {
        return static_cast<const Derived *>(this)->getData();
    }

    /**
     * @brief Compare two statevectors.
     *
     * @tparam RhsDerived The derived class for another statevector.
     * @param rhs Another statevector to compare.
     * @return bool
     */
    template <class RhsDerived>
    bool operator==(const StateVectorBase<PrecisionT, RhsDerived> &rhs) {
        if (num_qubits_ != rhs.getNumQubits()) {
            return false;
        }
        const ComplexPrecisionT *data1 = getData();
        const ComplexPrecisionT *data2 = rhs.getData();
        for (size_t k = 0; k < getLength(); k++) {
            if (data1[k] != data2[k]) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Apply a single gate to the state-vector using a given kernel.
     *
     * @param kernel Kernel to run the operation.
     * @param opName Name of gate to apply.
     * @param wires Wires to apply gate to.
     * @param inverse Indicates whether to use inverse of gate.
     * @param params Optional parameter list for parametric gates.
     */
    void applyOperation(Gates::KernelType kernel, const std::string &opName,
                        const std::vector<size_t> &wires, bool inverse = false,
                        const std::vector<PrecisionT> &params = {}) {
        auto *arr = getData();
        DynamicDispatcher<PrecisionT>::getInstance().applyOperation(
            kernel, arr, num_qubits_, opName, wires, inverse, params);
    }

    /**
     * @brief Apply a single gate to the state-vector.
     *
     * @param opName Name of gate to apply.
     * @param wires Wires to apply gate to.
     * @param inverse Indicates whether to use inverse of gate.
     * @param params Optional parameter list for parametric gates.
     */
    void applyOperation(const std::string &opName,
                        const std::vector<size_t> &wires, bool inverse = false,
                        const std::vector<PrecisionT> &params = {}) {
        auto *arr = getData();
        auto &dispatcher = DynamicDispatcher<PrecisionT>::getInstance();
        const auto gate_op = dispatcher.strToGateOp(opName);
        dispatcher.applyOperation(getDefaultKernelForGate(gate_op), arr,
                                  num_qubits_, gate_op, wires, inverse, params);
    }

    /**
     * @brief Apply multiple gates to the state-vector.
     *
     * @param ops Vector of gate names to be applied in order.
     * @param wires Vector of wires on which to apply index-matched gate name.
     * @param inverse Indicates whether gate at matched index is to be inverted.
     * @param params Optional parameter data for index matched gates.
     */
    void applyOperations(const std::vector<std::string> &ops,
                         const std::vector<std::vector<size_t>> &wires,
                         const std::vector<bool> &inverse,
                         const std::vector<std::vector<PrecisionT>> &params) {
        const size_t numOperations = ops.size();
        if (numOperations != wires.size()) {
            throw std::invalid_argument(
                "Invalid arguments: number of operations, wires, and "
                "parameters must all be equal");
        }
        for (size_t i = 0; i < numOperations; i++) {
            applyOperation(ops[i], wires[i], inverse[i], params[i]);
        }
    }

    /**
     * @brief Apply multiple gates to the state-vector.
     *
     * @param ops Vector of gate names to be applied in order.
     * @param wires Vector of wires on which to apply index-matched gate name.
     * @param inverse Indicates whether gate at matched index is to be inverted.
     */
    void applyOperations(const std::vector<std::string> &ops,
                         const std::vector<std::vector<size_t>> &wires,
                         const std::vector<bool> &inverse) {
        const size_t numOperations = ops.size();
        if (numOperations != wires.size()) {
            throw std::invalid_argument(
                "Invalid arguments: number of operations, wires, and "
                "parameters must all be equal");
        }
        for (size_t i = 0; i < numOperations; i++) {
            applyOperation(ops[i], wires[i], inverse[i], {});
        }
    }

    /**
     * @brief Apply a single generator to the state-vector using a given kernel.
     *
     * @param kernel Kernel to run the operation.
     * @param opName Name of gate to apply.
     * @param wires Wires to apply gate to.
     * @param adj Indicates whether to use adjoint of operator.
     */
    [[nodiscard]] inline auto applyGenerator(Gates::KernelType kernel,
                                             const std::string &opName,
                                             const std::vector<size_t> &wires,
                                             bool adj = false) -> PrecisionT {
        auto *arr = getData();
        return DynamicDispatcher<PrecisionT>::getInstance().applyGenerator(
            kernel, arr, num_qubits_, opName, wires, adj);
    }

    /**
     * @brief Apply a single generator to the state-vector.
     *
     * @param opName Name of gate to apply.
     * @param wires Wires the gate applies to.
     * @param adj Indicates whether to use adjoint of operator.
     */
    [[nodiscard]] auto applyGenerator(const std::string &opName,
                                      const std::vector<size_t> &wires,
                                      bool adj = false) -> PrecisionT {
        auto *arr = getData();
        const auto &dispatcher = DynamicDispatcher<PrecisionT>::getInstance();
        const auto gntr_op = dispatcher.strToGeneratorOp(opName);
        return dispatcher.applyGenerator(getDefaultKernelForGenerator(gntr_op),
                                         arr, num_qubits_, gntr_op, wires, adj);
    }

    /**
     * @brief Apply a given matrix directly to the statevector read directly
     * from numpy data. Data can be in 1D or 2D format.
     *
     * @param kernel Kernel to run the operation
     * @param matrix Pointer to the array data.
     * @param wires Wires to apply gate to.
     * @param inverse Indicate whether inverse should be taken.
     */
    inline void applyMatrix(Gates::KernelType kernel,
                            const ComplexPrecisionT *matrix,
                            const std::vector<size_t> &wires,
                            bool inverse = false) {
        using Gates::MatrixOperation;

        const auto &dispatcher = DynamicDispatcher<PrecisionT>::getInstance();
        auto *arr = getData();

        if (wires.empty()) {
            throw std::invalid_argument(
                "Number of wires must be larger than 0");
        }

        dispatcher.applyMatrix(kernel, arr, num_qubits_, matrix, wires,
                               inverse);
    }

    /**
     * @brief Apply a given matrix directly to the statevector read directly
     * from numpy data. Data can be in 1D or 2D format.
     *
     * @param matrix Pointer to the array data.
     * @param wires Wires to apply gate to.
     * @param inverse Indicate whether inverse should be taken.
     */
    inline void applyMatrix(const ComplexPrecisionT *matrix,
                            const std::vector<size_t> &wires,
                            bool inverse = false) {
        using Gates::MatrixOperation;

        if (wires.empty()) {
            throw std::invalid_argument(
                "Number of wires must be larger than 0");
        }

        const auto kernel = [n_wires = wires.size()]() {
            switch (n_wires) {
            case 1:
                return getDefaultKernelForMatrix(
                    MatrixOperation::SingleQubitOp);
            case 2:
                return getDefaultKernelForMatrix(MatrixOperation::TwoQubitOp);
            default:
                return getDefaultKernelForMatrix(MatrixOperation::MultiQubitOp);
            }
        }();
        applyMatrix(kernel, matrix, wires, inverse);
    }

    template <typename Alloc>
    inline void applyMatrix(const std::vector<ComplexPrecisionT, Alloc> &matrix,
                            const std::vector<size_t> &wires,
                            bool inverse = false) {
        if (matrix.size() != Util::exp2(2 * wires.size())) {
            throw std::invalid_argument(
                "The size of matrix does not match with the given "
                "number of wires");
        }

        applyMatrix(matrix.data(), wires, inverse);
    }

    /**
     * @brief Apply PauliX gate operation to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(PauliX)

    /**
     * @brief Apply PauliY gate operation to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(PauliY)

    /**
     * @brief Apply PauliZ gate operation to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(PauliZ)

    /**
     * @brief Apply Hadamard gate operation to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(Hadamard)

    /**
     * @brief Apply S gate operation to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(S)

    /**
     * @brief Apply T gate operation to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(T)

    /**
     * @brief Apply RX gate operation to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     * @param angle Rotation angle of gate.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(RX)

    /**
     * @brief Apply RY gate operation to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     * @param angle Rotation angle of gate.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(RY)

    /**
     * @brief Apply RZ gate operation to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     * @param angle Rotation angle of gate.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(RZ)

    /**
     * @brief Apply phase shift gate operation to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     * @param angle Phase shift angle.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(PhaseShift)

    /*
     * @brief Apply Rot gate \f$RZ(\omega)RY(\theta)RZ(\phi)\f$ to given indices
     * of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     * @param phi Gate rotation parameter \f$\phi\f$.
     * @param theta Gate rotation parameter \f$\theta\f$.
     * @param omega Gate rotation parameter \f$\omega\f$.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(Rot)

    /**
     * @brief Apply controlled phase shift gate operation to given indices of
     * statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     * @param angle Phase shift angle.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(ControlledPhaseShift)

    /**
     * @brief Apply CNOT (CX) gate to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(CNOT)

    /**
     * @brief Apply CY gate to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(CY)

    /**
     * @brief Apply CZ gate to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(CZ)

    /**
     * @brief Apply SWAP gate to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(SWAP)

    /**
     * @brief Apply CRX gate to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     * @param angle Rotation angle of gate.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(CRX)

    /**
     * @brief Apply CRY gate to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     * @param angle Rotation angle of gate.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(CRY)

    /**
     * @brief Apply CRZ gate to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     * @param angle Rotation angle of gate.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(CRZ)

    /**
     * @brief Apply CRot gate (controlled \f$RZ(\omega)RY(\theta)RZ(\phi)\f$) to
     * given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     * @param phi Gate rotation parameter \f$\phi\f$.
     * @param theta Gate rotation parameter \f$\theta\f$.
     * @param omega Gate rotation parameter \f$\omega\f$.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(CRot)

    /**
     * @brief Apply Toffoli (CCX) gate to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(Toffoli)

    /**
     * @brief Apply CSWAP gate to given indices of statevector.
     *
     * @param wires Wires to apply gate to.
     * @param inverse Take adjoint of given operation.
     */
    PENNYLANE_STATEVECTOR_DEFINE_GATE(CSWAP)
};

/**
 * @brief Streaming operator for StateVector data.
 *
 * @tparam T StateVector data precision.
 * @param out Output stream.
 * @param sv StateVector to stream.
 * @return std::ostream&
 */
template <class T, class Derived>
inline auto operator<<(std::ostream &out, const StateVectorBase<T, Derived> &sv)
    -> std::ostream & {
    const auto num_qubits = sv.getNumQubits();
    const auto data = sv.getData();
    const auto length = 1U << num_qubits;
    out << "num_qubits=" << num_qubits << std::endl;
    out << "data=[";
    out << data[0];
    for (size_t i = 1; i < length - 1; i++) {
        out << "," << data[i];
    }
    out << "," << data[length - 1] << "]";

    return out;
}

} // namespace Pennylane
