FROM trueagi/das:22.04-builder AS builder

ENV DAS_DIR=/opt/das
ENV BIN_DIR=${DAS_DIR}/bin
ENV LIB_DIR=${DAS_DIR}/lib

WORKDIR ${DAS_DIR}

COPY ./bin/ ${BIN_DIR}/
COPY ./lib/ ${LIB_DIR}/

# Strip debug symbols and copy only necessary binaries to /usr/local/bin
RUN if [ "$(uname -m)" = "aarch64" ] || [ "$(uname -m)" = "arm64" ]; then \
        ARCH="arm64"; \
    else \
        ARCH="amd64"; \
    fi \
    && rm -rf ${BIN_DIR}/${ARCH}/*_test \
    && strip ${BIN_DIR}/${ARCH}/* \
    && cp -r ${BIN_DIR}/${ARCH}/* /usr/local/bin/ \
    && rm -rf ${BIN_DIR} \
    && chmod +x /usr/local/bin/ -R

# Copy necessary libs to /opt/das/lib
RUN if [ "$(uname -m)" = "aarch64" ] || [ "$(uname -m)" = "arm64" ]; then \
        ARCH="arm64"; \
    else \
        ARCH="amd64"; \
    fi \
    && cp -r ${LIB_DIR}/${ARCH}/* ${LIB_DIR}/ \
    && rm -rf ${LIB_DIR}/${ARCH}/

# Collect system libraries to a common location for easier copying
RUN if [ "$(uname -m)" = "aarch64" ] || [ "$(uname -m)" = "arm64" ]; then \
        LIB_SUBDIR="aarch64-linux-gnu"; \
    else \
        LIB_SUBDIR="x86_64-linux-gnu"; \
    fi \
    && mkdir -p ${DAS_DIR}/syslibs/usr/lib/${LIB_SUBDIR} \
                ${DAS_DIR}/syslibs/lib/${LIB_SUBDIR} \
                ${DAS_DIR}/syslibs/etc/ssl/certs \
    # --- usr/lib/ libraries ---
    && USR_LIBS=" \
        libssl.so.3 \
        libcrypto.so.3 \
        libzstd.so.1 \
        libsnappy.so.1 \
        libcurl.so.4 \
        libevent-2.1.so.7 \
        # Libs from here and on are related to the libpqxx lib, used by the database-adapter.
        libgssapi_krb5.so.2 \
        libkrb5.so.3 \
        libk5crypto.so.3 \
        libcom_err.so.2 \
        libkrb5support.so.0 \
        liblber-2.5.so.0 \
        libkeyutils.so.1 \
        libp11-kit.so.0 \
        libidn2.so.0 \
        libunistring.so.2 \
        libtasn1.so.6 \
        libnettle.so.8 \
        libhogweed.so.6 \
        libgmp.so.10 \
        libffi.so.8 \
        libldap-2.5.so.0 \
        libsasl2.so.2 \
        libgnutls.so.30 \
    " \
    # lib/ folder libraries ---
    && LIB_LIBS=" \
        libz.so.1 \
        libbz2.so.1.0 \
        liblzma.so.5 \
    " \
    && for lib in $USR_LIBS; do \
        cp -f /usr/lib/${LIB_SUBDIR}/$lib ${DAS_DIR}/syslibs/usr/lib/${LIB_SUBDIR}/ 2>/dev/null || true; \
    done \
    && for lib in $LIB_LIBS; do \
        cp -f /lib/${LIB_SUBDIR}/$lib ${DAS_DIR}/syslibs/lib/${LIB_SUBDIR}/ 2>/dev/null || true; \
    done \
    && cp -f /etc/ssl/certs/ca-certificates.crt ${DAS_DIR}/syslibs/etc/ssl/certs/ 2>/dev/null || true

FROM gcr.io/distroless/cc-debian12:nonroot AS runner

WORKDIR /opt/das

COPY --chown=nonroot:nonroot --from=builder /usr/local/bin/ /usr/local/bin/
COPY --chown=nonroot:nonroot --from=builder /usr/local/lib/ /usr/local/lib/

COPY --chown=nonroot:nonroot --from=builder /opt/das/lib/ /opt/das/lib/

COPY --from=builder /opt/das/syslibs/usr/lib/ /usr/lib/
COPY --from=builder /opt/das/syslibs/lib/ /lib/
COPY --from=builder /opt/das/syslibs/etc/ssl/certs/ /etc/ssl/certs/

ENV LD_LIBRARY_PATH=/opt/das/lib:/usr/local/lib:/usr/lib/x86_64-linux-gnu:/usr/lib/aarch64-linux-gnu:/lib/x86_64-linux-gnu:/lib/aarch64-linux-gnu

USER nonroot:nonroot

CMD [ "busnode" ]
