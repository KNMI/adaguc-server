"""
 This Python script runs CGI scripts without an webserver.
 Created by Maarten Plieger - 20151104
"""

import sys
from subprocess import PIPE, Popen, STDOUT
from threading  import Thread
import os
import io
import errno
import time
import chardet

class CGIRunner:
  
  def __init__(self):
    self.headers = ""
    self.headersSent = False
    self.foundLF = False
    
  def startProcess(self,cmds,callback=None,env = None,bufsize=0):

    try:
        from Queue import Queue, Empty
    except ImportError:
        from queue import Queue, Empty  # python 3.x

    ON_POSIX = 'posix' in sys.builtin_module_names

    def enqueue_output(out, queue):
        for line in iter(out.readline, b''):
            queue.put(line)
        out.close()

    #print cmds
    #print env
    
    
    p = Popen(cmds, stdout=PIPE, stderr=STDOUT,bufsize=bufsize, close_fds=ON_POSIX,env=env)
    q = Queue()
    t = Thread(target=enqueue_output, args=(p.stdout, q))
    t.daemon = True # thread dies with the program
    t.start()
    
    #http://stackoverflow.com/questions/156360/get-all-items-from-thread-queue
    # read line without blocking
    while True:
      try:
        #line = q.get_nowait() #<-- Causes a lot of CPU usage!
        line = q.get(timeout=.01)
        if(callback != None):
          callback(line)
      except Empty:
          if(t.isAlive() == False):
            break;
    
    """ Somehow sometimes stuff is still in que """
    while True:
      try:  
        line = q.get(timeout=.1)
        if(callback != None):
          callback(line)
      except Empty:
          if(t.isAlive() == False):
            break;
  
    return p.wait()  

  def _filterHeader(self,_message,writefunction):
      
      if self.headersSent == False:
        self.headers = self.headers + _message.decode()
        message = bytearray(_message)
        endHeaderIndex = 0
        for j in range(len(message)):
          if message[j] == 10 :
            if self.foundLF == False:
              self.foundLF = True
              #print "LF Found"
              continue
          elif self.foundLF == True and message[j] != 13:
            self.foundLF = False
            #print "Sorry, not LF Found"
            continue
          
          if(self.foundLF == True):  
            if message[j] == 10 :
              #print "Second LF Found"
              self.headersSent = True;
              endHeaderIndex = j+2;
              #print "HEADER FOUND"
              #print message[:endHeaderIndex]
              writefunction(message[endHeaderIndex:])
              
              break;
      else:
        writefunction(_message)
    
  """
    Run the CGI script with specified URL and environment. Stdout is captured and put in a BytesIO object provided in output
  """
  def run(self,cmds,url,output,env = [], path = None, isCGI = True):
    #output = subprocess.Popen(["../../bin/adagucserver", "myarg"], stdout=subprocess.PIPE, env=adagucenv).communicate()[0]
    self.headersSent = False
    self.foundLF = False
    self.headers = ""

    
    if isCGI is False:
      self.headersSent = True
    
    def writefunction(data):
        output.write(data)
    
    def monitor1(_message):
      self._filterHeader(_message,writefunction)
    
    localenv = {}#os.environ.copy()
    if url != None:
      localenv['QUERY_STRING']=url
    else :
       localenv['QUERY_STRING']=""
      
    
    if path!=None:
      localenv['SCRIPT_NAME']="/myscriptname";
      #SCRIPT_NAME [/cgi-bin/autoresource.cgi], REQUEST_URI [/cgi-bin/autoresource.cgi/opendap/clipc/combinetest/wcs_nc2.nc.das]
      localenv['REQUEST_URI']="/myscriptname/" + path
      
    localenv.update(env)
    status = self.startProcess(cmds,monitor1,localenv,bufsize=8192)
    output.flush()
    
    return status, self.headers
