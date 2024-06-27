# DasLogger

DasLogger is a custom Python logger that supports both file and stream logging. It provides an easy-to-use interface for logging messages to a file and standard error. 

## Features

- Logs messages to both a file and `stderr`
- Customizable log levels
- Automatic handling of uncaught exceptions
- Child logger support

## Installation

To install the module, simply copy the `das_logging.py` file into your project directory.

## Usage

### Basic Usage

```python
from das_logging import log

# Log an info level message
log.info("This is an info message")

# Log an exception with a custom message
log.das_exception(Exception("This is an error message"))
```

### Creating a Child Logger

```python
from das_logging import log

# Create a child logger
child_logger = log.getChild("child")

# Log an info level message with the child logger
child_logger.info("This is an info message from the child logger")
```

## Configuration

The logger can be configured by modifying the default values in the module:

- `DEFAULT_LOG_FILE`: The file where logs will be written (default: `/tmp/das.log`)
- `DEFAULT_LOGGER_NAME`: The name of the logger (default: `das_logger`)
- `DEFAULT_LOG_LEVEL`: The default log level (default: `logging.INFO`)

## Example

Here's a complete example demonstrating the usage of DasLogger:

```python
from das_logging import log

# Log messages of various levels
log.debug("This is a debug message")
log.info("This is an info message")
log.warning("This is a warning message")
log.error("This is an error message")
log.critical("This is a critical message")

# Log an exception
try:
    raise ValueError("An example exception")
except Exception as e:
    log.das_exception(e)

# Create and use a child logger
child = log.getChild("child")
child.info("This is an info message from the child logger")
```

## Exception Handling

DasLogger automatically handles uncaught exceptions by logging them and then exiting the program with a status code of 1. This is achieved by setting `sys.excepthook` to the logger's custom exception handler.

>Note: The automatic handling of uncaught exceptions will not work in iPython shells (including Jupyter notebooks).


