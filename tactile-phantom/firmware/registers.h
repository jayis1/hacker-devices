/*
 * Tactile-Phantom — Touch-Controller Bus MITM Implant
 * registers.h — Vendor touch-controller register maps
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Each vendor's touch controller exposes a different I2C/SPI register map.
 * This file defines those maps as compile-time tables so the decode engine
 * can map raw register offsets to semantic meaning (touch state, coordinates,
 * config, firmware). Adding a new controller means adding a table entry.
 */

#ifndef TACTILE_PHANTOM_REGISTERS_H
#define TACTILE_PHANTOM_REGISTERS_H

#include <stdint.h>
#include "board.h"

/* --- Register semantic types ----------------------------------------- */
typedef enum {
    TP_REG_UNKNOWN    = 0,  /* not in map */
    TP_REG_CHIP_ID    = 1,  /* chip identification */
    TP_REG_CONFIG     = 2,  /* configuration (host writes) */
    TP_REG_TOUCH_STA  = 3,  /* touch status / finger count */
    TP_REG_COORD      = 4,  /* touch coordinate data */
    TP_REG_GESTURE    = 5,  /* gesture event */
    TP_REG_FW_VERSION = 6,  /* firmware version */
    TP_REG_FW_UPDATE  = 7,  /* firmware update buffer */
    TP_REG_PROXIMITY  = 8,  /* proximity / hover */
    TP_REG_POWER      = 9,  /* power mode control */
    TP_REG_INT        = 10, /* interrupt / status flags */
    TP_REG_RESET      = 11, /* software reset */
    TP_REG_CHECKSUM   = 12, /* checksum / CRC */
    TP_REG_COMMAND   = 13, /* command register */
    TP_REG_BUFFER     = 14, /* general-purpose data buffer */
} tp_reg_semantic_t;

/* --- Register map entry ---------------------------------------------- */
typedef struct {
    uint16_t           offset;     /* register offset (I2C) or SPI command */
    uint16_t           length;     /* bytes in this register */
    tp_reg_semantic_t  semantic;  /* what it means */
    const char        *name;      /* human-readable name */
} tp_reg_entry_t;

/* --- Goodix GT9xx series (GT911, GT9147, GT9157, GT9271, etc.) -------- */
/* 16-bit register address, big-endian */
static const tp_reg_entry_t tp_map_goodix[] = {
    { 0x8140, 4,  TP_REG_CHIP_ID,    "ProductID" },
    { 0x8047, 1,  TP_REG_FW_VERSION, "FWVer" },
    { 0x8048, 2,  TP_REG_CONFIG,    "XRes" },
    { 0x804A, 2,  TP_REG_CONFIG,    "YRes" },
    { 0x804C, 1,  TP_REG_CONFIG,    "TouchNum" },
    { 0x804D, 1,  TP_REG_CONFIG,    "ModuleSwitch" },
    { 0x8050, 1,  TP_REG_CONFIG,    "SensAdj" },
    { 0x8056, 1,  TP_REG_CONFIG,    "WaterProof" },
    { 0x8057, 1,  TP_REG_CONFIG,    "NoiseLevel" },
    { 0x806D, 1,  TP_REG_CONFIG,    "Drift" },
    { 0x8070, 1,  TP_REG_CONFIG,    "NoiseThreshold" },
    { 0x814E, 1,  TP_REG_TOUCH_STA, "TouchState" },
    { 0x814F, 1,  TP_REG_COORD,     "CoordStart" },
    { 0x8150, 8,  TP_REG_COORD,     "Point1" },
    { 0x8158, 8,  TP_REG_COORD,     "Point2" },
    { 0x8160, 8,  TP_REG_COORD,     "Point3" },
    { 0x8168, 8,  TP_REG_COORD,     "Point4" },
    { 0x8170, 8,  TP_REG_COORD,     "Point5" },
    { 0x8178, 8,  TP_REG_COORD,     "Point6" },
    { 0x8180, 8,  TP_REG_COORD,     "Point7" },
    { 0x8188, 8,  TP_REG_COORD,     "Point8" },
    { 0x8190, 8,  TP_REG_COORD,     "Point9" },
    { 0x8198, 8,  TP_REG_COORD,     "Point10" },
    { 0x8170, 1,  TP_REG_GESTURE,   "GestureID" },
    { 0x8144, 2,  TP_REG_CONFIG,    "CmdStatus" },
    { 0x8040, 1,  TP_REG_COMMAND,   "ConfigVer" },
    { 0x60FE, 1,  TP_REG_RESET,     "SoftReset" },
    { 0x0000, 0,  TP_REG_UNKNOWN,   NULL }  /* sentinel */
};

