"""
This module provides a singleton Logger class for logging messages to a file with a specific
format and level.

The Logger class ensures that only one instance of the logger is created and provides methods
for logging messages at different levels (debug, info, warning, error). The log messages are
written to a file specified by LOG_FILE_NAME with a logging level defined by LOGGING_LEVEL.
"""

import logging

LOG_FILE_NAME = "/tmp/das.log"
LOGGING_LEVEL = logging.INFO

# pylint: disable=logging-not-lazy


class Logger:
    """Singleton Logger class for logging messages to a file."""

    __instance = None

    @staticmethod
    def get_instance() -> "Logger":
        """Get the singleton instance of the Logger class."""
        if Logger.__instance is None:
            return Logger()
        return Logger.__instance

    def __init__(self):
        """Initializes the Logger instance and sets up the logging configuration.

        Raises:
            Exception: If an attempt is made to re-instantiate the Logger.
        """

        if Logger.__instance is not None:
            raise RuntimeError("Invalid re-instantiation of Logger")

        logging.basicConfig(
            filename=LOG_FILE_NAME,
            level=LOGGING_LEVEL,
            format="%(asctime)s %(levelname)-8s %(message)s",
            datefmt="%Y-%m-%d %H:%M:%S",
        )
        Logger.__instance = self

    @staticmethod
    def _prefix() -> str:
        """Returns a prefix for log messages.

        Returns:
            str: The prefix for log messages.
        """
        return ""

    def debug(self, msg: str) -> None:
        """Logs a debug message.

        Args:
            msg (str): The message to log.
        """
        logging.debug(self._prefix() + msg)

    def info(self, msg: str) -> None:
        """Logs an info message.

        Args:
            msg (str): The message to log.
        """
        logging.info(self._prefix() + msg)

    def warning(self, msg: str) -> None:
        """Logs a warning message.

        Args:
            msg (str): The message to log.
        """
        logging.warning(self._prefix() + msg)

    def error(self, msg: str) -> None:
        """Logs an error message.

        Args:
            msg (str): The message to log.
        """
        logging.error(self._prefix() + msg)


def logger() -> Logger:
    """Get the singleton instance of the Logger class.

    Returns:
        Logger: The singleton instance of the Logger class.
    """
    return Logger.get_instance()
