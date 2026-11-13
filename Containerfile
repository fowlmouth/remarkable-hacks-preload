# Build image for rmhacks — official reMarkable SDK (armhf target, device Qt 6).
# Multi-platform: linux/amd64 (x86_64-hosted SDK installer) and linux/arm64
# (aarch64-hosted SDK installer); both produce the same armhf cross toolchain.
# Build: docker build -f Containerfile -t rmhacks-build .

FROM debian:bookworm

ARG TARGETARCH
ARG SDK_VERSION=5.8.203
ARG FW_VERSION=3.28.0.172
ARG SDK_DIR=/opt/codex/rm2/${SDK_VERSION}

ARG VERSION=dev
ARG REVISION=
ARG CREATED=
LABEL org.opencontainers.image.title="rmhacks-build" \
      org.opencontainers.image.description="Build image for rmhacks (reMarkable 2 preload library) with the official reMarkable SDK" \
      org.opencontainers.image.source="https://github.com/fowlmouth/remarkable-hacks-preload" \
      org.opencontainers.image.documentation="https://github.com/fowlmouth/remarkable-hacks-preload/blob/main/README.md" \
      org.opencontainers.image.licenses="MIT" \
      org.opencontainers.image.version="${VERSION}" \
      org.opencontainers.image.revision="${REVISION}" \
      org.opencontainers.image.created="${CREATED}"

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        file \
        make \
        python3 \
        xz-utils \
    && rm -rf /var/lib/apt/lists/*

# The SDK installer is host-arch specific; pick the matching one.
RUN case "${TARGETARCH:-amd64}" in \
        amd64) sdk_host_arch=x86_64 ;; \
        arm64) sdk_host_arch=aarch64 ;; \
        *) echo "unsupported platform: ${TARGETARCH}" >&2; exit 1 ;; \
    esac \
    && curl -fsSL "https://storage.googleapis.com/remarkable-codex-toolchain/${FW_VERSION}/remarkable-production-image-${SDK_VERSION}-rm2-public-${sdk_host_arch}-toolchain.sh" \
        -o /tmp/remarkable-sdk.sh \
    && sh /tmp/remarkable-sdk.sh -y -d "${SDK_DIR}" \
    && rm -f /tmp/remarkable-sdk.sh

# The SDK's Qt tools require a full xx_YY.UTF-8 locale (C.UTF-8 is not
# enough for them and qmake warns); generate en_US.UTF-8.
RUN apt-get update && apt-get install -y --no-install-recommends locales \
    && sed -i 's/^# *en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen \
    && locale-gen \
    && rm -rf /var/lib/apt/lists/*

ENV LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8

# Source this before building:
#   . ${SDK_ENV}
ENV SDK_ENV=${SDK_DIR}/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi

WORKDIR /work
CMD ["/bin/bash"]