/* Goodix touch-state register bits */
#define GT_STATUS_BUF_READY  0x80
#define GT_STATUS_RESET      0x01
#define GT_STATUS_GESTURE    0x0E
#define GT_FINGER_COUNT_MASK 0x0F

/* Goodix per-point structure (8 bytes): */
/* [0]    TrackID, [1-2] X (LE), [3-4] Y (LE), [5] Area, [6] reserved, [7] reserved */
#define GT_POINT_STRUCT_LEN 8

/* --- FocalTech FT5xx series (FT5406, FT5316, FT6x06, etc.) ------------ */
/* 8-bit register address */
static const tp_reg_entry_t tp_map_focaltech[] = {
    { 0x00, 1,  TP_REG_CONFIG,     "DevMode" },
    { 0x01, 1,  TP_REG_GESTURE,    "GestureID" },
    { 0x02, 1,  TP_REG_FW_VERSION, "FWLib" },
    { 0x03, 1,  TP_REG_CONFIG,     "PowerMode" },
    { 0x04, 1,  TP_REG_FW_VERSION, "FWInt" },
    { 0x06, 1,  TP_REG_INT,        "Status" },
    { 0x07, 1,  TP_REG_TOUCH_STA,  "TouchNum" },
    /* Each point: 6 bytes at 0x03 offset from base, 1-5 points at 0x03..0x1E */
    { 0x03, 6,  TP_REG_COORD,     "Point1" },
    { 0x09, 6,  TP_REG_COORD,     "Point2" },
    { 0x0F, 6,  TP_REG_COORD,     "Point3" },
    { 0x15, 6,  TP_REG_COORD,     "Point4" },
    { 0x1B, 6,  TP_REG_COORD,     "Point5" },
    { 0xA3, 6,  TP_REG_CHIP_ID,   "ChipID" },
    { 0xA4, 1,  TP_REG_CONFIG,     "GMode" },
    { 0xA5, 1,  TP_REG_POWER,     "PowerMode" },
    { 0xA6, 1,  TP_REG_CONFIG,     "FWVer" },
    { 0xA7, 1,  TP_REG_CONFIG,     "ReleaseID" },
    { 0xA8, 1,  TP_REG_CONFIG,     "State" },
    { 0xA9, 1,  TP_REG_CHIP_ID,   "VendorID" },
    { 0xFB, 4,  TP_REG_FW_UPDATE, "FWUpdate" },
    { 0x00, 0,  TP_REG_UNKNOWN,   NULL }
};

/* FocalTech per-point struct (6 bytes): */
/* [0] X_H[3:0]=event, [1] X_L, [2] Y_H[3:0]=touch_id, [3] Y_L, [4] weight, [5] area */
#define FT_POINT_STRUCT_LEN 6
#define FT_EVENT_DOWN    0x00
#define FT_EVENT_UP      0x01
#define FT_EVENT_CONTACT 0x02
#define FT_EVENT_NONE    0x03

/* --- Synaptics S3320 / R63353 (used in many Apple/Samsung devices) ---- */
/* 16-bit register address, little-endian */
static const tp_reg_entry_t tp_map_synaptics[] = {
    { 0x0000, 2, TP_REG_CHIP_ID,    "ProductID" },
    { 0x0004, 2, TP_REG_FW_VERSION, "FWRev" },
    { 0x0008, 1, TP_REG_CONFIG,     "XMax" },
    { 0x000A, 1, TP_REG_CONFIG,     "YMax" },
    { 0x0010, 1, TP_REG_CONFIG,     "TouchNum" },
    { 0x0011, 1, TP_REG_INT,        "DeviceStatus" },
    { 0x0014, 1, TP_REG_INT,        "IRQStatus" },
    { 0x0015, 1, TP_REG_TOUCH_STA,  "FingerState" },
    /* S3320 reports up to 10 fingers, each 6 bytes from 0x18 */
    { 0x0018, 6, TP_REG_COORD,     "Point1" },
    { 0x001E, 6, TP_REG_COORD,     "Point2" },
    { 0x0024, 6, TP_REG_COORD,     "Point3" },
    { 0x002A, 6, TP_REG_COORD,     "Point4" },
    { 0x0030, 6, TP_REG_COORD,     "Point5" },
    { 0x0036, 6, TP_REG_COORD,     "Point6" },
    { 0x003C, 6, TP_REG_COORD,     "Point7" },
    { 0x0042, 6, TP_REG_COORD,     "Point8" },
    { 0x0048, 6, TP_REG_COORD,     "Point9" },
    { 0x004E, 6, TP_REG_COORD,     "Point10" },
    { 0x0100, 1, TP_REG_CONFIG,     "SensMode" },
    { 0x0102, 1, TP_REG_CONFIG,     "SleepMode" },
    { 0x0110, 1, TP_REG_COMMAND,   "Cmd" },
    { 0x0200, 1, TP_REG_GESTURE,   "Gesture" },
    { 0x0000, 0, TP_REG_UNKNOWN,   NULL }
};

