import sys
import logging
import os
from logging.handlers import TimedRotatingFileHandler

def get_logger(level):
    if not os.path.exists('log'):
        os.makedirs('log')
    logger = logging.getLogger('default')
    logger.setLevel(level)
    if not logger.handlers:
        # logging format
        fmt = logging.Formatter("[%(asctime)s] [%(levelname)s] %(message)s", "%Y-%m-%d %H:%M:%S.%f")

        # filehandler
        fh = TimedRotatingFileHandler("./log/log.txt", when='D',interval=1,backupCount=256)
        fh.setFormatter(fmt)
        fh.setLevel(level)
        logger.addHandler(fh)

        # streamhandler
        ch= logging.StreamHandler()
        ch.setFormatter(fmt)
        #ch.setLevel(logging.INFO)
        ch.setLevel(level)
        #logger.addHandler(ch)

    return logger

logger = get_logger(logging.DEBUG)
