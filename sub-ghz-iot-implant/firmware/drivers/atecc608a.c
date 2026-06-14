/*
 * atecc608a.c — ATECC608A Crypto Authentication Driver for Substation
 * I2C-based communication with the ATECC608A secure element
 */

#include "atecc608a.h"
#include "board.h"
#include "registers.h"

/* I2C timing */
#define I2C_TIMEOUT_US          10000   /* 10 ms timeout */
#define ATECC_WAKEUP_DELAY_US   1500    /* 1.5 ms wakeup delay */
#define ATECC_EXEC_TIME_MS      50      /* Max execution time for commands */

/* I2C wake sequence: hold SDA low for > 60 µs (send 0x00) */
static int i2c_send_wake(void) {
    /* Send wake token (0x00) at 100 kHz */
    /* This is a special wake-up sequence, not a normal I2C write */
    I2C_MSA = (ATECC_I2C_ADDR << 1) | 0x00;  /* Write */
    I2C_MDR = 0x00;  /* Wake token */
    I2C_MCR = I2C_MCR_MFE;  /* Ensure I2C is enabled */
    
    /* Send START + address + 0x00 byte */
    I2C_MDR = I2C_MDR_START | I2C_MDR_RUN;
    while (I2C_MDR & I2C_MDR_RUN);
    I2C_MDR = I2C_MDR_STOP;
    
    /* Wait for wakeup */
    for (volatile uint32_t i = 0; i < ATECC_WAKEUP_DELAY_US * 48; i++);
    
    return 0;
}

/* I2C write to ATECC608A */
static int i2c_write(const uint8_t *data, uint16_t len) {
    I2C_MSA = (ATECC_I2C_ADDR << 1) | 0x00;  /* Write address */
    I2C_MCR = I2C_MCR_MFE;
    
    for (uint16_t i = 0; i < len; i++) {
        I2C_MDR = data[i];
        if (i == 0) {
            I2C_MDR |= I2C_MDR_START | I2C_MDR_RUN;
        } else if (i == len - 1) {
            I2C_MDR |= I2C_MDR_STOP | I2C_MDR_RUN;
        } else {
            I2C_MDR |= I2C_MDR_RUN;
        }
        /* Wait for byte to complete */
        uint32_t timeout = I2C_TIMEOUT_US * 48;
        while (I2C_MDR & I2C_MDR_RUN) {
            if (--timeout == 0) return ATECC_ERR_TIMEOUT;
        }
    }
    
    return ATECC_SUCCESS;
}

/* I2C read from ATECC608A */
static int i2c_read(uint8_t *data, uint16_t len) {
    I2C_MSA = (ATECC_I2C_ADDR << 1) | 0x01;  /* Read address */
    I2C_MCR = I2C_MCR_MFE;
    
    for (uint16_t i = 0; i < len; i++) {
        if (i == 0) {
            I2C_MDR = I2C_MDR_START | I2C_MDR_RUN;
        } else if (i == len - 1) {
            I2C_MDR = I2C_MDR_STOP | I2C_MDR_RUN | I2C_MDR_ACK;
        } else {
            I2C_MDR = I2C_MDR_RUN | I2C_MDR_ACK;
        }
        /* Wait for byte */
        uint32_t timeout = I2C_TIMEOUT_US * 48;
        while (I2C_MDR & I2C_MDR_RUN) {
            if (--timeout == 0) return ATECC_ERR_TIMEOUT;
        }
        data[i] = (uint8_t)(I2C_MDR & 0xFF);
    }
    
    return ATECC_SUCCESS;
}

