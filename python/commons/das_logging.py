import logging
import sys
from types import TracebackType

DEFAULT_LOG_FILE = "/tmp/das.log"
DEFAULT_LOGGER_NAME = "das_logger"
DEFAULT_LOG_LEVEL = logging.WARNING


class DasLogger(logging.Logger):
    """
    Custom Logger class that supports file and stream logging.
    Future work will include a suport for queueing.

    Example:

    from das_logging import log
    log.info("test") -- outputs an info level log message to file /tmp/das.log and also to stderr

    # if you want to have a child logger you can do so like this:

    from das_logging import log
    child = log.getChild("child")
    child.info("test")
    """

    # Export constants from logging
    DEBUG = logging.DEBUG
    INFO = logging.INFO
    WARNING = logging.WARNING
    ERROR = logging.ERROR
    CRITICAL = logging.CRITICAL
    FATAL = logging.FATAL

    def __init__(self, name: str, level: int = DEFAULT_LOG_LEVEL) -> None:
        """
        Args:
            name: name of the logger
            level: level of the logger
        """
        super().__init__(name, level)

        self.formatter = logging.Formatter(
            fmt="%(asctime)s %(levelname)s %(module)s:%(threadName)s %(message)s",
            datefmt="%Y-%m-%d %H:%M:%S",
        )
        self._setup_stream_handler()
        self._setup_file_handler()

        sys.excepthook = self._uncaught_exc_handler

    def _setup_stream_handler(self) -> None:
        """
        Initializes and sets up the stream handler.
        """
        stream_handler = logging.StreamHandler(sys.stderr)
        stream_handler.setFormatter(self.formatter)
        self.addHandler(stream_handler)

    def _setup_file_handler(self):
        """
        Initializes and sets up the file handler.
        """
        file_handler = logging.FileHandler(DEFAULT_LOG_FILE)
        file_handler.setFormatter(self.formatter)
        self.addHandler(file_handler)

    def das_exception(self, msg: str, *args, exc_info: bool = True, **kwargs):
        """
        Convenience method for logging an WARNING with exception information.

        Args:
            msg: message to log
            *args: positional arguments
            exc_info: include exception information, defaulted to True
            **kwargs: keyword arguments
        """
        self.warning(msg, *args, exc_info=exc_info, **kwargs)

    def _uncaught_exc_handler(
        self,
        exc_type: type[BaseException],
        exc_value: BaseException,
        exc_traceback: TracebackType | None,
    ):
        if issubclass(exc_type, KeyboardInterrupt):
            return sys.__excepthook__(exc_type, exc_value, exc_traceback)

        self.error(
            "An uncaught exception occurred: ", exc_info=(exc_type, exc_value, exc_traceback)
        )


logging.setLoggerClass(DasLogger)
log: DasLogger = logging.getLogger(DEFAULT_LOGGER_NAME)  # type: ignore

if __name__ == "__main__":
    log.warning("test")