#define SYN_POINT_STRUCT_LEN 6
#define SYN_FINGER_MASK     0x0F  /* bits 0-3 of FingerState = active fingers */

/* --- Ilitek ILI213x / ILI251x series --------------------------------- */
/* 16-bit register address */
static const tp_reg_entry_t tp_map_ilitek[] = {
    { 0x0000, 2, TP_REG_CHIP_ID,    "PID" },
    { 0x0002, 2, TP_REG_FW_VERSION, "FWVer" },
    { 0x0010, 1, TP_REG_TOUCH_STA,  "TouchCount" },
    { 0x0011, 1, TP_REG_GESTURE,    "Gesture" },
    { 0x0012, 1, TP_REG_INT,        "Status" },
    /* Coordinates start at 0x20, each point 8 bytes */
    { 0x0020, 8, TP_REG_COORD,     "Point1" },
    { 0x0028, 8, TP_REG_COORD,     "Point2" },
    { 0x0030, 8, TP_REG_COORD,     "Point3" },
    { 0x0038, 8, TP_REG_COORD,     "Point4" },
    { 0x0040, 8, TP_REG_COORD,     "Point5" },
    { 0x0048, 8, TP_REG_COORD,     "Point6" },
    { 0x0050, 8, TP_REG_COORD,     "Point7" },
    { 0x0058, 8, TP_REG_COORD,     "Point8" },
    { 0x0060, 8, TP_REG_COORD,     "Point9" },
    { 0x0068, 8, TP_REG_COORD,     "Point10" },
    { 0x0100, 2, TP_REG_CONFIG,     "XRes" },
    { 0x0102, 2, TP_REG_CONFIG,     "YRes" },
    { 0x0104, 1, TP_REG_CONFIG,     "MaxTouch" },
    { 0x0105, 1, TP_REG_CONFIG,     "IntMode" },
    { 0x0106, 1, TP_REG_CONFIG,     "Sens" },
    { 0x0200, 64, TP_REG_FW_UPDATE, "FWBuf" },
    { 0x0300, 1, TP_REG_COMMAND,   "Cmd" },
    { 0x0000, 0, TP_REG_UNKNOWN,   NULL }
};

#define ILI_POINT_STRUCT_LEN 8

/* --- Cypress/Infineon TT2xx series ------------------------------------ */
static const tp_reg_entry_t tp_map_cypress[] = {
    { 0x0000, 2, TP_REG_CHIP_ID,    "FamilyID" },
    { 0x0002, 2, TP_REG_FW_VERSION, "FWRev" },
    { 0x0008, 1, TP_REG_CONFIG,     "XMax" },
    { 0x000A, 1, TP_REG_CONFIG,     "YMax" },
    { 0x0012, 1, TP_REG_TOUCH_STA,  "TouchNum" },
    { 0x0013, 1, TP_REG_INT,        "Status" },
    { 0x0020, 6, TP_REG_COORD,     "Point1" },
    { 0x0026, 6, TP_REG_COORD,     "Point2" },
    { 0x002C, 6, TP_REG_COORD,     "Point3" },
    { 0x0032, 6, TP_REG_COORD,     "Point4" },
    { 0x0038, 6, TP_REG_COORD,     "Point5" },
    { 0x003E, 6, TP_REG_COORD,     "Point6" },
    { 0x0100, 1, TP_REG_CONFIG,     "Sens" },
    { 0x0101, 1, TP_REG_POWER,      "Power" },
    { 0x0102, 1, TP_REG_COMMAND,   "Cmd" },
    { 0x0200, 64, TP_REG_FW_UPDATE, "FWBuf" },
    { 0x0000, 0, TP_REG_UNKNOWN,   NULL }
};

