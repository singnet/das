FROM trueagi/das:22.04-builder

COPY  ./bin/ /usr/local/bin

RUN chmod +x /usr/local/bin/ -R

COPY ./lib /usr/local/lib

RUN ldconfig

CMD [ "bash" ]
