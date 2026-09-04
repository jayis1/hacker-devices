/*
 * MCTP Wraith main.c
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/sideband_bus.h"
#include "drivers/policy_engine.h"
#include "drivers/pldm_codec.h"
#include "drivers/telemetry.h"
#include "drivers/radio_link.h"

mw_runtime_t g_runtime;
mw_event_t g_events[MW_MAX_EVENTS];
uint32_t g_event_head = 0U;
uint32_t g_now_ms = 0U;

const char *mw_bus_name(mw_bus_kind_t kind) {
    switch (kind) {
        case MW_BUS_SMBUS: return "smbus";
        case MW_BUS_I3C: return "i3c";
        case MW_BUS_PCIE_VDM: return "pcie-vdm";
        default: return "unknown";
    }
}

const char *mw_scenario_name(mw_scenario_kind_t kind) {
    switch (kind) {
        case MW_SCENARIO_ROUTE_POISON: return "route-poison";
        case MW_SCENARIO_SPDM_DOWNGRADE_PROBE: return "spdm-downgrade-probe";
        case MW_SCENARIO_PLDM_STAGE_FUZZ: return "pldm-stage-fuzz";
        case MW_SCENARIO_SENSOR_GHOST: return "sensor-ghost";
        case MW_SCENARIO_ENDPOINT_CLONE: return "endpoint-clone";
        case MW_SCENARIO_IDLE:
        default: return "idle";
    }
}

void mw_log_event(mw_event_type_t type, uint32_t p0, uint32_t p1, const char *text) {
    mw_event_t *slot = &g_events[g_event_head % MW_MAX_EVENTS];
    slot->timestamp_ms = g_now_ms;
    slot->type = type;
    slot->p0 = p0;
    slot->p1 = p1;
    snprintf(slot->text, sizeof(slot->text), "%s", text != NULL ? text : "");
    g_event_head++;
}

static void runtime_defaults(void) {
    memset(&g_runtime, 0, sizeof(g_runtime));
    g_runtime.flags = MW_FLAG_PASSIVE | MW_FLAG_BRIDGING_ENABLED | MW_FLAG_SAFE_MODE;
    g_runtime.bus_kind = MW_BUS_I3C;
}

static void add_endpoint(uint8_t eid,
                         const char *name,
                         const char *medium,
                         bool mutable_target,
                         bool supports_spdm,
                         bool supports_pldm) {
    if (g_runtime.endpoint_count >= MW_MAX_ENDPOINTS) {
        return;
    }
    mw_endpoint_t *endpoint = &g_runtime.endpoints[g_runtime.endpoint_count++];
    memset(endpoint, 0, sizeof(*endpoint));
    endpoint->eid = eid;
    endpoint->present = true;
    endpoint->bridge_visible = true;
    endpoint->mutable_target = mutable_target;
    endpoint->supports_spdm = supports_spdm ? 1U : 0U;
    endpoint->supports_pldm = supports_pldm ? 1U : 0U;
    snprintf(endpoint->name, sizeof(endpoint->name), "%s", name);
    snprintf(endpoint->medium, sizeof(endpoint->medium), "%s", medium);
}

static void add_route(uint8_t src, uint8_t dst, uint8_t next_hop, uint8_t medium_tag, uint8_t ttl) {
    if (g_runtime.route_count >= MW_MAX_ROUTES) {
        return;
    }
    mw_route_t *route = &g_runtime.routes[g_runtime.route_count++];
    memset(route, 0, sizeof(*route));
    route->src_eid = src;
    route->dst_eid = dst;
    route->next_hop = next_hop;
    route->medium_tag = medium_tag;
    route->ttl = ttl;
}

static void topology_init(void) {
    add_endpoint(8U, "host-root-complex", "i3c-backplane", false, true, true);
    add_endpoint(20U, "bmc-security-agent", "i3c-backplane", true, true, true);
    add_endpoint(33U, "retimer-1", "smbus-sideband", true, false, true);
    add_endpoint(44U, "nic-management", "ncsi-bridge", true, true, true);
    add_endpoint(52U, "nvme-enclosure", "pcie-vdm", true, true, true);

    add_route(8U, 20U, 20U, 1U, 8U);
    add_route(8U, 33U, 20U, 1U, 6U);
    add_route(8U, 44U, 20U, 2U, 6U);
    add_route(20U, 52U, 52U, 3U, 6U);
    add_route(44U, 8U, 20U, 2U, 6U);
}

static void board_init(void) {
    runtime_defaults();
    topology_init();
    telemetry_init();
    sideband_bus_init();
    policy_engine_init();
    policy_engine_load_defaults();
    radio_link_init();
    sideband_bus_set_kind(g_runtime.bus_kind);
    sideband_bus_enable_capture(true);
    sideband_bus_seed_demo_traffic();
    mw_log_event(MW_EVENT_BOOT, MW_FW_VERSION_MAJOR, MW_FW_VERSION_MINOR, "booted in passive bridge mode");
}

static void print_topology(void) {
    printf("[topology] endpoints=%zu routes=%zu bus=%s flags=0x%02x\n",
           g_runtime.endpoint_count,
           g_runtime.route_count,
           mw_bus_name(g_runtime.bus_kind),
           g_runtime.flags);
    for (size_t i = 0; i < g_runtime.endpoint_count; ++i) {
        const mw_endpoint_t *endpoint = &g_runtime.endpoints[i];
        printf("  endpoint eid=%u name=%s medium=%s mutable=%u spdm=%u pldm=%u\n",
               endpoint->eid,
               endpoint->name,
               endpoint->medium,
               endpoint->mutable_target ? 1U : 0U,
               endpoint->supports_spdm,
               endpoint->supports_pldm);
    }
    for (size_t i = 0; i < g_runtime.route_count; ++i) {
        const mw_route_t *route = &g_runtime.routes[i];
        printf("  route src=%u dst=%u hop=%u medium=%u ttl=%u poisoned=%u\n",
               route->src_eid,
               route->dst_eid,
               route->next_hop,
               route->medium_tag,
               route->ttl,
               route->poisoned ? 1U : 0U);
    }
}

static void apply_scenario_topology_changes(void) {
    switch (g_runtime.scenario.kind) {
        case MW_SCENARIO_ROUTE_POISON:
            if (g_runtime.route_count > 2U) {
                g_runtime.routes[2].next_hop = 44U;
                g_runtime.routes[2].poisoned = true;
                mw_log_event(MW_EVENT_ROUTE, 2U, 44U, "rewired host-to-retimer route via nic-management");
            }
            break;
        case MW_SCENARIO_ENDPOINT_CLONE:
            if (g_runtime.endpoint_count > 3U) {
                g_runtime.endpoints[3].bridge_visible = true;
                snprintf(g_runtime.endpoints[3].name, sizeof(g_runtime.endpoints[3].name), "%s", "nic-management-clone");
                mw_log_event(MW_EVENT_ROUTE, 44U, 1U, "endpoint clone persona enabled");
            }
            break;
        default:
            break;
    }
}

static void process_traffic(void) {
    mw_message_t message;
    while (sideband_bus_next_message(&message)) {
        telemetry_tick();
        pldm_codec_print_message(&message);
        const bool forward = policy_engine_apply(&message);
        if (forward) {
            printf("[forward] src=%u dst=%u cmd=0x%02x measurement=0x%08x\n",
                   message.eid_src,
                   message.eid_dst,
                   message.command_code,
                   pldm_codec_measure(&message));
        } else {
            printf("[drop] src=%u dst=%u cmd=0x%02x\n",
                   message.eid_src,
                   message.eid_dst,
                   message.command_code);
        }
        g_now_ms += 47U;
        policy_engine_tick();
    }
}

static void print_events(void) {
    const uint32_t count = g_event_head < MW_MAX_EVENTS ? g_event_head : MW_MAX_EVENTS;
    printf("[events] count=%u\n", count);
    for (uint32_t i = 0U; i < count; ++i) {
        const mw_event_t *event = &g_events[i];
        printf("  [%03u] t=%ums type=%u p0=%u p1=%u text=%s\n",
               i,
               event->timestamp_ms,
               event->type,
               event->p0,
               event->p1,
               event->text);
    }
}

static mw_scenario_kind_t parse_scenario(const char *arg) {
    if (arg == NULL) {
        return MW_SCENARIO_IDLE;
    }
    if (strcmp(arg, "route-poison") == 0) {
        return MW_SCENARIO_ROUTE_POISON;
    }
    if (strcmp(arg, "spdm-downgrade-probe") == 0) {
        return MW_SCENARIO_SPDM_DOWNGRADE_PROBE;
    }
    if (strcmp(arg, "pldm-stage-fuzz") == 0) {
        return MW_SCENARIO_PLDM_STAGE_FUZZ;
    }
    if (strcmp(arg, "sensor-ghost") == 0) {
        return MW_SCENARIO_SENSOR_GHOST;
    }
    if (strcmp(arg, "endpoint-clone") == 0) {
        return MW_SCENARIO_ENDPOINT_CLONE;
    }
    return MW_SCENARIO_IDLE;
}

int main(int argc, char **argv) {
    board_init();
    mw_scenario_kind_t scenario = MW_SCENARIO_SPDM_DOWNGRADE_PROBE;
    if (argc > 1) {
        scenario = parse_scenario(argv[1]);
    }
    if (scenario != MW_SCENARIO_IDLE) {
        policy_engine_arm_scenario(scenario);
        apply_scenario_topology_changes();
    }

    print_topology();
    policy_engine_dump();
    process_traffic();
    sideband_bus_print_capture_summary();
    telemetry_print();
    radio_link_print_log();
    print_events();
    return 0;
}
