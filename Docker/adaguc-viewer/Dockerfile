FROM openearth/adaguc-viewer
# Install adaguc-viewer from context
COPY config.js /var/www/html/adaguc-viewer
COPY index.html /var/www/html/adaguc-viewer
COPY index.html /var/www/html/adaguc-viewer/index.php

ENV ADAGUCSERVICES_AUTOWMS="http://localhost:8090/adaguc-services/autowms?"
