# Build image for the Docker workflow (see README, "Building on a locked-down Windows machine").
# build.cmd builds it once, with no build context:  docker build -t capsule-knob-build - < Dockerfile
#
# git: PlatformIO clones lib_deps that are git URLs (WiFiManager). python:slim has no git.
# platformio + esptool are baked in so builds don't reinstall them every run.
# Versions are deliberately unpinned. To pick up newer ones: docker rmi capsule-knob-build
FROM python:3.12-slim
# apt is switched to HTTPS: some networks intercept plain HTTP (port 80) and apt then fails with
# "Clearsigned file isn't valid, got 'NOSPLIT'". The slim image already ships CA certificates.
RUN sed -i 's|http://deb.debian.org|https://deb.debian.org|g' /etc/apt/sources.list.d/debian.sources \
 && apt-get update \
 && apt-get install -y --no-install-recommends git ca-certificates \
 && rm -rf /var/lib/apt/lists/* \
 && pip install --no-cache-dir platformio esptool
