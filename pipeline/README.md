# Pipeline Framework

A lightweight C++ framework for building high-performance processing pipelines without forcing application developers to rewrite the usual concurrency, threading, affinity, queueing, and data-passing infrastructure.

The core idea is simple:

> **Write the processing logic. Describe the pipeline. Let the runtime handle concurrency, placement, communication, and execution.**

---

## Vision

Applications such as image processing, computer vision, signal processing, robotics, scientific instrumentation, simulation workflows, and real-time analytics are often naturally expressed as pipelines.

For example:

```text
Camera
  |
  v
[ Decode ]
  |
  v
[ Resize ]
  |
  v
[ Filter ]
  |
  v
[ Detection ]
  |
  v
[ Encode ]
```

However, implementing such pipelines in native C or C++ often requires substantial infrastructure code for:

- thread creation and lifetime management
- CPU affinity and core pinning
- queues and synchronization
- buffer ownership
- backpressure
- shutdown handling
- error propagation
- load balancing
- performance statistics
- inter-stage data transfer

The goal of this framework is to move those responsibilities into a reusable runtime so that application programmers can focus primarily on the algorithms executed by each stage.

---

## Design Goal

A programmer should be able to express a pipeline conceptually like this:

```cpp
pipeline pipe;

pipe.add_stage("decode", decode);
pipe.add_stage("resize", resize);
pipe.add_stage("filter", filter);
pipe.add_stage("detect", detect);
pipe.add_stage("encode", encode);

pipe.connect("decode", "resize");
pipe.connect("resize", "filter");
pipe.connect("filter", "detect");
pipe.connect("detect", "encode");

pipe.run();
```

The user defines **what each stage does**.

The framework manages **how those stages execute and communicate**.

---

## Why C++

The initial implementation is planned in modern C++, with the possibility of exposing a C API later.

C++ provides useful language features for this type of runtime:

- RAII
- templates
- concepts
- lambdas
- move semantics
- atomics
- `std::thread` / `std::jthread`
- type-safe stage interfaces
- custom allocators
- strong resource lifetime management

Eventually, creating a stage could be as simple as:

```cpp
pipe.stage("grayscale", [](image input) {
    return grayscale(input);
});
```

---

## Core Architecture

The framework should separate four major concerns:

```text
Algorithm
    !=
Execution
    !=
Transport
    !=
Memory
```

This separation is central to making the framework reusable and extensible.

The initial architecture can revolve around five abstractions:

```text
pipeline
stage
channel
executor
buffer
```

### `pipeline`

Owns and coordinates the processing graph.

### `stage`

Contains the user-defined processing or algorithmic logic.

### `channel`

Transfers data between connected stages.

### `executor`

Defines how and where stages execute.

### `buffer`

Defines ownership, allocation, lifetime, and movement of pipeline data.

---

## Pluggable Execution Model

One of the long-term goals is to allow the same logical pipeline to run using different execution strategies without rewriting the stage algorithms.

Given:

```text
A -> B -> C -> D -> E
```

the runtime could support several mappings.

### One thread per stage

```text
Core 0     Core 1     Core 2     Core 3     Core 4

Stage A -> Stage B -> Stage C -> Stage D -> Stage E
thread 0    thread 1    thread 2    thread 3    thread 4
```

### Shared thread pool

```text
             Pipeline Graph

 A ---> B ---> C ---> D ---> E
        |
        v
     Scheduler
        |
  +-----+-----+
  v     v     v
 T0    T1    T2
```

### Multiple processes

```text
Process 0        Process 1        Process 2

[A][B]    IPC    [C][D]    IPC    [E]
```

### Distributed execution

```text
Node 0                     Node 1

Stage A -> Stage B --MPI--> Stage C -> Stage D
```

The logical pipeline remains unchanged while the execution policy changes.

A possible architecture is:

