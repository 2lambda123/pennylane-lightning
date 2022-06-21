#include "Constant.hpp"
#include "ConstantUtil.hpp"
#include "Error.hpp"
#include "GateOperation.hpp"
#include "LinearAlgebra.hpp"
#include "Macros.hpp"
#include "Memory.hpp"
#include "StateVectorManagedCPU.hpp"
#include "TestKernels.hpp"
#include "Util.hpp"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <complex>
#include <concepts>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace Pennylane {

template <class T> struct is_std_vector {
    constexpr static bool value = false;
};
template <class T, class Alloc> struct is_std_vector<std::vector<T, Alloc>> {
    constexpr static bool value = true;
};
template <class T>
concept std_vector = is_std_vector<T>::value;

template <typename T, typename AllocComp>
struct VectorApprox : Catch::Matchers::MatcherGenericBase {
  private:
    const std::vector<T, AllocComp> &comp_;
    mutable Catch::Approx approx = Catch::Approx::custom();

  public:
    VectorApprox(const std::vector<T, AllocComp> &comp) : comp_{comp} {}

    std::string describe() const {
        using Util::operator<<;
        std::ostringstream ss;
        ss << "is approx to " << comp_;
        return ss.str();
    }

    template <typename AllocMatch>
    bool match(const std::vector<T, AllocMatch> &v) const {
        if (comp_.size() != v.size()) {
            return false;
        }

        if constexpr (Util::is_complex_v<T>) {
            for (size_t i = 0; i < v.size(); i++) {
                if ((std::real(comp_[i]) != approx(std::real(v[i]))) ||
                    (std::imag(comp_[i]) != approx(std::imag(v[i])))) {
                    return false;
                }
            }
        } else {
            for (size_t i = 0; i < v.size(); i++) {
                if (comp_[i] != approx(v[i])) {
                    return false;
                }
            }
        }
        return true;
    }

    VectorApprox &epsilon(Util::remove_complex_t<T> new_eps) {
        approx.epsilon(new_eps);
        return *this;
    }

    VectorApprox &margin(Util::remove_complex_t<T> new_margin) {
        approx.margin(new_margin);
        return *this;
    }
};

template <std_vector T> auto Approx(const T &comp) -> decltype(auto) {
    return VectorApprox(comp);
}

template <class T>
struct ComplexNumberApprox : Catch::Matchers::MatcherGenericBase {
  private:
    const std::complex<T> comp_;
    mutable Catch::Approx approx = Catch::Approx::custom();

  public:
    ComplexNumberApprox(const std::complex<T> &comp) : comp_{comp} {}

    std::string describe() const {
        std::ostringstream ss;
        ss << "is approx to " << comp_;
        return ss.str();
    }

    bool match(const std::complex<T> &v) const {
        return ((std::real(comp_) == approx(std::real(v))) &&
                (std::imag(comp_) == approx(std::imag(v))));
    }

    ComplexNumberApprox &epsilon(T new_eps) {
        approx.epsilon(new_eps);
        return *this;
    }

    ComplexNumberApprox &margin(T new_margin) {
        approx.margin(new_margin);
        return *this;
    }
};

template <class T>
struct RealNumberApprox : Catch::Matchers::MatcherGenericBase {
  private:
    const T comp_;
    mutable Catch::Approx approx = Catch::Approx::custom();

  public:
    RealNumberApprox(T comp) : comp_{comp} {}

    std::string describe() const {
        std::ostringstream ss;
        ss << "is approx to " << comp_;
        return ss.str();
    }

    bool match(T v) const { return comp_ == approx(std::real(v)); }

    RealNumberApprox &epsilon(T new_eps) {
        approx.epsilon(new_eps);
        return *this;
    }

    RealNumberApprox &margin(T new_margin) {
        approx.margin(new_margin);
        return *this;
    }
};

template <typename T> ComplexNumberApprox<T> Approx(std::complex<T> comp) {
    return ComplexNumberApprox(comp);
}
template <std::floating_point T> RealNumberApprox<T> Approx(T comp) {
    return RealNumberApprox(comp);
}

