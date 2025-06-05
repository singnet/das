import logging

log_format = "%(asctime)s | [%(levelname)s] | %(message)s"
date_format = "%Y-%m-%d %H:%M:%S"

log = logging.getLogger("python_client")
log.setLevel(logging.INFO)

handler = logging.StreamHandler()
handler.setFormatter(logging.Formatter(fmt=log_format, datefmt=date_format))
log.addHandler(handler)
