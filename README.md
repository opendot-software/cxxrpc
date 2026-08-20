# C++ Runtime for Parallelism & Concurrency
A set of frameworks, libraries and tools that enable parallel application development with simple programming interface while offering insight into the runtime system. 

## Software Components
  1. [Pipeline](pipeline/README.md)

## Use Cases
  * data processing workloads like image processing, video processing
  * scientific applications needing a simpler way of utilizing cores for a pipelined task

## Branching Strategy

  * **main** branch will always contain the code being merged via release only after necessary verification, validation and test suites run properly
  * **release** branch will always contain code merged from pipeline branch after all pipeline project related verification, validation and test suites are complete
  * **pipeline** branch will contain code after the build for pipeline-develop branch passes
  * **pipeline-develop** branch will be root branch for people working on the pipeline project. Developers will need to branch-off from pipeline-develop if team has more than 1 member. **all current development should happen in **\***-develop branch e.g. 'pipeline-develop'**