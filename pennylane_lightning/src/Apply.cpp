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
#include <set>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <complex>

#include "Apply.hpp"
#include "Gates.hpp"
#include "StateVector.hpp"
#include "Util.hpp"

using std::set;
using std::string;
using std::unique_ptr;
using std::vector;

const std::complex<double> im(0.0,1.0);

vector<unsigned int> Pennylane::getIndicesAfterExclusion(const vector<unsigned int>& indicesToExclude, const unsigned int qubits) {
    set<unsigned int> indices;
    for (unsigned int i = 0; i < qubits; i++) {
        indices.insert(indices.end(), i);
    }
    for (const unsigned int& excludedIndex : indicesToExclude) {
        indices.erase(excludedIndex);
    }
    return vector<unsigned int>(indices.begin(), indices.end());
}

vector<size_t> Pennylane::generateBitPatterns(const vector<unsigned int>& qubitIndices, const unsigned int qubits) {
    vector<size_t> indices;
    indices.reserve(exp2(qubitIndices.size()));
    indices.push_back(0);
    for (int i = qubitIndices.size() - 1; i >= 0; i--) {
        size_t value = maxDecimalForQubit(qubitIndices[i], qubits);
        size_t currentSize = indices.size();
        for (size_t j = 0; j < currentSize; j++) {
            indices.push_back(indices[j] + value);
        }
    }
    return indices;
}

void Pennylane::constructAndApplyOperation(
    StateVector& state,
    const string& opLabel,
    const vector<unsigned int>& opWires,
    const vector<double>& opParams,
    bool inverse,
    const unsigned int qubits
) {
    unique_ptr<AbstractGate> gate = constructGate(opLabel, opParams);
    if (gate->numQubits != opWires.size())
        throw std::invalid_argument(string("The gate of type ") + opLabel + " requires " + std::to_string(gate->numQubits) + " wires, but " + std::to_string(opWires.size()) + " were supplied");

    vector<size_t> internalIndices = generateBitPatterns(opWires, qubits);

    vector<unsigned int> externalWires = getIndicesAfterExclusion(opWires, qubits);
    vector<size_t> externalIndices = generateBitPatterns(externalWires, qubits);

    gate->applyKernel(state, internalIndices, externalIndices, inverse);
}

void Pennylane::applyGateGenerator(
    StateVector& state,
    unique_ptr<AbstractGate> const &gate,
    const vector<unsigned int>& opWires,
    const unsigned int qubits
) {
    vector<size_t> internalIndices = generateBitPatterns(opWires, qubits);

    vector<unsigned int> externalWires = getIndicesAfterExclusion(opWires, qubits);
    vector<size_t> externalIndices = generateBitPatterns(externalWires, qubits);

    gate->applyGenerator(state, internalIndices, externalIndices);
}

void Pennylane::apply(
    StateVector& state,
    const vector<string>& ops,
    const vector<vector<unsigned int>>& wires,
    const vector<vector<double>>& params,
    const vector<bool>& inverse,
    const unsigned int qubits
) {
    if (qubits <= 0)
        throw std::invalid_argument("Must specify one or more qubits");

    size_t expectedLength = exp2(qubits);
    if (state.length != expectedLength)
        throw std::invalid_argument(string("Input state vector length (") + std::to_string(state.length) + ") does not match the given number of qubits " + std::to_string(qubits));

    size_t numOperations = ops.size();
    if (numOperations != wires.size() || numOperations != params.size())
        throw std::invalid_argument("Invalid arguments: number of operations, wires, and parameters must all be equal");

    for (int i = 0; i < numOperations; i++) {
        constructAndApplyOperation(state, ops[i], wires[i], params[i], inverse[i], qubits);
    }

}

