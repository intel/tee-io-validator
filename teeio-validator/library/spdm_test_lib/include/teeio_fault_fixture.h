/** Copyright 2026 Intel. All rights reserved. License: BSD-3-Clause. */
#ifndef TEEIO_FAULT_FIXTURE_H
#define TEEIO_FAULT_FIXTURE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "internal/libspdm_common_lib.h"
#include "teeio_fault_injection.h"

/* Storage is appended to the owning libspdm context allocation. The root
 * setter BORROWS it: never free it while that context is alive. */
#define TEEIO_FAULT_ROOT_CAPACITY 4096

static inline bool teeio_fault_fixture_root_required(bool is_fault_group)
{
  return is_fault_group &&
    (teeio_fault_driver() == TEEIO_FAULT_DRIVER_KEY_EXCHANGE_DUPLICATE ||
     teeio_fault_driver() == TEEIO_FAULT_DRIVER_KEY_EXCHANGE_CACHED_CERT ||
     teeio_fault_driver() == TEEIO_FAULT_DRIVER_FINISH_SIGNATURE);
}

static inline bool teeio_fault_fixture_path(const char *name, char **path)
{
  const char *root = getenv("TEEIO_FAULT_FIXTURE_DIR");
  struct stat info;
  char *base, *candidate, *resolved;
  size_t length;
  *path = NULL;
  if (root == NULL || root[0] != '/' || stat(root, &info) != 0 ||
      !S_ISDIR(info.st_mode)) {
    return false;
  }
  base = realpath(root, NULL);
  if (base == NULL) {
    return false;
  }
  length = strlen(base);
  candidate = malloc(length + strlen(name) + 2);
  if (candidate == NULL) {
    free(base);
    return false;
  }
  sprintf(candidate, "%s/%s", base, name);
  resolved = realpath(candidate, NULL);
  free(candidate);
  /* No path escapes, including symlink escapes from an explicit fixture set. */
  if (resolved == NULL || strncmp(base, resolved, length) != 0 ||
      (length != 1 && resolved[length] != '/') ||
      stat(resolved, &info) != 0 || !S_ISREG(info.st_mode)) {
    free(resolved);
    free(base);
    return false;
  }
  free(base);
  *path = resolved;
  return true;
}

/* The upstream PEM signer reads cwd-relative files. Require that BOTH sample
 * reads resolve to the explicitly selected fixtures; never chdir or fall back
 * to an unrelated key in the artifact directory. */
static inline bool teeio_fault_fixture_sample_paths(void)
{
  const char *names[] = {"ecp384/bundle_requester.certchain.der",
                         "ecp384/end_requester.key"};
  size_t index;
  for (index = 0; index < sizeof(names) / sizeof(names[0]); index++) {
    char *expected = NULL;
    char *actual = realpath(names[index], NULL);
    bool valid = teeio_fault_fixture_path(names[index], &expected) &&
                 actual != NULL && strcmp(expected, actual) == 0;
    free(expected);
    free(actual);
    if (!valid) {
      return false;
    }
  }
  return true;
}

/* Exactly one definite-length DER SEQUENCE spanning the whole buffer. Content
 * pinning is the campaign's responsibility. */
static inline bool teeio_fault_fixture_is_single_der(const uint8_t *der, size_t size)
{
  size_t header, length, count, index;
  if (size < 2 || der[0] != 0x30) {
    return false;
  }
  if (der[1] < 0x80) {
    header = 2;
    length = der[1];
  } else {
    count = der[1] & 0x7f;
    if (count == 0 || count > sizeof(uint32_t) || size < 2 + count) {
      return false;
    }
    length = 0;
    for (index = 0; index < count; index++) {
      length = (length << 8) | der[2 + index];
    }
    header = 2 + count;
  }
  return length == size - header;
}

static inline bool teeio_fault_fixture_install_root(void *spdm, void *storage)
{
  char *path;
  FILE *stream;
  size_t size;
  bool valid;
  libspdm_data_parameter_t parameter = {0};
  if (!teeio_fault_fixture_path("responder_root.der", &path)) {
    return false;
  }
  stream = fopen(path, "rb");
  free(path);
  if (stream == NULL) {
    return false;
  }
  size = fread(storage, 1, TEEIO_FAULT_ROOT_CAPACITY, stream);
  /* A full buffer does not set EOF; probe one byte to reject larger files. */
  valid = !ferror(stream) &&
          (size < TEEIO_FAULT_ROOT_CAPACITY ? feof(stream) :
           (fgetc(stream) == EOF && !ferror(stream))) &&
          teeio_fault_fixture_is_single_der(storage, size);
  fclose(stream);
  if (!valid) {
    return false;
  }
  parameter.location = LIBSPDM_DATA_LOCATION_LOCAL;
  return libspdm_set_data(spdm, LIBSPDM_DATA_PEER_PUBLIC_ROOT_CERT,
                           &parameter, storage, size) == LIBSPDM_STATUS_SUCCESS;
}

#endif