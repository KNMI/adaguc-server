FROM debian:bookworm-slim AS build

FROM docker.tyk.io/tyk-gateway/tyk-gateway:v5.12.0 




COPY apps /opt/tyk-gateway/apps
COPY policies/policies.json /opt/tyk-gateway/policies/policies.json
COPY tyk.standalone.conf /opt/tyk-gateway/tyk.conf
COPY certs /opt/tyk-gateway/certs
COPY middleware /opt/tyk-gateway/middleware
COPY --chmod=a+x starttyk.sh /bin/

# needed for sed and bash
COPY --from=build /bin/* /bin
COPY --from=build /usr/bin/sed /usr/bin/sed
COPY --from=build /lib/x86_64-linux-gnu/ /lib/x86_64-linux-gnu/



# ENV TYK_GW_SECRET=""
# ENV TYK_GW_STORAGE_HOST="localhost"



EXPOSE 8080

ENTRYPOINT [ "/bin/starttyk.sh" ]