void Pennylane::adjointJacobian(
    StateVector& phi,
    double* jac,
    const vector<string>& observables,
    const vector<vector<double> >& obsParams,
    const vector<vector<unsigned int> >& obsWires,
    const vector<string>& operations,
    const vector<vector<double> >& opParams,
    const vector<vector<unsigned int> >& opWires,
    const vector<int>& trainableParams,
    int paramNumber
) {
    // TODO: need to generalize
    int num_qubits = 2;

    size_t numObservables = observables.size();
    size_t trainableParamNumber = trainableParams.size() - 1;

    CplxType* lambdaStateArr = new CplxType[phi.length];
    std::memcpy(lambdaStateArr, phi.arr, sizeof(CplxType)*phi.length);
    Pennylane::StateVector lambdaState(lambdaStateArr, phi.length);

    // 1. Apply the unitaries (\hat{U}_{1:P}) to lambda
    std::vector<bool> inverses(operations.size(), false);
    apply(lambdaState, operations, opWires, opParams, inverses, num_qubits);

    // mult_observables();
    vector<Pennylane::StateVector> lambdas;
    for (unsigned int i = 0; i < numObservables; i++) {
        // copy |phi> and apply observables one at a time
        CplxType* phiCopyArr = new CplxType[lambdaState.length];
        std::memcpy(phiCopyArr, lambdaState.arr, sizeof(CplxType)*lambdaState.length);
        Pennylane::StateVector phiCopy(phiCopyArr, lambdaState.length);

        Pennylane::constructAndApplyOperation(
            phiCopy,
            observables[i],
            obsWires[i],
            obsParams[i],
            false,
            num_qubits
        );

        lambdas.push_back(phiCopy);
    }

    for (int i = operations.size() - 1; i >= 0; i--) {
        if (opParams[i].size() > 1) {
            throw std::invalid_argument("The operation is not supported using the adjoint differentiation method");
        } else if ((operations[i] != "QubitStateVector") && (operations[i] != "BasisState")) {
            // copy |phi> to |mu> before applying Uj*
            CplxType* phiCopyArr = new CplxType[lambdaState.length];
            std::memcpy(phiCopyArr, lambdaState.arr, sizeof(CplxType)*lambdaState.length);
            Pennylane::StateVector mu(phiCopyArr, lambdaState.length);

            // create |phi'> = Uj*|phi>
            Pennylane::constructAndApplyOperation(
                lambdaState,
                operations[i],
                opWires[i],
                opParams[i],
                true,
                num_qubits
            );

            // We have a parametrized gate
            if (!opParams[i].empty()){
                if (std::find(trainableParams.begin(), trainableParams.end(), paramNumber) != trainableParams.end()) {

                    // create iH|phi> = d/d dUj/dtheta Uj* |phi> = dUj/dtheta|phi'>
                    unique_ptr<AbstractGate> gate = constructGate(operations[i], opParams[i]);
                    // double scalingFactor = gate->generatorScalingFactor;
                    double scalingFactor = Pennylane::RotationYGate::generatorScalingFactor;
                    Pennylane::applyGateGenerator(
                        mu,
                        gate,
                        opWires[i],
                        num_qubits
                    );

                    for (unsigned int j = 0; j < lambdas.size(); j++) {
                        CplxType sum = Pennylane::inner_product(lambdas[j], mu);
                        // calculate 2 * shift * Real(i * sum) = -2 * shift * Imag(sum)
                        jac[j * trainableParams.size() + trainableParamNumber] = -2 * scalingFactor * std::imag(sum);
                    }
                    delete[] phiCopyArr;
                    trainableParamNumber--;
                }
                paramNumber--;
        }

        // if i > 0: (?)
        if(i > 0){
            for (unsigned int j = 0; j < lambdas.size(); j++) {
                Pennylane::constructAndApplyOperation(
                    lambdas[j],
                    operations[i],
                    opWires[i],
                    opParams[i],
                    true,
                    num_qubits
                );
            }
        }
        }
    }
    // delete copied state arrays
    for (int i; i < lambdas.size(); i++) {
        delete[] lambdas[i].arr;
    }
}
