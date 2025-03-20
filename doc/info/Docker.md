Docker compose for both adaguc-viewer and adaguc-server
=======================================================

The docker compose file allows you to setup the server and the viewer
simultaneously with one command, it provides a complete environment
where you can view your own files with the adaguc-viewer.

This document provides instructions on how to set this up on your own
workstation:

-   https://github.com/KNMI/adaguc-server\#docker-compose-with-server-and-viewer

On the Preinstalled ADAGUC VM
(\[\[Preinstalled_virtual_environment\]\]) you do:
```
git clone https://github.com/KNMI/adaguc-server
/src/KNMI/adaguc-server/Docker
cd /src/KNMI/adaguc-server/Docker
git checkout . && git pull
bash docker-compose-generate-env.sh \\
-a /data/adaguc-autowms \\
-d /data/adaguc-datasets \\
-f /data/adaguc-autowms \\
-p 443

docker-compose pull
docker-compose build
docker-compose up -d && sleep 10

1.  Scan datasets:
    docker exec -i -t my-adaguc-server /adaguc/scan.sh

<!-- -->

1.  To view logs:
    docker exec -it my-adaguc-server tail -f
    /var/log/adaguc/adaguc-server.log

firefox https://adaguc-virtualbox/
```

On your own environment you can do:
-----------------------------------

```

1.  Prerequisites: git, docker, docker-compose

<!-- -->

1.  Make sure that your local user is part of the docker group. In that
    case you don't need root permissions to run
    sudo groupadd docker
    sudo usermod -aG docker \$USER
    sudo systemctl enable docker
2.  Login and logout to make this work

<!-- -->

1.  Install docker compose
    sudo curl -L
    "https://github.com/docker/compose/releases/download/1.23.1/docker-compose-\$(uname
    ~~s)~~\$(uname -m)" -o /usr/local/bin/docker-compose
    sudo chmod +x /usr/local/bin/docker-compose
    docker-compose --version

<!-- -->

1.  Clone adaguc-server
    git clone https://github.com/KNMI/adaguc-server

#Go to Docker directory
cd adaguc-server/Docker/

#Setup directories
mkdir \$HOME/adaguc-autowms
mkdir \$HOME/adaguc-datasets

1.  Create .env file
    bash docker-compose-generate-env.sh \\
    -a \$HOME/adaguc-autowms \\
    -d \$HOME/adaguc-datasets \\
    -f \$HOME/adaguc-autowms \\
    -p 444

<!-- -->

1.  Look at the output, it should show the url where it is going to
    start

<!-- -->

1.  Now pull and build the containers
    docker-compose pull
    docker-compose build
    docker-compose up -d && sleep 10

<!-- -->

1.  Check if you see three running docker containers
    docker ps

<!-- -->

1.  Scan datasets:
    docker exec -i -t my-adaguc-server /adaguc/scan.sh

<!-- -->

1.  To view logs:
    docker exec -it my-adaguc-server tail -f
    /var/log/adaguc/adaguc-server.log

<!-- -->

1.  To stop you can do:
    docker-compose down

```

Docker for adaguc-server
========================

A prebuilt docker image is available from openearth @ dockerhub
https://hub.docker.com/r/openearth/adaguc-server/. The dockerfile is
located at
https://github.com/KNMI/adaguc-server/blob/master/data/docker/Dockerfile.

To get it running please read the instructions at
https://github.com/KNMI/adaguc-server\#start-the-docker-to-serve-your-data-securely-using-https

Docker for adaguc-viewer
========================

https://github.com/KNMI/adaguc-viewer/blob/master/README.md
