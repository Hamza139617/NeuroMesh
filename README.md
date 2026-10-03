# NeuroMesh

A **neural substrate** implemented from scratch in **C++**, designed to represent a dynamic, interconnected mesh of neurons and synapses with interactive visualization using **Raylib**.

NeuroMesh provides a computational substrate in which neuron-like entities can be organized into layers, connected through weighted synapses, and dynamically modified through operations such as insertion, deletion, pruning, and merging.

## Overview

NeuroMesh is a **dynamic neural substrate and custom data-structure system** designed to explore how interconnected neuron-like structures can be represented, organized, and dynamically reconfigured in software.

The substrate models:

* **Neurons** as computational nodes
* **Synapses** as weighted connections between neurons
* **Layers** as organized groups of neurons
* **Directional relationships** between neighboring neurons
* **Weights and thresholds** for structural operations
* **Dynamic restructuring** through insertion, deletion, pruning, and merging

The project also includes a **Raylib-based graphical interface** for visualizing the neural substrate.

> **Note:** NeuroMesh is a computational neural substrate and neural-inspired data structure. It is not a conventional machine-learning neural network and does not currently implement model training or backpropagation.

## What Is a Neural Substrate?

A **neural substrate** can be understood as the underlying structure in which neuron-like entities and their connections exist and interact.

In NeuroMesh, this substrate is represented computationally through:

```text
                 Neural Substrate
                       │
        ┌──────────────┼──────────────┐
        │              │              │
     Layer 1        Layer 2        Layer 3
        │              │              │
     Neurons        Neurons        Neurons
        │              │              │
     Synapses       Synapses       Synapses
        └──────────────┼──────────────┘
                       │
                Dynamic Structure
```

The substrate provides the structural foundation for representing neurons, their spatial relationships, and their weighted connections.

Rather than treating neurons as isolated objects, NeuroMesh models them as components of a **dynamic interconnected substrate** whose structure can change during execution.

## Core Components

### Neuron

A `Neuron` represents an individual node within the neural substrate.

Each neuron contains:

* `id`
* `weight`
* `up`
* `down`
* `left`
* `right`
* `head_axon`

The directional pointers allow neurons to maintain spatial relationships with neighboring neurons.

### Synapse

A `Synapse` represents a connection between neurons within the substrate.

Each synapse contains:

* `weight`
* `is_active`
* `direction`
* `target_neuron`
* `next_synapse`

This allows the substrate to represent weighted relationships between neuron-like entities.

### Layer

A `Layer` organizes neurons into a structured group.

Layers maintain:

* Layer ID
* Mesh dimension
* Current neuron count
* Top-left neuron
* Previous layer
* Next layer

The layers themselves form a linked structure:

```text
Layer <-> Layer <-> Layer <-> Layer
```

## Neural Substrate Operations

The substrate can dynamically change through several operations.

### Neuron Insertion

New neurons can be inserted into available positions within the mesh.

The system determines an appropriate placement, connects the neuron to its neighbors, and updates relevant synaptic relationships.

### Neuron Deletion

Existing neurons can be removed while the surrounding mesh is reorganized to fill the resulting vacancy.

Affected connections are subsequently reconstructed.

### Neuron Pruning

Neurons can be pruned according to their weights and the configured pruning threshold.

Pruning modifies the structure of the substrate while preserving the surrounding mesh relationships.

### Neuron Merging

Two neuron relationships can be merged through synaptic connections.

The operation can combine weights, transfer synaptic connections, remove the target neuron, and restructure the affected portion of the substrate.

## Weighted Connectivity

Connectivity is a central component of the neural substrate.

A simplified representation is:

```text
Neuron A
   │
   ├── Synapse ──► Neuron B
   │
   ├── Synapse ──► Neuron C
   │
   └── Synapse ──► Neuron D
```

Each synapse maintains its own weight and state, allowing the substrate to represent different strengths of connectivity.

## Dynamic Structure

Unlike a static collection of nodes, NeuroMesh is designed around a **dynamically reconfigurable substrate**.

Structural operations can modify:

* Neuron positions
* Neighbor relationships
* Layer membership
* Neuron weights
* Synaptic connections
* Overall mesh organization

This makes the project primarily an exploration of **dynamic data structures, connectivity, and neural-inspired computational organization**.

## Visualization

NeuroMesh uses **Raylib** to provide an interactive graphical representation of the neural substrate.

The visualization is intended to make the underlying structure and its dynamic organization easier to observe.

## File-Based Initialization

The initial substrate can be loaded from `mesh.txt`.

The configuration begins with:

```text
N  pruning_threshold  firing_threshold
```

For example:

```text
4 15.0 20.0
```

followed by neuron IDs and weights:

```text
1 4.0
2 9.0
3 2.0
4 7.0
...
```

## Technologies

* **C++**
* **Raylib**
* Dynamic memory management
* Pointers
* Structs and classes
* Linked data structures
* Graph-like connectivity
* File I/O
* Interactive visualization
* Custom data-structure design

## Project Structure

```text
NeuroMesh/
│
├── NeuroMesh/
│   ├── Header.h
│   ├── Source.cpp
│   ├── mesh.txt
│   ├── NeuroMesh.vcxproj
│   └── NeuroMesh.vcxproj.filters
│
├── .gitattributes
├── .gitignore
└── NeuroMesh.slnx
```

## Core Operations

The `NeuroMesh` class provides functionality for manipulating the neural substrate, including:

```text
insertNeuron()
deleteNeuron()
deleteLayer()
pruneNeuron()
mergeNeurons()
loadFromFile()
```

The system also contains functionality for rebuilding synaptic connections and maintaining the structural organization of the substrate.

## Learning Objectives

This project provided hands-on experience with:

* Dynamic memory management in C++
* Pointer manipulation
* Custom data structures
* Graph-like relationships
* Spatial organization of nodes
* File handling
* Dynamic insertion and deletion
* Structural reorganization
* Weighted connectivity
* Interactive visualization
* Designing a neural substrate from scratch

## Future Improvements

Potential future improvements include:

* [ ] Add automated tests for substrate operations
* [ ] Improve error handling for malformed input files
* [ ] Add richer synapse visualization
* [ ] Add interactive substrate controls
* [ ] Add configurable substrate generation
* [ ] Add performance benchmarks
* [ ] Improve memory-management safety using modern C++ practices
* [ ] Split the implementation into separate `.h` and `.cpp` files
* [ ] Add screenshots and demonstrations
* [ ] Explore learning and adaptive mechanisms on top of the substrate

## Author

**Hamza Khan**

Bachelor of Computer Science
FAST NUCES Islamabad

## Repository

**Hamza139617/NeuroMesh**

## License

This project currently does not specify a license.
