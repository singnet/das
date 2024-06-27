import logging
import os
import unittest
from time import sleep

from python.commons.das_logging import DEFAULT_LOG_FILE, DEFAULT_LOGGER_NAME, DasLogger, log

assert isinstance(log, DasLogger) # pyright


class TestDasLogger(unittest.TestCase):

    def test_singleton(self):
        self.assertEqual(logging.getLoggerClass(), DasLogger)
        new_logger = logging.getLogger(DEFAULT_LOGGER_NAME)
        self.assertEqual(new_logger, log)

    def test_das_logger(self):
        self.assertEqual(DEFAULT_LOGGER_NAME, log.name)
        self.assertEqual(logging.INFO, log.level)

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
            sleep(1)
            self.assertTrue(os.path.exists(DEFAULT_LOG_FILE))


    # def test_das_exception(self):
    #     with self.assertLogs(log, level=log.ERROR) as cm:
    #         log.das_exception(Exception("test"))
    #         self.assertEqual(cm.output, ["ERROR:das_commons:Exception: test"])
    #
    # def test_uncaught_exc_handler(self):
    #     with self.assertRaises(Exception):
    #         raise Exception("This should be loogged by sys.excepthook")
    #
    #     # assert that exception was logged
    #     self.assertTrue(os.path.exists(LOG_FILE))
    #
