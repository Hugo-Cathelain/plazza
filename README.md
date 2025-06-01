# The Plazza

The `Plazza` is a `multithreaded simulation` of a pizzeria designed to efficiently manage the pizza-making process. The system
implements a `concurrent architecture` where a `Reception` handles incoming orders and distributes them to multiple `Kitchen instances`. Each `Kitchen` maintains its own `inventory` of ingredients and employs several `Cook threads` that prepare pizzas simultaneously.

# Installation/Setup

This project is compiled via `Makefile` with two build options:
- `make` or `make re`: Standard compilation for the base simulation
- `make bonus`: Enables additional features through conditional compilation (PLAZZA_BONUS flag), activating a graphical visualization of the pizzeria

# Basic Usage

once compiled with a `make` you can launch our project with the binary `./plazza`
with 3 flags for the `cooking speed multiplier`, the amount of `cooks per kitchen` and `restock time` the amount of time it takes to restock 1 of every item.

```bash
./plazza <multiplier> <cooks_per_kitchen> <restock_time_ms>
```

# Testing

In addition to the two make options for program execution, we provide a third one for `unit tests`. This can be compiled with: `make tests_run`

This option runs 40 comprehensive tests across our codebase, focusing on:

1. **Parsing**: Tests for command-line input parsing and validation
2. **PizzaFactory**: Tests our pizza creation system which is derived from our `Singleton` pattern
3. **Singleton**: Tests our implementation of the Singleton design pattern

The test suite first verifies the PizzaFactory functionality and available commands, then validates our Singleton implementation by creating a `mock class`.

# Architecture

We can seperate the architecture into 3 parts. `IPC`, `Encapsulation` and `Dialogue Logic`


## IPC
In computer science, `interprocess communication (IPC)` is the sharing of data between running processes in a computer system.<bre>
The IPC used in plazza is a `Named pipes` also called a `FIFO`.

- **interface Definition**<br>
The abstract interface: `IIPCChannel` defines the proper communication channels with the base being these 4 key methods: `Open()`, `Close`, `SendMessage()`, `PollMessage()`

- **Named Pipes Implentation**<br>
The `pipe` class implements the `IIPCChannel` using `Named Pipe (FIFO)` mechanism. 2 predefined pipes are used to communicate between `reception` and `kitchen`:<br>
```cpp
#define KITCHEN_TO_RECEPTION_PIPE "/tmp/plazza_kitchen_to_reception_pipe"
#define RECEPTION_TO_KITCHEN_PIPE "/tmp/plazza_reception_to_kitchen_pipe"
```
- **Message Serialization**<br>
Messages are serialized/deserialized using the `Message` class.
They utlize a type-safe variant system for the different types of message:<br><br>
    * `Closed`: Notification that a kitchen is closing
    * `Order`: New pizza orders from Reception to Kitchen
    * `Status`: Kitchen status updates sent to Reception
    * `RequestStatus`: Requests for status updates
    * `CookedPizza`: Notification that a pizza is ready

- **functionality**

    1. **Pipe Creation, Connection**
        * Pipes are created using `mkfifo()` in the `Pipe::Open()` method
        * Each pipe has a designated direction `READ_ONLY or WRITE_ONLY`
        * The Reception and Kitchen processes connect to the same named pipes but with opposite modes
    2. **Non-blocking I/O**
        * utilizes flag `O_NONBLOCK` allows the `PollMessage()` method to check for messages without hanging
    3. **Message Protocol**
        * Messages are formatted with a `4-byte` length header followed by the serialized payload. The payload itself is packed with a `message type byte` followed by the message contents. The `PollMessage()` method accumulates partial reads in a buffer until a complete message is received
    4. **Bidirectional Communication FLow**
        * The `Reception` sends orders to `Kitchens` via `one pipe` Kitchens respond with status updates and `cooked pizza notifications` via the `other pipe`. This separation of concerns allows for clear, organized communication between processes. i.e:
        ```cpp
        #define KITCHEN_TO_RECEPTION_PIPE "/tmp/plazza_kitchen_to_reception_pipe"
        ```
        in **kitchen**:
        ```cpp
        m_toReception = std::make_unique<Pipe>(std::string(KITCHEN_TO_RECEPTION_PIPE), Pipe::OpenMode::WRITE_ONLY);
        ```
        in **reception**:
        ```cpp
        m_pipe(std::make_unique<Pipe>(KITCHEN_TO_RECEPTION_PIPE, Pipe::OpenMode::READ_ONLY))
        ```

## Encapsulation

this project ecapsluates multiplpe encapsulation of methods and variable, creating in total, 4 classes:
- **Process**: Encapsulates process creation and management using the `fork()` system call. `Kitchen` is the only class that inherits its behavior, which handles:
    * Creating new `Kitchen processes` when needed
    * Managing `parent-child` relationships between `Reception` and `Kitchens`
    * Handling `process exit codes` and `status information`

- **Thread**: Encapsulates `thread` creation and management using the `pthread` library. exculsively inherited by the `cook` class. its primarly used for:
    * Creating `Cook threads` within each `Kitchen process`
    * Managing `thread lifecycle` (*creation, execution, joining*)
    * Providing a standardized interface for `thread execution`
    * Implementing background tasks like ingredient restocking

- **Mutex**: Encapsulates `mutual exclusion primitives` that prevent `multiple threads` from accessing shared resources simultaneously. No class inherits `Mutex` but all of them contains it as a `variable`, utlizing it for various means. Such as:
    * Protecting access to the shared ingredients `inventory`
    * Ensuring thread-safe `order queue operations`
    * Protecting internal kitchen state during `status updates`
    * Synchronizing access to the cooked pizzas collection

- **Condvar**: stands for `conditional variable`, this `class` encapsulates the `standard condition variable functionality`. It provides a mechanism for `threads` to block until notified by another thread that a condition has been met. once again no class inherits Condvar. Condvar is utilized in 3 instances:
   * Implementing `producer-consumer` patterns between `Reception` and `Cooks`
   * Allowing `Cooks` to wait for new orders without consuming `CPU`
   * Signaling when ingredients have been restocked


## Dialogue Logic

# Load Balancer

# bonus Feature