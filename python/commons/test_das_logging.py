import logging
import sys
import unittest

from python.commons.das_logging import DEFAULT_LOG_LEVEL, DEFAULT_LOGGER_NAME, DasLogger, log

assert isinstance(log, DasLogger)  # pyright


class TestDasLogger(unittest.TestCase):
    def test_singleton(self):
        self.assertEqual(logging.getLoggerClass(), DasLogger)
        new_logger = logging.getLogger(DEFAULT_LOGGER_NAME)
        self.assertEqual(new_logger, log)

    def test_das_logger(self):
        self.assertEqual(DEFAULT_LOGGER_NAME, log.name)
        self.assertEqual(DEFAULT_LOG_LEVEL, log.level)

    def test_hashmap(self):
        self.assertEqual(logging.DEBUG, log.DEBUG)
        self.assertEqual(logging.INFO, log.INFO)
        self.assertEqual(logging.WARNING, log.WARNING)
        self.assertEqual(logging.ERROR, log.ERROR)
        self.assertEqual(logging.CRITICAL, log.CRITICAL)
        self.assertEqual(logging.FATAL, log.FATAL)

    def test_log_logs(self):
        with self.assertLogs(log, log.INFO) as cm:
            log.info("test")
            self.assertEqual(cm.output, ["INFO:das_logger:test"])

    def test_exception(self):
        """Test that log.exception logs and it's level is set correctly"""
        with self.assertLogs(log, level=log.WARNING) as cm:
            try:
                raise Exception("test")
            except Exception as e:
                log.exception(e)

            self.assertTrue(cm.output[0].startswith("ERROR:das_logger"))

    def test_das_exception(self):
        """Test that das_exception logs and it's level is set correctly"""
        with self.assertLogs(log, level=log.WARNING) as cm:
            try:
                raise Exception("test")
            except Exception as e:
                log.das_exception(e)  # type: ignore

            self.assertTrue(cm.output[0].startswith("WARNING:das_logger"))

    def test_sys_excepthook(self):
        """Test that sys.excepthook is set correctly"""

        self.assertEqual(sys.excepthook, log._uncaught_exc_handler)

    def test_uncaught_exc_handler(self):
        """Test that uncaught exception handler logs and it's level is set correctly"""
        with self.assertLogs(log, level=log.WARNING) as cm:
            exc = Exception("test")
            log._uncaught_exc_handler(type(exc), exc, None)
            self.assertTrue(
                cm.output[0].startswith("ERROR:das_logger:An uncaught exception occurred")
            )