```text
            USER APPLICATION

             Pipeline Graph
                  |
                  v
          +-----------------+
          |     Runtime     |
          +--------+--------+
                   |
          +--------+---------+
          | Execution Policy|
          +--------+---------+
                   |
        +----------+-----------+
        |          |           |
        v          v           v
     threads    processes      MPI
        |          |           |
        +----------+-----------+
                   |
                   v
            Transport Layer
```

---

## CPU Affinity

CPU affinity should be a first-class feature.

Instead of requiring users to write platform-specific affinity logic:

```cpp
pipe.stage("decode", decode)
    .cores({0, 1});

pipe.stage("filter", filter)
    .cores({2, 3});

pipe.stage("detect", detection)
    .cores({4, 5, 6, 7});
```

The same mapping could also be provided through configuration:

```yaml
stages:
  decode:
    cores: [0]

  resize:
    cores: [1]

  inference:
    cores: [2, 3, 4, 5]

  encode:
    cores: [6]
```

This allows hardware placement to be changed without modifying the algorithms themselves.

---

## Backpressure

Backpressure should be part of the framework from the beginning.

Consider:

```text
Stage A
1000 items/sec
     |
     v
Stage B
100 items/sec
```

Without bounded buffering, the queue between the two stages can grow indefinitely.

Instead:

```text
A ---> [ bounded queue: 32 ] ---> B
```

Possible policies could include:

```cpp
.block()
.drop_oldest()
.drop_newest()
.overwrite()
```

This makes overload behavior explicit and predictable.

---

## Observability

A future goal is to make the runtime observable so that developers can understand pipeline behaviour while the application is running.

Example:

```text
Pipeline: camera_processing

Stage          Rate       Latency      Queue     CPU
------------------------------------------------------
decode         118 FPS     2.1 ms       2/32      71%
resize         118 FPS     1.4 ms       1/32      46%
detect          72 FPS    11.8 ms      31/32      98%
encode          72 FPS     3.2 ms       4/32      62%
```

The runtime could eventually detect bottlenecks:

```text
WARNING

Stage "detect" is limiting pipeline throughput.

Input rate:        118 items/s
Output rate:        72 items/s
Queue utilisation: 97%

Suggested actions:
- allocate additional workers
- increase stage parallelism
- inspect algorithm performance
```

At that point the framework becomes both:

1. a pipeline runtime
2. a performance-engineering tool

---

## Target Use Cases

The framework is intended to be general rather than tied to a single domain.

Potential applications include:

- image processing
- computer vision
- video processing
- signal processing
- robotics
- sensor processing
- scientific instruments
- simulation workflows
- real-time analytics
- data acquisition systems
- HPC workflows

---

## Product Positioning

The goal is **not** to claim that pipeline parallelism or task runtimes are new.

Existing systems such as oneTBB, GStreamer, Kokkos, and other task-graph or runtime systems already solve important parts of this problem.

The opportunity is to provide a framework with a strong emphasis on:

- simple integration
- explicit hardware placement
- predictable execution
- pluggable transport
- bounded communication
- native C++ performance
- minimal concurrency boilerplate
- observability
- portability across execution models

The central product question is:

> **How quickly can a developer build a high-performance native pipeline?**

---

## Intended Developer Experience

Without a framework, a pipeline implementation may require substantial code for:

```text
thread creation
condition variables
mutexes
queues
shutdown handling
CPU affinity
buffer management
error handling
metrics
```

With the framework:

```cpp
pipeline p;

p.stage("load", load);
p.stage("resize", resize);
p.stage("detect", detect);
p.stage("save", save);

p.connect("load", "resize");
p.connect("resize", "detect");
p.connect("detect", "save");

p.run();
```

The difference in developer effort should be immediately visible.

---

# Initial MVP

Version `0.1` should remain deliberately small.

### Planned language

```text
C++20
```

### Initial execution model