#define CYP_POINT_STRUCT_LEN 6

/* --- Atmel maXTouch (uses 16-bit T37/T38 object registers) ------------ */
static const tp_reg_entry_t tp_map_atmel[] = {
    { 0x0000, 2, TP_REG_CHIP_ID,    "Family" },
    { 0x0002, 1, TP_REG_FW_VERSION, "FWVer" },
    { 0x0004, 1, TP_REG_CONFIG,     "ObjectNum" },
    { 0x0010, 2, TP_REG_INT,        "Status" },
    { 0x0012, 1, TP_REG_INT,        "MsgCount" },
    { 0x0014, 8, TP_REG_TOUCH_STA, "T9Msg" },
    { 0x0020, 8, TP_REG_COORD,     "TouchData" },
    { 0x0100, 2, TP_REG_CONFIG,     "XRes" },
    { 0x0102, 2, TP_REG_CONFIG,     "YRes" },
    { 0x0200, 64, TP_REG_FW_UPDATE, "FWUpdate" },
    { 0x0000, 0, TP_REG_UNKNOWN,   NULL }
};

/* --- Novatek NT36xxx series ------------------------------------------ */
static const tp_reg_entry_t tp_map_novatek[] = {
    { 0x00FF, 2, TP_REG_CHIP_ID,    "ChipVer" },
    { 0x0001, 1, TP_REG_FW_VERSION, "FWVer" },
    { 0x000E, 2, TP_REG_CONFIG,     "XRes" },
    { 0x0010, 2, TP_REG_CONFIG,     "YRes" },
    { 0x0012, 1, TP_REG_TOUCH_STA,  "FingerCount" },
    { 0x0014, 1, TP_REG_GESTURE,    "Gesture" },
    { 0x0020, 8, TP_REG_COORD,     "Point1" },
    { 0x0028, 8, TP_REG_COORD,     "Point2" },
    { 0x0030, 8, TP_REG_COORD,     "Point3" },
    { 0x0038, 8, TP_REG_COORD,     "Point4" },
    { 0x0040, 8, TP_REG_COORD,     "Point5" },
    { 0x0048, 8, TP_REG_COORD,     "Point6" },
    { 0x0050, 8, TP_REG_COORD,     "Point7" },
    { 0x0058, 8, TP_REG_COORD,     "Point8" },
    { 0x0060, 8, TP_REG_COORD,     "Point9" },
    { 0x0068, 8, TP_REG_COORD,     "Point10" },
    { 0x7F00, 128, TP_REG_FW_UPDATE, "FWBuf" },
    { 0x0000, 0, TP_REG_UNKNOWN,   NULL }
};

#define NVK_POINT_STRUCT_LEN 8

/* --- Vendor map registry --------------------------------------------- */
typedef struct {
    tp_tc_vendor_t         vendor;
    const tp_reg_entry_t  *map;
    uint8_t                point_struct_len;
    uint8_t                i2c_addr_default;
    bool                   addr_16bit;   /* true if 16-bit register address */
} tp_vendor_map_t;

static const tp_vendor_map_t tp_vendor_maps[] = {
    { TP_TC_GOODIX,    tp_map_goodix,    GT_POINT_STRUCT_LEN,  TP_I2C_ADDR_GOODIX_PRIMARY,  true  },
    { TP_TC_FOCALTECH, tp_map_focaltech, FT_POINT_STRUCT_LEN,  TP_I2C_ADDR_FOCALTECH,       false },
    { TP_TC_SYNAPTICS, tp_map_synaptics, SYN_POINT_STRUCT_LEN,  TP_I2C_ADDR_SYNAPTICS,       true  },
    { TP_TC_ILITEK,   tp_map_ilitek,   ILI_POINT_STRUCT_LEN,  TP_I2C_ADDR_ILITEK,           true  },
    { TP_TC_CYPRESS,  tp_map_cypress,  CYP_POINT_STRUCT_LEN,  TP_I2C_ADDR_CYPRESS,          false },
    { TP_TC_ATMEL,    tp_map_atmel,    8,                     TP_I2C_ADDR_ATMEL,            true  },
    { TP_TC_NOVATEK,  tp_map_novatek,  NVK_POINT_STRUCT_LEN,  TP_I2C_ADDR_NOVATEK,          true  },
};
#define TP_VENDOR_MAP_COUNT (sizeof(tp_vendor_maps) / sizeof(tp_vendor_maps[0]))

