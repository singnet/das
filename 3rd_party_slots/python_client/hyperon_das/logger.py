import logging
import sys
from os import environ

RESET = "\033[0m"
COLOR_MAP = {
    logging.DEBUG: "\033[33m",  # yellow
    logging.INFO: "\033[32m",  # green
    logging.WARNING: "\033[35m",  # magenta
    logging.ERROR: "\033[31m",  # red
    logging.CRITICAL: "\033[1;31m",  # bold red
}


class ColoredFormatter(logging.Formatter):
    def format(self, record):
        color = COLOR_MAP.get(record.levelno, RESET)
        record.levelname = f"{color}{record.levelname}{RESET}"
        msg = super().format(record)
        return msg


log_format = "%(asctime)s | [%(levelname)s] | %(message)s"
date_format = "%Y-%m-%d %H:%M:%S"

log = logging.getLogger("python_client")

if level := environ.get("LOG_LEVEL"):
    if level == 'DEBUG':
        level = logging.DEBUG
    elif level == 'INFO':
        level = logging.INFO
    elif level == 'WARNING':
        level = logging.WARNING
    elif level == 'ERROR':
        level = logging.ERROR
    elif level == 'CRITICAL':
        level = logging.CRITICAL
    else:
        level = logging.INFO
else:
    level = logging.INFO

log.setLevel(level)

handler = logging.StreamHandler(sys.stdout)
handler.setFormatter(ColoredFormatter(fmt=log_format, datefmt=date_format))
log.addHandler(handler)