```text
pipeline<T>
    |
    +--> stage A
    |
    +--> bounded channel
    |
    +--> stage B
    |
    +--> bounded channel
    |
    +--> stage C
```

### MVP Features

- [ ] arbitrary number of pipeline stages
- [ ] one worker thread per stage
- [ ] bounded queues
- [ ] automatic thread lifecycle management
- [ ] clean pipeline shutdown
- [ ] CPU affinity / core pinning
- [ ] move-based data transfer
- [ ] simple stage API
- [ ] pipeline-level performance statistics
- [ ] basic error propagation
- [ ] example application
- [ ] unit tests

---

## First Demonstration Application

The first example should be intentionally simple and easy to benchmark.

A possible image-processing pipeline:

```text
Load
  |
  v
Grayscale
  |
  v
Blur
  |
  v
Edge Detection
  |
  v
Save
```

The example should demonstrate that the programmer only implements the image-processing operations while the framework handles execution and communication.

---

# Roadmap

## v0.1 — Core Pipeline Runtime

- one thread per stage
- bounded queues
- stage lifecycle
- CPU affinity
- move-based communication
- runtime statistics
- clean startup and shutdown

## v0.2 — Parallel Stages

- multiple workers per stage
- shared thread pools
- stage concurrency configuration
- basic scheduling policies

## v0.3 — Execution Policies

- configurable executors
- alternative stage mappings
- runtime-configurable placement
- external configuration files

## v0.4 — Memory and Zero-Copy

- custom allocators
- reusable buffer pools
- zero-copy paths where possible
- configurable ownership policies

## v0.5 — Multi-Process Execution

- IPC transport
- shared-memory channels
- process-level stage placement

## v0.6 — Accelerator Support

- accelerator-aware stages
- GPU execution hooks
- asynchronous device work
- device-aware buffers

## v0.7 — Distributed Pipelines

- MPI transport
- distributed stage placement
- multi-node pipelines
- distributed monitoring

---

## Long-Term Direction

A mature version of the framework could allow users to define the **logical processing graph once** and independently configure:

```text
What runs?
    -> stage logic

Where does it run?
    -> executor / placement

How does data move?
    -> transport

Where does data live?
    -> memory policy
```

Possible backends could eventually include:

```text
threads
thread pools
processes
shared memory
MPI
accelerators
```

without requiring substantial changes to the application algorithms.

---

## Project Philosophy

The framework should favour:

- simple APIs
- composability
- predictable behaviour
- explicit performance control
- low runtime overhead
- strong ownership semantics
- observability
- incremental complexity

Advanced capabilities should not make the basic case difficult.

The simplest pipeline should remain simple.

---

## Status

**Project stage:** early architecture and design exploration.

Initial focus:

1. define the stage API
2. define channel semantics
3. implement a bounded queue
4. implement one-thread-per-stage execution
5. implement CPU affinity
6. add runtime metrics
7. build the first image-processing example
8. benchmark against an equivalent hand-written implementation

---

## Working Product Statement

> **A lightweight native C++ pipeline framework that lets developers attach algorithms to processing stages while the runtime manages threading, affinity, communication, buffering, and execution.**



---

# Plugin Architecture and GUI-Based Pipeline Studio

A longer-term direction for the framework is to support **dynamically loadable stage implementations** together with a graphical configuration, launch, and performance-analysis environment.

The central idea is that stage logic is implemented independently from the pipeline runtime and compiled into reusable native plugins.

On Linux, the preferred plugin format would be a shared library:

```text
libdecode_stage.so
libresize_stage.so
libfilter_stage.so
libdetect_stage.so
```

Shared libraries are preferred over static `.a` archives for runtime selection because `.so` files can be loaded dynamically, while static libraries are normally resolved during linking.

A future GUI could allow the user to choose which stage plugin is assigned to each pipeline node without recompiling the main pipeline application.

Conceptually:

