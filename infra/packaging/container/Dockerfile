FROM ubuntu:24.04 AS build

RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    build-essential \
    git \
    make \
    zsh \
    ca-certificates \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY services/vfo ./services/vfo

RUN make -C services/vfo all

FROM ubuntu:24.04

RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    bash \
    ffmpeg \
    mkvtoolnix \
    ca-certificates \
  && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /usr/local/bin/vfo_conf_folder/presets

COPY --from=build /workspace/services/vfo/bin/vfo /usr/local/bin/vfo
COPY --from=build /workspace/services/vfo/actions/*.sh /usr/local/bin/
COPY --from=build /workspace/services/vfo/src/vfo_config.conf /usr/local/bin/vfo_conf_folder/vfo_config.conf
COPY --from=build /workspace/services/vfo/presets/ /usr/local/bin/vfo_conf_folder/presets/

RUN chmod +x /usr/local/bin/vfo /usr/local/bin/*.sh

ENTRYPOINT ["vfo"]
CMD ["--help"]
