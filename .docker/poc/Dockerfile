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
    && mkdir -p ${DAS_DIR}/syslibs/usr/lib/${LIB_SUBDIR} ${DAS_DIR}/syslibs/lib/${LIB_SUBDIR} ${DAS_DIR}/syslibs/etc/ssl/certs \
    && cp -f /usr/lib/${LIB_SUBDIR}/libssl.so.3 ${DAS_DIR}/syslibs/usr/lib/${LIB_SUBDIR}/ 2>/dev/null || true \
    && cp -f /usr/lib/${LIB_SUBDIR}/libcrypto.so.3 ${DAS_DIR}/syslibs/usr/lib/${LIB_SUBDIR}/ 2>/dev/null || true \
    && cp -f /lib/${LIB_SUBDIR}/libz.so.1 ${DAS_DIR}/syslibs/lib/${LIB_SUBDIR}/ 2>/dev/null || true \
    && cp -f /usr/lib/${LIB_SUBDIR}/libzstd.so.1 ${DAS_DIR}/syslibs/usr/lib/${LIB_SUBDIR}/ 2>/dev/null || true \
    && cp -f /usr/lib/${LIB_SUBDIR}/libsnappy.so.1 ${DAS_DIR}/syslibs/usr/lib/${LIB_SUBDIR}/ 2>/dev/null || true \
    && cp -f /lib/${LIB_SUBDIR}/libbz2.so.1.0 ${DAS_DIR}/syslibs/lib/${LIB_SUBDIR}/ 2>/dev/null || true \
    && cp -f /lib/${LIB_SUBDIR}/liblzma.so.5 ${DAS_DIR}/syslibs/lib/${LIB_SUBDIR}/ 2>/dev/null || true \
    && cp -f /usr/lib/${LIB_SUBDIR}/libcurl.so.4 ${DAS_DIR}/syslibs/usr/lib/${LIB_SUBDIR}/ 2>/dev/null || true \
    && cp -f /usr/lib/${LIB_SUBDIR}/libevent-2.1.so.7 ${DAS_DIR}/syslibs/usr/lib/${LIB_SUBDIR}/ 2>/dev/null || true \
    && cp -f /etc/ssl/certs/ca-certificates.crt ${DAS_DIR}/syslibs/etc/ssl/certs/ 2>/dev/null || true

FROM debian:12-slim AS tz
RUN apt-get update && apt-get install -y tzdata

FROM gcr.io/distroless/cc-debian12:nonroot AS runner
COPY --from=tz /usr/share/zoneinfo /usr/share/zoneinfo

WORKDIR /opt/das

COPY --chown=nonroot:nonroot --from=builder /usr/local/bin/ /usr/local/bin/
COPY --chown=nonroot:nonroot --from=builder /usr/local/lib/ /usr/local/lib/

COPY --chown=nonroot:nonroot --from=builder /opt/das/lib/ /opt/das/lib/

COPY --from=builder /opt/das/syslibs/usr/lib/ /usr/lib/
COPY --from=builder /opt/das/syslibs/lib/ /lib/
COPY --from=builder /opt/das/syslibs/etc/ssl/certs/ /etc/ssl/certs/

ENV LD_LIBRARY_PATH=/usr/local/lib:/usr/lib/x86_64-linux-gnu:/usr/lib/aarch64-linux-gnu:/lib/x86_64-linux-gnu:/lib/aarch64-linux-gnu

USER nonroot:nonroot

CMD [ "query_broker" ]