```text
                    GUI / Pipeline Studio
                              |
                              | produces configuration
                              v
                      pipeline.yaml/json
                              |
                              v
                      Pipeline Runtime
                              |
               +--------------+--------------+
               |              |              |
               v              v              v
            Stage 0         Stage 1         Stage 2
               |              |              |
               v              v              v
          decode.so       filter.so       detect.so
```

---

## Stage Plugin Model

Each stage plugin should implement a well-defined framework interface.

Conceptually, the C++ developer-facing interface could resemble:

```cpp
class stage {
public:
    virtual void initialize() = 0;

    virtual void process(
        const input_buffer& input,
        output_buffer& output
    ) = 0;

    virtual void shutdown() = 0;

    virtual ~stage() = default;
};
```

A user-defined plugin could then implement:

```cpp
class grayscale_stage : public stage {
public:
    void initialize() override {
    }

    void process(
        const input_buffer& input,
        output_buffer& output
    ) override {
        // User algorithm
    }

    void shutdown() override {
    }
};
```

and build it as:

```text
libgrayscale_stage.so
```

The runtime would be responsible for loading the selected stage implementation and connecting it to the pipeline.

---

## Stable Plugin ABI

Although stage implementations may be written using C++, exposing raw C++ class boundaries directly across shared-library interfaces can create ABI compatibility problems across compiler versions, standard-library versions, build options, platforms, and framework versions.

A safer design is to keep the user-facing stage implementation in C++ while exposing a very small stable C-compatible ABI at the plugin boundary.

For example:

```cpp
extern "C" {

stage* pipeline_create_stage();

void pipeline_destroy_stage(stage* ptr);

}
```

An even stronger design could expose opaque handles instead:

```cpp
extern "C" {

pipeline_stage_handle create_stage();

void destroy_stage(pipeline_stage_handle);

}
```

The framework should hide most of this boilerplate from plugin authors. A helper macro could eventually make plugin registration as simple as:

```cpp
PIPELINE_EXPORT_STAGE(grayscale_stage)
```

This keeps plugin development ergonomic while maintaining a controlled ABI boundary.

### Supported Plugin Build and Distribution Models

There are several practical ways to keep dynamically loaded C++ stage plugins compatible with the runtime. The framework can support more than one model because different users will have different deployment requirements.

The important distinction is whether the framework project controls the build environment or whether the application developer controls it.

| Model | Toolchain owner | User convenience | ABI predictability | Typical use |
|---|---|---:|---:|---|
| Prebuilt suite + supported plugin SDK | Framework | High | High | General application developers |
| Reproducible container / SDK / Yocto environment | Framework | Medium | Very high | Reproducible, embedded, industrial deployments |
| Build runtime and plugins together | Application developer | Medium | Very high within that build | HPC, research, unusual platforms, custom toolchains |

#### Model A — Prebuilt Runtime + Supported Plugin SDK

The framework can ship prebuilt components such as:

```text
pipeline-runtime
pipeline-studio
pipeline-sdk/
    include/
    cmake/
    toolchain/
```

and publish the exact supported build environment for plugins, for example:

```text
Compiler: GCC 15.x
C++ standard: C++20
Standard library: libstdc++
Framework plugin ABI: 3
Required ABI-related compile definitions and flags: documented by the SDK
```

An application developer would then only need to implement their stage logic and build it using the supplied SDK.

For example:

```cmake
find_package(PipelineFramework REQUIRED)

pipeline_add_plugin(my_algorithm
    SOURCES my_algorithm_stage.cpp
)
```

The framework could eventually provide a wrapper command such as:

```bash
pipeline-build-plugin ./my_stage
```

so the user does not need to manually reproduce ABI-sensitive compiler settings.

#### Model B — Reproducible Build Environment

The project can also provide a canonical Linux build environment containing the tested compiler, standard library, CMake configuration, framework headers, dependencies, and required build flags.

For normal Linux development, a container-based SDK is likely to be lighter than requiring a full virtual machine:

