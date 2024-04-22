FROM alpine

RUN apk update && apk add git bash curl libc6-compat tar jq && \
    wget https://github.com/cli/cli/releases/download/v2.42.0/gh_2.42.0_linux_amd64.tar.gz && \
    tar -zxvf gh_2.42.0_linux_amd64.tar.gz && \
    chmod +x gh_2.42.0_linux_amd64/bin/gh && \
    mv gh_2.42.0_linux_amd64/bin/gh /usr/local/bin

COPY version-control.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/version-control.sh
