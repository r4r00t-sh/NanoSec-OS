/*
 * NanoSec OS - Pipes (Inter-Process Communication)
 * ==================================================
 * Unidirectional data channels between processes
 */

#include "../kernel.h"
#include "../proc/process.h"

/* Pipe buffer size */
#define PIPE_BUFFER_SIZE 4096

/* Pipe structure */
typedef struct {
  uint8_t buffer[PIPE_BUFFER_SIZE];
  uint16_t read_pos;
  uint16_t write_pos;
  uint16_t count;
  int read_end_open;
  int write_end_open;
  int in_use;
} pipe_t;

#define MAX_PIPES 32
static pipe_t pipes[MAX_PIPES];

/*
 * Initialize pipe subsystem
 */
void pipe_init(void) {
  for (int i = 0; i < MAX_PIPES; i++) {
    pipes[i].in_use = 0;
  }
}

/*
 * Create a pipe
 * Returns: pipe ID on success, -1 on failure
 * read_fd and write_fd are set to the file descriptors
 */
int pipe_create(int *read_fd, int *write_fd) {
  for (int i = 0; i < MAX_PIPES; i++) {
    if (!pipes[i].in_use) {
      pipes[i].in_use = 1;
      pipes[i].read_pos = 0;
      pipes[i].write_pos = 0;
      pipes[i].count = 0;
      pipes[i].read_end_open = 1;
      pipes[i].write_end_open = 1;

      /* Return pipe ID (simplified - real OS would use file descriptors) */
      *read_fd = i * 2;      /* Even = read end */
      *write_fd = i * 2 + 1; /* Odd = write end */

      return i;
    }
  }
  return -1;
}

/*
 * Get pipe from file descriptor
 */
static pipe_t *pipe_from_fd(int fd, int *is_write) {
  int pipe_id = fd / 2;
  *is_write = fd & 1;

  if (pipe_id < 0 || pipe_id >= MAX_PIPES)
    return NULL;
  if (!pipes[pipe_id].in_use)
    return NULL;

  return &pipes[pipe_id];
}

/*
 * Write to pipe
 */
int pipe_write(int fd, const void *data, size_t len) {
  int is_write;
  pipe_t *pipe = pipe_from_fd(fd, &is_write);

  if (!pipe || !is_write)
    return -1;
  if (!pipe->read_end_open)
    return -1; /* Broken pipe */

  const uint8_t *buf = (const uint8_t *)data;
  size_t written = 0;

  while (written < len) {
    /* Wait for space */
    while (pipe->count >= PIPE_BUFFER_SIZE) {
      if (!pipe->read_end_open)
        return written > 0 ? written : -1;
      proc_yield(); /* Wait for reader */
    }

    /* Write one byte */
    pipe->buffer[pipe->write_pos] = buf[written++];
    pipe->write_pos = (pipe->write_pos + 1) % PIPE_BUFFER_SIZE;
    pipe->count++;
  }

  return written;
}

/*
 * Read from pipe
 */
int pipe_read(int fd, void *data, size_t len) {
  int is_write;
  pipe_t *pipe = pipe_from_fd(fd, &is_write);

  if (!pipe || is_write)
    return -1;

  uint8_t *buf = (uint8_t *)data;
  size_t read_count = 0;

  /* Block until data available (or pipe closed) */
  while (pipe->count == 0) {
    if (!pipe->write_end_open)
      return 0; /* EOF */
    proc_yield();
  }

  /* Read available data */
  while (read_count < len && pipe->count > 0) {
    buf[read_count++] = pipe->buffer[pipe->read_pos];
    pipe->read_pos = (pipe->read_pos + 1) % PIPE_BUFFER_SIZE;
    pipe->count--;
  }

  return read_count;
}

/*
 * Close pipe end
 */
int pipe_close(int fd) {
  int is_write;
  pipe_t *pipe = pipe_from_fd(fd, &is_write);

  if (!pipe)
    return -1;

  if (is_write) {
    pipe->write_end_open = 0;
  } else {
    pipe->read_end_open = 0;
  }

  /* Free pipe if both ends closed */
  if (!pipe->read_end_open && !pipe->write_end_open) {
    pipe->in_use = 0;
  }

  return 0;
}