```bash
docker run \
    -v $PWD:/workspace \
    pipeline/plugin-sdk:1.0 \
    pipeline-build-plugin
```

A Podman-compatible image could provide the same workflow.

For embedded Linux deployments, the framework could later provide a Yocto layer such as:

```text
meta-pipeline-framework/
```

with recipes for components such as:

```text
pipeline-runtime
pipeline-sdk
pipeline-agent
pipeline-plugins
```

QEMU can still be useful for testing complete target images or cross-architecture deployments, but it does not need to be the primary plugin-build mechanism.

#### Model C — Build the Full Suite Together

The framework should also support users who want to compile the runtime, GUI, and application plugins together using their own toolchain.

For example:

```text
application/
|-- pipeline-framework/
|-- plugins/
|   |-- camera/
|   |-- filter/
|   `-- inference/
`-- CMakeLists.txt
```

A normal build could then compile the complete stack:

```bash
cmake -B build
cmake --build build
```

This approach is especially valuable for environments using:

```text
GCC
Clang
Cray
NVHPC
ARM cross-compilers
custom embedded toolchains
HPC system toolchains
```

Because all participating components are built together, the runtime and plugins naturally share the selected compiler, standard library, compile definitions, and ABI-related settings.

### Common Toolchain Requirements

If the runtime and plugins are built using the same compiler version, standard library, ABI configuration, architecture settings, and relevant compile flags, a C++ stage interface can be practical and most toolchain-induced ABI mismatch risks are greatly reduced.

The framework should nevertheless treat compiler compatibility and framework interface compatibility as separate concerns. Two components may use the same compiler and still be incompatible if the stage interface itself has changed.

Therefore every plugin should expose or embed an explicit framework ABI version, for example:

```cpp
extern "C" int pipeline_plugin_abi_version()
{
    return PIPELINE_ABI_VERSION;
}
```

Before creating a stage, the runtime should verify that the plugin ABI is supported.

A plugin descriptor could eventually record additional build information:

```text
plugin_name: OpenCV Resize
plugin_version: 1.2
pipeline_abi: 7
compiler: gcc-15.2
stdlib: libstdc++
architecture: x86_64
build_id: pf-linux-x86_64-gcc15
```

Pipeline Studio could use this information during plugin discovery and show compatibility before launch:

```text
OpenCV Resize
[OK] Framework ABI compatible
[OK] Architecture compatible
[OK] Supported toolchain
```

An incompatible plugin could be rejected before stage instantiation with a useful diagnostic rather than failing unpredictably at runtime.

### Source Compatibility vs Binary Compatibility

The framework should make a clear distinction between two promises:

```text
Source compatibility:
Plugin source written against an older framework API can still be recompiled.

Binary compatibility:
An already-compiled .so from an older framework release can be loaded unchanged.
```

Binary compatibility is a substantially stronger commitment. During early development it would be sensible not to promise a stable binary ABI.

A possible policy is:

```text
0.x    No stable binary ABI guarantee.
1.x    Stable plugin API; plugin recompilation may still be required.
Later  Stable binary plugin ABI where practical and intentionally supported.
```

This allows the framework architecture to evolve without prematurely freezing internal C++ interfaces.

### Recommended Distribution Strategy

The long-term project can officially support all three modes:

```text
                  Pipeline Framework
                          |
              +-----------+-----------+
              |           |           |
              v           v           v
         Source Build   Binary SDK   Container SDK
              |           |           |
        custom/HPC     normal users  reproducible builds
                                      |
                                      v
                                  Yocto layer
                                  embedded targets
```

The C-compatible exported factory functions or `PIPELINE_EXPORT_STAGE(...)` macro remain the stable loading boundary, while the supported build model determines how application developers produce compatible `.so` stage implementations.

---

## Plugin Metadata

Plugins should expose metadata in addition to processing logic.

Useful metadata may include:

