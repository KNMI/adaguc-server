import os
def mkdir_p(directory):
  if not os.path.exists(directory):
    os.makedirs(directory)
