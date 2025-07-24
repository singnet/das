import logging
import sys

RESET = "\033[0m"
COLOR_MAP = {
    logging.DEBUG: "\033[33m",      # yellow
    logging.INFO: "\033[32m",       # green
    logging.WARNING: "\033[35m",    # magenta
    logging.ERROR: "\033[31m",      # red
    logging.CRITICAL: "\033[1;31m"  # bold red
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
log.setLevel(logging.INFO)

handler = logging.StreamHandler(sys.stdout)
handler.setFormatter(ColoredFormatter(fmt=log_format, datefmt=date_format))
log.addHandler(handler)