```text
plugin name
plugin version
framework ABI version
input type
output type
configuration parameters
thread-safety properties
resource requirements
capabilities
```

For example:

```json
{
  "name": "Gaussian Blur",
  "version": "1.2",
  "framework_api": 1,
  "input": "image/rgb8",
  "output": "image/rgb8",
  "parameters": {
    "kernel_size": {
      "type": "integer",
      "default": 5
    },
    "sigma": {
      "type": "float",
      "default": 1.5
    }
  }
}
```

This metadata could be queried by the runtime or GUI before execution.

---

# Pipeline Studio

A future graphical application could provide a visual interface for configuring, launching, inspecting, and analysing pipelines.

Working name:

> **Pipeline Studio**

The GUI should remain a client of the core runtime rather than being required for the runtime itself.

The same pipeline should still be executable headlessly using a command such as:

```bash
pipeline-run pipeline.yaml
```

This separation would allow the framework to support:

```text
C++ library integration
CLI execution
automated deployment
batch processing
remote execution
GUI-based development
interactive performance analysis
```

all using the same runtime engine.

---

## Visual Pipeline Editor

The GUI could represent pipeline stages as visual nodes:

```text
 +----------+
 | Camera   |
 +----+-----+
      |
      v
 +----------+
 | Decode   |
 | core 0   |
 +----+-----+
      |
      v
 +----------+
 | Resize   |
 | core 1   |
 +----+-----+
      |
      v
 +-------------+
 | Detection   |
 | cores 2-5   |
 +----+--------+
      |
      v
 +----------+
 | Encode   |
 | core 6   |
 +----------+
```

Users could drag available stage plugins into the graph and connect them visually.

A plugin browser might eventually look conceptually like:

```text
AVAILABLE STAGES

Image
 |-- JPEG Decode
 |-- Resize
 |-- Crop
 |-- Grayscale
 |-- Blur
 `-- Edge Detection

AI
 |-- TensorRT Inference
 `-- ONNX Inference

I/O
 |-- Camera
 |-- File Reader
 `-- File Writer
```

Each entry could correspond to an installed `.so` stage plugin.

---

## GUI-Based Stage Configuration

Selecting a stage could expose both algorithm configuration and runtime configuration.

For example:

```text
Stage: Detection

Plugin:
    libyolo_detection.so

Workers:
    2

CPU affinity:
    4,5

Queue size:
    32

Backpressure:
    block
```

Because plugin parameters are described through metadata, the GUI could automatically construct configuration controls.

For a Gaussian blur stage:

```text
Kernel size
[ 5 ]

Sigma
[ 1.5 ]

Workers
[ 2 ]

CPU Affinity
[ 4,5 ]
```

This avoids hard-coding GUI logic for every plugin.

---

# Separation of Pipeline, Execution, and Implementation

Three concepts should remain independent:

```text
Pipeline Description
        |
        v
Execution Configuration
        |
        v
Stage Implementation
```

For example, the logical pipeline could be described independently:

```yaml
pipeline:
  - id: decode
    plugin: libdecode.so

  - id: detect
    plugin: libdetect.so

connections:
  - from: decode
    to: detect
```

Execution placement could then be configured separately:

```yaml
execution:
  decode:
    workers: 1
    cores: [0]

  detect:
    workers: 4
    cores: [2, 3, 4, 5]