/* --- Lookup helpers --------------------------------------------------- */
/* Find the register-map entry for a given vendor + register offset.
 * Returns NULL if not found. Uses sentinel entry {0,0,UNKNOWN,NULL}. */
static inline const tp_reg_entry_t *tp_reg_lookup(tp_tc_vendor_t vendor,
                                                    uint16_t offset)
{
    for (uint32_t i = 0; i < TP_VENDOR_MAP_COUNT; i++) {
        if (tp_vendor_maps[i].vendor != vendor)
            continue;
        const tp_reg_entry_t *m = tp_vendor_maps[i].map;
        for (uint32_t j = 0; m[j].length != 0 || m[j].offset != 0; j++) {
            if (m[j].offset == offset)
                return &m[j];
        }
        /* Check if offset falls within a coordinate block (point N) */
        if (m[0].semantic == TP_REG_CHIP_ID) {
            /* Walk the full map including sentinel — but sentinel is {0,0}.
             * The loop above already handles all real entries. */
        }
        break;
    }
    return NULL;
}

/* Get the vendor map descriptor for a given vendor */
static inline const tp_vendor_map_t *tp_vendor_get(tp_tc_vendor_t v)
{
    for (uint32_t i = 0; i < TP_VENDOR_MAP_COUNT; i++) {
        if (tp_vendor_maps[i].vendor == v)
            return &tp_vendor_maps[i];
    }
    return NULL;
}

/* Chip-ID register values for vendor identification during auto-detect.
 * The auto-detect reads the chip-ID register and matches against these
 * to identify the controller. */
typedef struct {
    tp_tc_vendor_t vendor;
    uint16_t       id_reg_offset;
    uint8_t        id_bytes[4];   /* expected chip ID bytes */
    uint8_t        id_len;       /* 2 or 4 bytes to compare */
} tp_chip_id_t;

static const tp_chip_id_t tp_chip_ids[] = {
    /* Goodix: 0x8140 returns ASCII product ID */
    { TP_TC_GOODIX,    0x8140, { '9', '1', '1', 0 }, 3 },  /* "911" */
    { TP_TC_GOODIX,    0x8140, { '9', '1', '4', 0 }, 3 },  /* "9147" prefix */
    { TP_TC_GOODIX,    0x8140, { '9', '2', '7', 0 }, 3 },  /* "9271" prefix */
    { TP_TC_GOODIX,    0x8140, { 'G', 'T', 'X', 0 }, 3 },  /* GTX series */
    /* FocalTech: 0xA3 returns 2-byte chip ID (0x06 0x55 for FT5406) */
    { TP_TC_FOCALTECH, 0xA3,   { 0x06, 0x55, 0, 0 }, 2 },
    { TP_TC_FOCALTECH, 0xA3,   { 0x07, 0x35, 0, 0 }, 2 },  /* FT6206 */
    { TP_TC_FOCALTECH, 0xA3,   { 0x07, 0x98, 0, 0 }, 2 },  /* FT6236 */
    /* Synaptics S3320: 0x0000 returns 0x33 0x20 */
    { TP_TC_SYNAPTICS, 0x0000, { 0x33, 0x20, 0, 0 }, 2 },
    { TP_TC_SYNAPTICS, 0x0000, { 0x73, 0x32, 0, 0 }, 2 },
    /* Ilitek ILI2130: 0x0000 returns 0x21 0x30 */
    { TP_TC_ILITEK,   0x0000, { 0x21, 0x30, 0, 0 }, 2 },
    { TP_TC_ILITEK,   0x0000, { 0x25, 0x10, 0, 0 }, 2 },
    /* Cypress TT2xx: 0x0000 returns 0xCy prefix */
    { TP_TC_CYPRESS,  0x0000, { 0x43, 0x79, 0, 0 }, 2 },  /* "Cy" */
    /* Atmel: 0x0000 returns family ID */
    { TP_TC_ATMEL,    0x0000, { 0x71, 0x01, 0, 0 }, 2 },
    /* Novatek NT36672: 0x00FF returns 0x36 0x72 */
    { TP_TC_NOVATEK,  0x00FF, { 0x36, 0x72, 0, 0 }, 2 },
    { TP_TC_NOVATEK,  0x00FF, { 0x36, 0x76, 0, 0 }, 2 },
};
#define TP_CHIP_ID_COUNT (sizeof(tp_chip_ids) / sizeof(tp_chip_ids[0]))

#endif /* TACTILE_PHANTOM_REGISTERS_H */