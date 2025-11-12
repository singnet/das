FROM trueagi/das:22.04-builder

WORKDIR /opt/das

COPY ./bin .

COPY ./lib .

RUN ARCH=$(uname -m) && \
    if [ "$ARCH" = "x86_64" ]; then \
        cp -r ./amd64/* /usr/local/bin/; \
    elif [ "$ARCH" = "aarch64" ]; then \
        cp -r ./arm64/* /usr/local/bin/; \
    else \
        echo "Unsupported architecture: $ARCH" && exit 1; \
    fi

RUN chmod +x /usr/local/bin/ -R

RUN ldconfig

CMD [ "bash" ]
