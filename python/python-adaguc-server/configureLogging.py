import sys


def configureLogging(logging):
  root = logging.getLogger()
  root.setLevel(logging.DEBUG)
  handler = logging.StreamHandler(sys.stdout)
  handler.setLevel(logging.DEBUG)
  formatter = logging.Formatter(
      'applicationlog %(asctime)s - %(name)s - %(levelname)s - %(message)s')
  handler.setFormatter(formatter)
  root.addHandler(handler)