/**
 * @brief Utility function to compare complex statevector data.
 *
 * @tparam Data_t Floating point data-type.
 * @param data1 StateVector data 1.
 * @param data2 StateVector data 2.
 * @return true Data are approximately equal.
 * @return false Data are not approximately equal.
 */
template <class Data_t, class AllocA, class AllocB>
inline bool
isApproxEqual(const std::vector<Data_t, AllocA> &data1,
              const std::vector<Data_t, AllocB> &data2,
              const typename Data_t::value_type eps =
                  std::numeric_limits<typename Data_t::value_type>::epsilon() *
                  100) {
    return data1 == Approx<Data_t, AllocB>(data2).epsilon(eps);
}

/**
 * @brief Utility function to compare complex statevector data.
 *
 * @tparam Data_t Floating point data-type.
 * @param data1 StateVector data 1.
 * @param data2 StateVector data 2.
 * @return true Data are approximately equal.
 * @return false Data are not approximately equal.
 */
template <class Data_t>
inline bool
isApproxEqual(const Data_t &data1, const Data_t &data2,
              typename Data_t::value_type eps =
                  std::numeric_limits<typename Data_t::value_type>::epsilon() *
                  100) {
    return !(data1.real() != Catch::Approx(data2.real()).epsilon(eps) ||
             data1.imag() != Catch::Approx(data2.imag()).epsilon(eps));
}

template <typename T>
constexpr static auto test_allocator =
    Util::AlignedAllocator<T>{Util::common_alignment_v<T, TestKernels>};

template <typename T>
using TestVector = std::vector<T, Util::AlignedAllocator<T>>;

/**
 * @brief Multiplies every value in a dataset by a given complex scalar value.
 *
 * @tparam Data_t Precision of complex data type. Supports float and double
 * data.
 * @param data Data to be scaled.
 * @param scalar Scalar value.
 */
template <class Data_t, class Alloc>
void scaleVector(std::vector<std::complex<Data_t>, Alloc> &data,
                 std::complex<Data_t> scalar) {
    std::transform(
        data.begin(), data.end(), data.begin(),
        [scalar](const std::complex<Data_t> &c) { return c * scalar; });
}

/**
 * @brief Multiplies every value in a dataset by a given complex scalar value.
 *
 * @tparam Data_t Precision of complex data type. Supports float and double
 * data.
 * @param data Data to be scaled.
 * @param scalar Scalar value.
 */
template <class Data_t, class Alloc>
void scaleVector(std::vector<std::complex<Data_t>, Alloc> &data,
                 Data_t scalar) {
    std::transform(
        data.begin(), data.end(), data.begin(),
        [scalar](const std::complex<Data_t> &c) { return c * scalar; });
}

/**
 * @brief create |0>^N
 */
template <typename PrecisionT>
auto createZeroState(size_t num_qubits)
    -> TestVector<std::complex<PrecisionT>> {
    TestVector<std::complex<PrecisionT>> res(
        size_t{1U} << num_qubits, {0.0, 0.0},
        test_allocator<std::complex<PrecisionT>>);
    res[0] = std::complex<PrecisionT>{1.0, 0.0};
    return res;
}

/**
 * @brief create |+>^N
 */
template <typename PrecisionT>
auto createPlusState(size_t num_qubits)
    -> TestVector<std::complex<PrecisionT>> {
    TestVector<std::complex<PrecisionT>> res(
        size_t{1U} << num_qubits, {1.0, 0.0},
        test_allocator<std::complex<PrecisionT>>);
    for (auto &elt : res) {
        elt /= std::sqrt(1U << num_qubits);
    }
    return res;
}

/**
 * @brief create a random state
 */
