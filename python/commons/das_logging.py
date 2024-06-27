import logging
import sys

DEFAULT_LOG_FILE = "/tmp/das.log"
DEFAULT_LOGGER_NAME = "das_logger"
DEFAULT_LOG_LEVEL = logging.INFO

class DasLogger(logging.Logger):
    """
    Custom Logger class that supports file and stream logging.
    Future work will include a suport for queueing.

    Example:

    from das_logging import log
    log.info("test") -- outputs an info level log message to file /tmp/das.log and also to stderr
    log.das_exception(Exception("test")) -- outputs an error level log message to file /tmp/das.log and also to stderr

    # if you want to have a child logger you can do so like this:

    from das_logging import log
    child = log.getChild("child")
    child.info("test")
    """

    # Export constats from logging
    DEBUG = logging.DEBUG
    INFO = logging.INFO
    WARNING = logging.WARNING
    ERROR = logging.ERROR
    CRITICAL = logging.CRITICAL
    FATAL = logging.FATAL

    def __init__(self, name, level=DEFAULT_LOG_LEVEL):
        """
        Args:
            name: name of the logger
            level: level of the logger
        """
        super().__init__(name, level)

        self.formatter = logging.Formatter(
                fmt="%(asctime)s - %(levelname)-8s - %(message)s",
                datefmt="%Y-%m-%d %H:%M:%S",
        )
        self._setup_stream_handler()
        self._setup_file_handler()

        sys.excepthook = self._uncaught_exc_handler


    def _setup_stream_handler(self):
        stream_handler = logging.StreamHandler(sys.stderr)
        stream_handler.setFormatter(self.formatter)
        self.addHandler(stream_handler)


    def _setup_file_handler(self):
        file_handler = logging.FileHandler(DEFAULT_LOG_FILE)
        file_handler.setFormatter(self.formatter)
        self.addHandler(file_handler)

    def das_exception(self, exception):
        self.exception(exception)

    def _uncaught_exc_handler(self, exception):
        self.das_exception(exception)
        sys.exit(1)


logging.setLoggerClass(DasLogger)
log = logging.getLogger(DEFAULT_LOGGER_NAME)
