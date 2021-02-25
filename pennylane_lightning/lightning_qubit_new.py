# Copyright 2021 Xanadu Quantum Technologies Inc.

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
r"""
This module contains the :class:`~.LightningQubitNew` class, a PennyLane simulator device that
interfaces with C++ for fast linear algebra calculations.
"""
import warnings

from pennylane.devices import DefaultQubit
from .lightning_qubit_new_ops import apply
import numpy as np
from pennylane import QubitStateVector, BasisState, DeviceError

from ._version import __version__


class LightningQubitNew(DefaultQubit):
    """PennyLane Lightning device.

    An extension of PennyLane's built-in ``default.qubit`` device that interfaces with C++ to
    perform fast linear algebra calculations.

    Use of this device requires pre-built binaries or compilation from source. Check out the
    :doc:`/installation` guide for more details.

    Args:
        wires (int): the number of wires to initialize the device with
        shots (int): How many times the circuit should be evaluated (or sampled) to estimate
            the expectation values. Defaults to 1000 if not specified.
            If ``analytic == True``, then the number of shots is ignored
            in the calculation of expectation values and variances, and only controls the number
            of samples returned by ``sample``.
        analytic (bool): indicates if the device should calculate expectations
            and variances analytically
    """

    name = "Lightning Qubit PennyLane plugin"
    short_name = "lightning.qubit.new"
    pennylane_requires = ">=0.12"
    version = __version__
    author = "Xanadu Inc."

    operations = {
        "BasisState",
        "QubitStateVector",
        "PauliX",
        "PauliY",
        "PauliZ",
        "Hadamard",
        "S",
        "T",
        "CNOT",
        "SWAP",
        "CSWAP",
        "Toffoli",
        "CZ",
        "PhaseShift",
        "RX",
        "RY",
        "RZ",
        "Rot",
        "CRX",
        "CRY",
        "CRZ",
        "CRot",
    }

    observables = {"PauliX", "PauliY", "PauliZ", "Hadamard", "Identity"}

    def __init__(self, wires, *, shots=1000, analytic=True):
        super().__init__(wires, shots=shots, analytic=analytic)

    @classmethod
    def capabilities(cls):
        capabilities = super().capabilities().copy()
        capabilities.update(
            model="qubit",
            supports_reversible_diff=False,
            supports_inverse_operations=False,
            supports_analytic_computation=True,
            returns_state=True,
        )
        capabilities.pop("passthru_devices", None)
        return capabilities

    def apply(self, operations, rotations=None, **kwargs):
        for i, operation in enumerate(operations):  # State preparation is currently done in Python
            if isinstance(operation, (QubitStateVector, BasisState)):
                if i == 0:

                    if isinstance(operation, QubitStateVector):
                        self._apply_state_vector(operation.parameters[0], operation.wires)
                    else:
                        self._apply_basis_state(operation.parameters[0], operation.wires)

                    del operations[0]
                else:
                    raise DeviceError(
                        "Operation {} cannot be used after other Operations have already been "
                        "applied on a {} device.".format(operation.name, self.short_name)
                    )

        if operations:
            self._pre_rotated_state = self.apply_lightning(self._state, operations)
        else:
            self._pre_rotated_state = self._state

        if rotations:
            self._state = self.apply_lightning(np.copy(self._pre_rotated_state), rotations)
        else:
            self._state = self._pre_rotated_state

    def apply_lightning(self, state, operations):
        """Apply a list of operations to the state tensor.

        Args:
            state (array[complex]): the input state tensor
            operations (list[~pennylane.operation.Operation]): operations to apply

        Returns:
            array[complex]: the output state tensor
        """
        op_names = [o.name for o in operations]
        op_wires = [self.wires.indices(o.wires) for o in operations]
        op_param = [o.parameters for o in operations]

        state_vector = np.ravel(state)
        apply(state_vector, op_names, op_wires, op_param, self.num_wires)
        return np.reshape(state_vector, state.shape)