template <typename PrecisionT, class RandomEngine>
auto createRandomState(RandomEngine &re, size_t num_qubits)
    -> TestVector<std::complex<PrecisionT>> {
    using Util::squaredNorm;

    TestVector<std::complex<PrecisionT>> res(
        size_t{1U} << num_qubits, {0.0, 0.0},
        test_allocator<std::complex<PrecisionT>>);
    std::uniform_real_distribution<PrecisionT> dist;
    for (size_t idx = 0; idx < (size_t{1U} << num_qubits); idx++) {
        res[idx] = {dist(re), dist(re)};
    }

    scaleVector(res, std::complex<PrecisionT>{1.0, 0.0} /
                         std::sqrt(Util::squaredNorm(res.data(), res.size())));
    return res;
}

/**
 * @brief Create an arbitrary product state in X- or Z-basis.
 *
 * Example: createProductState("+01") will produce |+01> state.
 * Note that the wire index starts from the left.
 */
template <typename PrecisionT>
auto createProductState(std::string_view str)
    -> TestVector<std::complex<PrecisionT>> {
    using Pennylane::Util::INVSQRT2;
    TestVector<std::complex<PrecisionT>> st(
        test_allocator<std::complex<PrecisionT>>);
    st.resize(1U << str.length());

    std::vector<PrecisionT> zero{1.0, 0.0};
    std::vector<PrecisionT> one{0.0, 1.0};

    std::vector<PrecisionT> plus{INVSQRT2<PrecisionT>(),
                                 INVSQRT2<PrecisionT>()};
    std::vector<PrecisionT> minus{INVSQRT2<PrecisionT>(),
                                  -INVSQRT2<PrecisionT>()};

    for (size_t k = 0; k < (size_t{1U} << str.length()); k++) {
        PrecisionT elt = 1.0;
        for (size_t n = 0; n < str.length(); n++) {
            char c = str[n];
            const size_t wire = str.length() - 1 - n;
            switch (c) {
            case '0':
                elt *= zero[(k >> wire) & 1U];
                break;
            case '1':
                elt *= one[(k >> wire) & 1U];
                break;
            case '+':
                elt *= plus[(k >> wire) & 1U];
                break;
            case '-':
                elt *= minus[(k >> wire) & 1U];
                break;
            default:
                PL_ABORT("Unknown character in the argument.");
            }
        }
        st[k] = elt;
    }
    return st;
}

inline auto createWires(Gates::GateOperation op, size_t num_qubits)
    -> std::vector<size_t> {
    if (Pennylane::Util::array_has_elt(Gates::Constant::multi_qubit_gates,
                                       op)) {
        std::vector<size_t> wires(num_qubits);
        std::iota(wires.begin(), wires.end(), 0);
        return wires;
    }
    switch (Pennylane::Util::lookup(Gates::Constant::gate_wires, op)) {
    case 1:
        return {0};
    case 2:
        return {0, 1};
    case 3:
        return {0, 1, 2};
    case 4:
        return {0, 1, 2, 3};
    default:
        PL_ABORT("The number of wires for a given gate is unknown.");
    }
    return {};
}

template <class PrecisionT>
auto createParams(Gates::GateOperation op) -> std::vector<PrecisionT> {
    switch (Pennylane::Util::lookup(Gates::Constant::gate_num_params, op)) {
    case 0:
        return {};
    case 1:
        return {static_cast<PrecisionT>(0.312)};
    case 3:
        return {static_cast<PrecisionT>(0.128), static_cast<PrecisionT>(-0.563),
                static_cast<PrecisionT>(1.414)};
    default:
        PL_ABORT("The number of parameters for a given gate is unknown.");
    }
    return {};
}

/**
 * @brief Initialize the statevector in a non-trivial configuration.
 *
 * @tparam T statevector float point precision.
 * @param num_qubits number of qubits
 * @return StateVectorManaged<T>
 */
