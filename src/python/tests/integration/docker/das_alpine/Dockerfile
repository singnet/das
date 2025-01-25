FROM docker:20.10.21-dind-alpine3.16
RUN apk add --update --no-cache \
    python3 \
    git \
    bash \
    curl \
    && ln -sf /usr/bin/python3 /usr/bin/python \
    && /usr/bin/python -m ensurepip
WORKDIR /das
RUN git clone https://github.com/singnet/das-toolbox.git
RUN pip3 install --no-cache --upgrade \
    pip \
    setuptools \
    hyperon-das \
    && pip3 install -r das-toolbox/src/requirements.txt
COPY config.json /root/.das/config.json
COPY entrypoint.sh entrypoint.sh
COPY animals.metta animals.metta
EXPOSE 8080
CMD ["/bin/bash","-c","./entrypoint.sh"]