```

This would allow users to modify hardware placement and runtime behaviour without modifying algorithm implementations or the logical processing graph.

---

# GUI-Based Launch Control

Pipeline Studio could provide launch controls for:

```text
load configuration
validate plugins
validate stage connections
start pipeline
pause pipeline
stop pipeline
restart pipeline
inspect runtime state
change selected runtime parameters
save configuration
```

Before launch, the GUI could perform validation such as:

```text
Are all referenced plugins available?
Do plugin ABI versions match?
Are input/output types compatible?
Are requested CPU cores available?
Are stage parameters valid?
Are queue capacities valid?
```

This could prevent many runtime configuration errors before execution begins.

---

# Pipeline Studio as a Performance Analysis Tool

Because the runtime owns the stage execution, queues, scheduling, and communication paths, it can collect detailed performance data automatically.

Possible measurements include:

```text
execution time
waiting time
queue depth
input throughput
output throughput
CPU utilisation
items processed
items dropped
end-to-end latency
per-stage latency
backpressure events
worker utilisation
```

The GUI could display live pipeline performance.

For example:

```text
Pipeline Throughput

Camera       120 fps
   |
   v
Decode       120 fps
   |
   v
Resize       120 fps
   |
   v
Detection     72 fps  <-- bottleneck
   |
   v
Encode        72 fps
```

Selecting the bottleneck stage might show:

```text
Detection Stage
-------------------------

Plugin:
libyolo.so

Workers:
1

Average compute:
11.7 ms

Queue:
31 / 32

Input:
118 items/sec

Output:
72 items/sec

CPU:
99%

Status:
BOTTLENECK
```

---

## Visualising Backpressure

Backpressure could also be shown visually:

```text
Decode          Resize           Detection

120 fps ---> 120 fps ---> [############### ] ---> 72 fps
                         Queue 31 / 32
```

This allows a developer to immediately see that the downstream stage cannot keep pace with its producer.

The user could then change:

```text
Workers: 1
```

to:

```text
Workers: 4
```

and rerun the pipeline to observe the effect.

This turns Pipeline Studio into a performance experimentation environment rather than just a graphical launcher.

---

# Proposed Project Layers

The complete project could eventually be organised into three major components:

```text
Pipeline Framework
|
|-- Core Runtime
|   |-- stages
|   |-- channels
|   |-- scheduler
|   |-- affinity
|   |-- buffers
|   `-- metrics
|
|-- Plugin SDK
|   |-- stage interface
|   |-- plugin ABI
|   |-- metadata API
|   `-- plugin helpers
|
`-- Pipeline Studio
    |-- visual graph editor
    |-- plugin browser
    |-- execution configuration
    |-- launch / stop controls
    `-- performance analysis
```

---

# Plugin Security and Trust

Dynamically loading a native `.so` plugin means executing native code inside the pipeline process.

Therefore, future production versions should consider:

```text
plugin ABI validation
plugin version validation
plugin signatures or trusted plugin directories
clear trust boundaries
sandboxing where appropriate
capability declarations
crash isolation
process-based execution for untrusted plugins
```

For early development, plugins can reasonably be treated as trusted application code, but the trust model should remain explicit.

---

# Extended Development Roadmap

The GUI and plugin architecture should be developed only after the core runtime is stable.

A possible progression is:

```text
Core C++ Runtime
        |
        v
Stage Plugin Interface
        |
        v
Shared-Library Plugin Loading
        |
        v
Configuration Format
        |
        v
Headless CLI Launcher
        |
        v
Pipeline Studio
        |
        v
Live Performance Analysis
```

This preserves a clean architecture in which the GUI remains optional.

---

## Extended Product Vision

A mature version of the project could support several different developer workflows from the same runtime:

### Native C++ Library

Developers construct pipelines directly in application code.

### Plugin Runtime

Developers compile algorithms into reusable stage plugins.

### Headless CLI

Pipelines are launched from configuration files:

```bash
pipeline-run pipeline.yaml
```

### Pipeline Studio

Users visually:

- build processing graphs
- choose stage plugins
- assign cores and workers
- configure queues
- launch pipelines
- monitor throughput
- inspect bottlenecks
- compare runtime configurations

The underlying execution engine remains the same in every case.

---

## Expanded Product Statement

> **A native C++ pipeline platform that lets developers focus on stage algorithms while the runtime manages concurrency, affinity, communication, buffering, execution, dynamic stage loading, and performance observability.**
