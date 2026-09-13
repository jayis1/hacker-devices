/* DALI Sentinel CRC-protected evidence storage
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "storage.h"
#include <string.h>

typedef struct {
    uint8_t magic[4];
    uint8_t version;
    uint8_t flags;
    uint16_t used;
    uint32_t sequence;
    uint32_t first_timestamp;
    uint8_t payload[492];
    uint32_t crc;
} evidence_block_t;

typedef struct {
    uint8_t type;
    uint8_t length;
    uint16_t reserved;
    uint32_t timestamp;
} record_header_t;

static evidence_block_t block_buffer;
static storage_writer_t block_writer;
static storage_status_t state;

uint32_t storage_crc32c(const void *data, size_t length)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= bytes[i];
        for (unsigned bit = 0; bit < 8u; ++bit) {
            uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1u) ^ (0x82F63B78u & mask);
        }
    }
    return ~crc;
}

static void new_block(void)
{
    memset(&block_buffer, 0, sizeof(block_buffer));
    block_buffer.magic[0] = 'D';
    block_buffer.magic[1] = 'A';
    block_buffer.magic[2] = 'L';
    block_buffer.magic[3] = 'S';
    block_buffer.version = 1u;
    block_buffer.sequence = state.sequence;
}

void storage_init(storage_writer_t writer)
{
    memset(&state, 0, sizeof(state));
    block_writer = writer;
    new_block();
}

bool storage_flush(void)
{
    if (!state.dirty) return true;
    block_buffer.crc = storage_crc32c(&block_buffer, sizeof(block_buffer) - sizeof(uint32_t));
    if (block_writer == NULL || !block_writer(state.sequence, (const uint8_t *)&block_buffer)) {
        state.write_failures++;
        return false;
    }
    state.blocks_written++;
    state.sequence++;
    state.dirty = false;
    new_block();
    return true;
}

static bool append_record(uint8_t type, uint32_t timestamp, const void *record, size_t length)
{
    if (record == NULL || length == 0u || length > UINT8_MAX) return false;
    size_t required = sizeof(record_header_t) + length;
    if ((size_t)block_buffer.used + required > sizeof(block_buffer.payload)) {
        if (!storage_flush()) return false;
    }
    record_header_t header;
    header.type = type;
    header.length = (uint8_t)length;
    header.reserved = 0u;
    header.timestamp = timestamp;
    if (block_buffer.used == 0u) block_buffer.first_timestamp = timestamp;
    memcpy(&block_buffer.payload[block_buffer.used], &header, sizeof(header));
    block_buffer.used = (uint16_t)(block_buffer.used + sizeof(header));
    memcpy(&block_buffer.payload[block_buffer.used], record, length);
    block_buffer.used = (uint16_t)(block_buffer.used + length);
    state.records_written++;
    state.dirty = true;
    return true;
}

bool storage_append_frame(const dali_frame_t *frame)
{
    if (frame == NULL) return false;
    return append_record(1u, frame->start_tick, frame, sizeof(*frame));
}

bool storage_append_finding(const finding_t *finding)
{
    if (finding == NULL) return false;
    return append_record(2u, finding->timestamp_ms, finding, sizeof(*finding));
}

storage_status_t storage_status(void) { return state; }

static uint8_t test_block[512];
static bool test_writer(uint32_t number, const uint8_t data[512])
{
    if (number != 0u) return false;
    memcpy(test_block, data, sizeof(test_block));
    return true;
}

bool storage_run_self_test(void)
{
    static const char check[] = "123456789";
    if (storage_crc32c(check, 9u) != 0xE3069283u) return false;
    storage_init(test_writer);
    dali_frame_t frame = {0};
    frame.start_tick = 42u;
    frame.raw = 0xFF05u;
    frame.bit_count = 16u;
    frame.framing_ok = true;
    if (!storage_append_frame(&frame)) return false;
    if (!storage_flush()) return false;
    evidence_block_t copy;
    memcpy(&copy, test_block, sizeof(copy));
    uint32_t expected = copy.crc;
    copy.crc = 0u;
    uint32_t actual = storage_crc32c(&copy, sizeof(copy) - sizeof(uint32_t));
    return expected == actual && state.blocks_written == 1u && state.records_written == 1u;
}