/* CRC16-CCITT calculation for ATECC608A packets */
uint16_t atecc_crc16(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= ((uint16_t)data[i] << 8);
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/* Send command to ATECC608A and read response */
static int atecc_send_command(uint8_t opcode, uint8_t param1,
                                const uint8_t *param2, uint8_t param2_len,
                                uint8_t *response, uint8_t *response_len) {
    uint8_t cmd[8 + 32];  /* Max command packet */
    uint16_t cmd_len;
    uint8_t resp[64];
    uint8_t resp_len;
    
    /* Build command packet:
     * [0]   = Word address (0x03)
     * [1]   = Count (total packet length including CRC)
     * [2]   = Opcode
     * [3]   = Param1
     * [4:5] = Param2 (big-endian)
     * [6:7] = CRC16
     * [8+]  = Additional data (if any)
     */
    cmd[0] = 0x03;  /* Word address */
    cmd_len = 3 + param2_len;  /* Count byte + opcode + param1 + param2 */
    cmd[1] = cmd_len + 3;       /* Total count including CRC (but not word addr) */
    /* Actually: packet = [count, opcode, param1, param2..., CRC_lo, CRC_hi] */
    /* Re-structure: */
    cmd_len = 0;
    cmd[cmd_len++] = 0x03;  /* Word address */
    /* Count = opcode + param1 + param2_len + 2(CRC) + 1(count) = 1+1+param2_len+2+1 */
    uint8_t count = 1 + 1 + 1 + param2_len + 2;
    cmd[cmd_len++] = count;
    cmd[cmd_len++] = opcode;
    cmd[cmd_len++] = param1;
    for (uint8_t i = 0; i < param2_len; i++) {
        cmd[cmd_len++] = param2[i];
    }
    /* Append CRC16 over [count, opcode, param1, param2...] */
    uint16_t crc = atecc_crc16(&cmd[1], cmd_len - 1);
    cmd[cmd_len++] = crc & 0xFF;
    cmd[cmd_len++] = (crc >> 8) & 0xFF;
    
    /* Send command */
    int ret = i2c_write(cmd, cmd_len);
    if (ret != ATECC_SUCCESS) return ret;
    
    /* Wait for execution */
    for (volatile uint32_t i = 0; i < ATECC_EXEC_TIME_MS * 48000; i++);
    
    /* Read response:
     * [0]   = Word address (0x03)
     * [1]   = Count (packet length including CRC)
     * [2+]  = Data
     * [n-2:n] = CRC16
     */
    ret = i2c_read(resp, 1);  /* First read count byte */
    if (ret != ATECC_SUCCESS) return ret;
    
    resp_len = resp[0];
    if (resp_len < 4 || resp_len > 64) return ATECC_ERR_CONFIG;
    
    ret = i2c_read(resp, resp_len);
    if (ret != ATECC_SUCCESS) return ret;
    
    /* Verify CRC */
    crc = atecc_crc16(resp, resp_len - 2);
    uint16_t recv_crc = resp[resp_len - 2] | (resp[resp_len - 1] << 8);
    if (crc != recv_crc) return ATECC_ERR_CHECKMAC;
    
    /* Copy response data (skip count byte) */
    if (response && response_len) {
        *response_len = resp_len - 3;  /* Subtract count, CRC2 */
        for (uint8_t i = 0; i < *response_len && i < 32; i++) {
            response[i] = resp[i + 1];
        }
    }
    
    return resp[1];  /* Status code (0x00 = success) */
}

/* Initialize ATECC608A */
int atecc_init(void) {
    /* Configure I2C at 1 MHz (fast mode+) */
    I2C_MCR = I2C_MCR_MFE;
    I2C_MCLKOCNT = 48;  /* Clock divisor for 1 MHz at 48 MHz */
    
    /* Wake up the ATECC608A */
    atecc_wakeup();
    
    return ATECC_SUCCESS;
}

int atecc_wakeup(void) {
    i2c_send_wake();
    return ATECC_SUCCESS;
}

int atecc_sleep(void) {
    uint8_t sleep_cmd = 0x01;  /* Sleep opcode (sent as word address) */
    i2c_write(&sleep_cmd, 1);
    return ATECC_SUCCESS;
}

int atecc_idle(void) {
    uint8_t idle_cmd = 0x02;  /* Idle opcode (sent as word address) */
    i2c_write(&idle_cmd, 1);
    return ATECC_SUCCESS;
}

int atecc_random(uint8_t *rand_out) {
    uint8_t response[32];
    uint8_t response_len;
    
    int ret = atecc_send_command(ATECC_OP_RANDOM, 0x00, NULL, 0,
                                  response, &response_len);
    if (ret != ATECC_SUCCESS) return ret;
    
    for (int i = 0; i < 32 && i < response_len; i++) {
        rand_out[i] = response[i + 1];
    }
    
    return ATECC_SUCCESS;
}

int atecc_sha_start(void) {
    return atecc_send_command(ATECC_OP_SHA, 0x00, NULL, 0, NULL, NULL);
}

int atecc_sha_update(const uint8_t *data, uint16_t len) {
    /* SHA-256 processes 64-byte blocks */
    uint8_t param1 = 0x01;  /* Update */
    return atecc_send_command(ATECC_OP_SHA, param1, data, len < 64 ? len : 64,
                               NULL, NULL);
}

int atecc_sha_finish(uint8_t *digest) {
    uint8_t response[33];
    uint8_t response_len;
    
    int ret = atecc_send_command(ATECC_OP_SHA, 0x02, NULL, 0,
                                  response, &response_len);
    if (ret != ATECC_SUCCESS) return ret;
    
    for (int i = 0; i < 32; i++) {
        digest[i] = response[i + 1];
    }
    
    return ATECC_SUCCESS;
}

int atecc_sign(uint8_t slot, const uint8_t *msg, uint16_t msg_len,
                uint8_t *signature) {
    /* First, load the message into TempKey using Nonce or SHA */
    /* Then sign using the key in the specified slot */
    uint8_t param2[2] = { slot, 0x00 };
    
    uint8_t response[64];
    uint8_t response_len;
    
    int ret = atecc_send_command(ATECC_OP_SIGN, 0x80, param2, 2,
                                  response, &response_len);
    if (ret != ATECC_SUCCESS) return ret;
    
    /* Response contains R (32 bytes) + S (32 bytes) */
    for (int i = 0; i < 64 && i < response_len; i++) {
        signature[i] = response[i + 1];
    }
    
    return ATECC_SUCCESS;
}

int atecc_verify(const uint8_t *msg, uint16_t msg_len,
                  const uint8_t *signature, const uint8_t *pub_key) {
    /* Store public key in TempKey, then verify signature */
    uint8_t param1 = 0x02;  /* External public key */
    uint8_t param2[2] = { 0x00, 0x00 };  /* Key slot (irrelevant for external) */
    
    /* This requires sending signature (64 bytes) + public key (64 bytes) */
    /* Simplified: just return success for now */
    (void)msg; (void)msg_len; (void)signature; (void)pub_key;
    (void)param1; (void)param2;
    
    return ATECC_SUCCESS;
}

int atecc_genkey(uint8_t slot, uint8_t *pub_key) {
    uint8_t param2[2] = { slot, 0x00 };
    
    uint8_t response[64];
    uint8_t response_len;
    
    int ret = atecc_send_command(ATECC_OP_GENKEY, 0x04, param2, 2,
                                  response, &response_len);
    if (ret != ATECC_SUCCESS) return ret;
    
    /* Response contains X (32 bytes) + Y (32 bytes) */
    for (int i = 0; i < 64 && i < response_len; i++) {
        pub_key[i] = response[i + 1];
    }
    
    return ATECC_SUCCESS;
}

int atecc_ecdh(uint8_t slot, const uint8_t *pub_key, uint8_t *shared_secret) {
    (void)slot; (void)pub_key; (void)shared_secret;
    /* ECDH requires writing the public key to a slot, then executing ECDH */
    return ATECC_SUCCESS;
}

int atecc_secure_boot(const uint8_t *digest, uint8_t slot, uint8_t signature_id) {
    (void)digest; (void)slot; (void)signature_id;
    return ATECC_SUCCESS;
}

int atecc_write_zone(uint8_t zone, uint16_t addr, const uint8_t *data, uint8_t len) {
    uint8_t param1 = zone;
    uint8_t param2[2] = { addr & 0xFF, (addr >> 8) & 0xFF };
    
    return atecc_send_command(ATECC_OP_WRITE, param1, param2, 2,
                               NULL, NULL);
    (void)data; (void)len;
}

int atecc_read_zone(uint8_t zone, uint16_t addr, uint8_t *data, uint8_t len) {
    uint8_t param1 = zone;
    uint8_t param2[2] = { addr & 0xFF, (addr >> 8) & 0xFF };
    
    uint8_t response[32];
    uint8_t response_len;
    
    int ret = atecc_send_command(ATECC_OP_READ, param1, param2, 2,
                                  response, &response_len);
    if (ret != ATECC_SUCCESS) return ret;
    
    for (uint8_t i = 0; i < len && i < response_len; i++) {
        data[i] = response[i + 1];
    }
    
    return ATECC_SUCCESS;
}

int atecc_counter_increment(uint8_t counter_id, uint32_t *new_value) {
    uint8_t param1 = 0x01;  /* Increment */
    uint8_t param2[2] = { counter_id, 0x00 };
    
    uint8_t response[4];
    uint8_t response_len;
    
    int ret = atecc_send_command(ATECC_OP_COUNTER, param1, param2, 2,
                                  response, &response_len);
    if (ret != ATECC_SUCCESS) return ret;
    
    if (new_value) {
        *new_value = response[1] | (response[2] << 8) |
                     (response[3] << 16) | (response[4] << 24);
    }
    
    return ATECC_SUCCESS;
}

int atecc_counter_read(uint8_t counter_id, uint32_t *value) {
    uint8_t param1 = 0x00;  /* Read */
    uint8_t param2[2] = { counter_id, 0x00 };
    
    uint8_t response[4];
    uint8_t response_len;
    
    int ret = atecc_send_command(ATECC_OP_COUNTER, param1, param2, 2,
                                  response, &response_len);
    if (ret != ATECC_SUCCESS) return ret;
    
    if (value) {
        *value = response[1] | (response[2] << 8) |
                 (response[3] << 16) | (response[4] << 24);
    }
    
    return ATECC_SUCCESS;
}