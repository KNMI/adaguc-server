FROM openearth/adaguc-server
USER root
WORKDIR /adaguc/adaguc-server-master/python/lib/
ENTRYPOINT [ "bash", "-c", "python3 /adaguc/adaguc-server-master/python/examples/runautowms/run.py && ls result.png" ] 