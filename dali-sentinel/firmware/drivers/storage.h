/* CRC-protected evidence storage. Author: jayis1. */
#ifndef DALI_STORAGE_H
#define DALI_STORAGE_H
#include "dali_phy.h"
#include "analyzer.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef bool (*storage_writer_t)(uint32_t block, const uint8_t data[512]);
typedef struct { uint32_t sequence, blocks_written, records_written, write_failures; bool dirty; } storage_status_t;
void storage_init(storage_writer_t writer);
bool storage_append_frame(const dali_frame_t *frame);
bool storage_append_finding(const finding_t *finding);
bool storage_flush(void);
storage_status_t storage_status(void);
uint32_t storage_crc32c(const void *data, size_t length);
bool storage_run_self_test(void);
#endif