template <typename T = double>
StateVectorManagedCPU<T> Initializing_StateVector(size_t num_qubits = 3) {
    size_t data_size = Util::exp2(num_qubits);

    std::vector<std::complex<T>> arr(data_size, 0);
    arr[0] = 1;
    StateVectorManagedCPU<T> Measured_StateVector(arr.data(), data_size);

    std::vector<std::string> gates;
    std::vector<std::vector<size_t>> wires;
    std::vector<bool> inv_op(num_qubits * 2, false);
    std::vector<std::vector<T>> phase;

    T initial_phase = 0.7;
    for (size_t n_qubit = 0; n_qubit < num_qubits; n_qubit++) {
        gates.emplace_back("RX");
        gates.emplace_back("RY");

        wires.push_back({n_qubit});
        wires.push_back({n_qubit});

        phase.push_back({initial_phase});
        phase.push_back({initial_phase});
        initial_phase -= 0.2;
    }
    Measured_StateVector.applyOperations(gates, wires, inv_op, phase);

    return Measured_StateVector;
}

/**
 * @brief Fills the empty vectors with the CSR (Compressed Sparse Row) sparse
 * matrix representation for a tridiagonal + periodic boundary conditions
 * Hamiltonian.
 *
 * @tparam fp_precision data float point precision.
 * @tparam index_type integer type used as indices of the sparse matrix.
 * @param row_map the j element encodes the total number of non-zeros above
 * row j.
 * @param entries column indices.
 * @param values  matrix non-zero elements.
 * @param numRows matrix number of rows.
 */
template <class fp_precision, class index_type>
void write_CSR_vectors(std::vector<index_type> &row_map,
                       std::vector<index_type> &entries,
                       std::vector<std::complex<fp_precision>> &values,
                       index_type numRows) {
    const std::complex<fp_precision> SC_ONE = 1.0;

    row_map.resize(numRows + 1);
    for (index_type rowIdx = 1; rowIdx < (index_type)row_map.size(); ++rowIdx) {
        row_map[rowIdx] = row_map[rowIdx - 1] + 3;
    };
    const index_type numNNZ = row_map[numRows];

    entries.resize(numNNZ);
    values.resize(numNNZ);
    for (index_type rowIdx = 0; rowIdx < numRows; ++rowIdx) {
        if (rowIdx == 0) {
            entries[0] = rowIdx;
            entries[1] = rowIdx + 1;
            entries[2] = numRows - 1;

            values[0] = SC_ONE;
            values[1] = -SC_ONE;
            values[2] = -SC_ONE;
        } else if (rowIdx == numRows - 1) {
            entries[row_map[rowIdx]] = 0;
            entries[row_map[rowIdx] + 1] = rowIdx - 1;
            entries[row_map[rowIdx] + 2] = rowIdx;

            values[row_map[rowIdx]] = -SC_ONE;
            values[row_map[rowIdx] + 1] = -SC_ONE;
            values[row_map[rowIdx] + 2] = SC_ONE;
        } else {
            entries[row_map[rowIdx]] = rowIdx - 1;
            entries[row_map[rowIdx] + 1] = rowIdx;
            entries[row_map[rowIdx] + 2] = rowIdx + 1;

            values[row_map[rowIdx]] = -SC_ONE;
            values[row_map[rowIdx] + 1] = SC_ONE;
            values[row_map[rowIdx] + 2] = -SC_ONE;
        }
    }
};

template <class PrecisionT> struct PrecisionToName;

template <> struct PrecisionToName<float> {
    constexpr static auto value = "float";
};
template <> struct PrecisionToName<double> {
    constexpr static auto value = "double";
};

#define PL_REQUIRE_THROWS_MATCHES(expr, type, message_match)                   \
    do {                                                                       \
        REQUIRE_THROWS_AS(expr, type);                                         \
        REQUIRE_THROWS_WITH(                                                   \
            expr, Catch::Matchers::ContainsSubstring(message_match));          \
    } while (false);

#define PL_CHECK_THROWS_MATCHES(expr, type, message_match)                     \
    do {                                                                       \
        CHECK_THROWS_AS(expr, type);                                           \
        CHECK_THROWS_WITH(expr,                                                \
                          Catch::Matchers::ContainsSubstring(message_match));  \
    } while (false);

} // namespace Pennylane
