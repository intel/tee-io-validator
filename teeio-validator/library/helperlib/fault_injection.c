/**
 *  Copyright Notice:
 *  Copyright 2026 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "teeio_fault_injection.h"

#define TEEIO_DOE_HEADER_SIZE 8
#define TEEIO_DOE_LENGTH_OFFSET 4
#define TEEIO_SPDM_CODE_OFFSET (TEEIO_DOE_HEADER_SIZE + 1)

static IDE_TEST_FAULT_CONFIG *m_config;
static uint8_t m_primary[TEEIO_FAULT_MAX_MESSAGE_SIZE];
static uint8_t m_secondary[TEEIO_FAULT_MAX_MESSAGE_SIZE];
static uint8_t m_previous[TEEIO_FAULT_MAX_RULES][TEEIO_FAULT_MAX_MESSAGE_SIZE];
static size_t m_previous_size[TEEIO_FAULT_MAX_RULES];
static uint8_t m_held[TEEIO_FAULT_MAX_MESSAGE_SIZE];
static size_t m_held_size;
static uint32_t m_total_fire_count;
static char m_audit[512];
static char m_result[768];
static char m_active_scenario[TEEIO_FAULT_MAX_NAME_SIZE];
static char m_active_expected[TEEIO_FAULT_MAX_EXPECTED_SIZE];
static char m_actual[TEEIO_FAULT_MAX_EXPECTED_SIZE];
static bool m_waiting_for_actual;
static int m_active_rule_id;
static teeio_fault_disposition_t m_active_disposition;
static uint32_t m_active_match_count;
static teeio_fault_recovery_t m_active_recovery;
static bool m_same_session_recovery_checked;
static bool m_same_session_recovery_passed;
static bool m_same_session_preflight;
static bool m_fault_suspended;

static void set_error(char *error, size_t error_size, const char *format, ...)
{
    va_list args;

    if (error == NULL || error_size == 0) {
        return;
    }

    va_start(args, format);
    vsnprintf(error, error_size, format, args);
    va_end(args);
}

static int hex_digit(char value)
{
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F') {
        return value - 'A' + 10;
    }
    return -1;
}

const char *teeio_fault_action_name(teeio_fault_action_t action)
{
    static const char *names[] = {
        "set", "xor", "truncate", "truncate_to", "extend",
        "set_declared_length",
        "drop", "duplicate", "replay", "reorder", "abandon"
    };

    if (action >= TEEIO_FAULT_ACTION_MAX) {
        return "unknown";
    }
    return names[action];
}

bool teeio_fault_parse_action(const char *name, teeio_fault_action_t *action)
{
    teeio_fault_action_t candidate;

    if (name == NULL || action == NULL) {
        return false;
    }
    for (candidate = 0; candidate < TEEIO_FAULT_ACTION_MAX; candidate++) {
        if (strcmp(name, teeio_fault_action_name(candidate)) == 0) {
            *action = candidate;
            return true;
        }
    }
    return false;
}

bool teeio_fault_parse_recovery(const char *name,
                                teeio_fault_recovery_t *recovery)
{
    if (name == NULL || recovery == NULL) {
        return false;
    }
    if (strcmp(name, "none") == 0) {
        *recovery = TEEIO_FAULT_RECOVERY_NONE;
        return true;
    }
    if (strcmp(name, "same_session") == 0) {
        *recovery = TEEIO_FAULT_RECOVERY_SAME_SESSION;
        return true;
    }
    return false;
}

const char *teeio_fault_disposition_name(teeio_fault_disposition_t disposition)
{
    static const char *names[] = {
        "pass", "mutate", "drop", "duplicate", "replay", "hold",
        "reorder", "abandon", "error"
    };

    if (disposition > TEEIO_FAULT_DISPOSITION_ERROR) {
        return "unknown";
    }
    return names[disposition];
}

int teeio_fault_parse_pattern(const char *value, uint8_t *pattern,
                              size_t pattern_capacity)
{
    size_t length;
    size_t source_index;
    size_t output_index;
    int high;
    int low;

    if (value == NULL || pattern == NULL) {
        return -1;
    }
    if (value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
        value += 2;
    }

    length = strlen(value);
    output_index = 0;
    source_index = 0;
    while (source_index < length) {
        while (source_index < length &&
               (value[source_index] == ' ' || value[source_index] == ':')) {
            source_index++;
        }
        if (source_index == length) {
            break;
        }
        if (source_index + 1 >= length || output_index >= pattern_capacity) {
            return -1;
        }
        high = hex_digit(value[source_index]);
        low = hex_digit(value[source_index + 1]);
        if (high < 0 || low < 0) {
            return -1;
        }
        pattern[output_index++] = (uint8_t)((high << 4) | low);
        source_index += 2;
    }

    return output_index == 0 ? -1 : (int)output_index;
}

bool teeio_fault_validate_rule(const teeio_fault_rule_t *rule, char *error,
                               size_t error_size)
{
    if (rule == NULL) {
        set_error(error, error_size, "rule is null");
        return false;
    }
    if (rule->scenario[0] == '\0') {
        set_error(error, error_size, "rule %d has no scenario", rule->id);
        return false;
    }
    if (rule->action >= TEEIO_FAULT_ACTION_MAX) {
        set_error(error, error_size, "rule %d has invalid action", rule->id);
        return false;
    }
    if (rule->recovery >= TEEIO_FAULT_RECOVERY_MAX) {
        set_error(error, error_size, "rule %d has invalid recovery", rule->id);
        return false;
    }
    if ((rule->action == TEEIO_FAULT_ACTION_SET ||
         rule->action == TEEIO_FAULT_ACTION_XOR ||
         rule->action == TEEIO_FAULT_ACTION_EXTEND) &&
        rule->pattern_size == 0) {
        set_error(error, error_size, "rule %d requires a pattern", rule->id);
        return false;
    }
    if (rule->pattern_size > TEEIO_FAULT_MAX_PATTERN_SIZE) {
        set_error(error, error_size, "rule %d pattern is too large", rule->id);
        return false;
    }
    if (rule->action == TEEIO_FAULT_ACTION_TRUNCATE && rule->size == 0) {
        set_error(error, error_size, "rule %d requires a truncate size", rule->id);
        return false;
    }
    if (rule->action == TEEIO_FAULT_ACTION_TRUNCATE_TO && rule->size == 0) {
        set_error(error, error_size, "rule %d requires a target size", rule->id);
        return false;
    }
    return true;
}

void teeio_fault_reset(void)
{
    uint32_t index;

    m_total_fire_count = 0;
    memset(m_previous, 0, sizeof(m_previous));
    memset(m_previous_size, 0, sizeof(m_previous_size));
    memset(m_held, 0, sizeof(m_held));
    m_held_size = 0;
    memset(m_audit, 0, sizeof(m_audit));
    memset(m_result, 0, sizeof(m_result));
    memset(m_active_scenario, 0, sizeof(m_active_scenario));
    memset(m_active_expected, 0, sizeof(m_active_expected));
    memset(m_actual, 0, sizeof(m_actual));
    m_waiting_for_actual = false;
    m_active_rule_id = -1;
    m_active_disposition = TEEIO_FAULT_DISPOSITION_PASS;
    m_active_match_count = 0;
    m_active_recovery = TEEIO_FAULT_RECOVERY_NONE;
    m_same_session_recovery_checked = false;
    m_same_session_recovery_passed = false;
    m_same_session_preflight = false;
    m_fault_suspended = false;

    if (m_config == NULL) {
        return;
    }
    for (index = 0; index < m_config->rule_count; index++) {
        m_config->rules[index].match_count = 0;
    }
}

void teeio_fault_init(IDE_TEST_FAULT_CONFIG *config)
{
    m_config = config;
    teeio_fault_reset();
}

bool teeio_fault_is_enabled(void)
{
    return m_config != NULL && m_config->rule_count != 0;
}

bool teeio_fault_scenario_is(const char *scenario)
{
    uint32_t index;

    if (m_config == NULL || scenario == NULL) {
        return false;
    }
    for (index = 0; index < m_config->rule_count; index++) {
        if (strcmp(m_config->rules[index].scenario, scenario) == 0) {
            return true;
        }
    }
    return false;
}

static uint32_t read_u32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static void write_u32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
}

static void update_doe_length(size_t message_size)
{
    if (message_size >= TEEIO_DOE_HEADER_SIZE) {
        write_u32(m_primary + TEEIO_DOE_LENGTH_OFFSET,
                  (uint32_t)((message_size + 3) / 4));
    }
}

static bool rule_matches(teeio_fault_rule_t *rule,
                         const uint8_t *message, size_t message_size,
                         bool *selector_matched)
{
    uint32_t doe_type;
    uint32_t spdm_code;

    *selector_matched = false;
    if (message_size < TEEIO_DOE_HEADER_SIZE) {
        return false;
    }

    doe_type = message[2];
    if (rule->doe_type != TEEIO_FAULT_MATCH_ANY && rule->doe_type != doe_type) {
        return false;
    }

    spdm_code = TEEIO_FAULT_MATCH_ANY;
    if (message_size > TEEIO_SPDM_CODE_OFFSET) {
        spdm_code = message[TEEIO_SPDM_CODE_OFFSET];
    }
    if (rule->spdm_code != TEEIO_FAULT_MATCH_ANY &&
        rule->spdm_code != spdm_code) {
        return false;
    }

    *selector_matched = true;
    if (m_same_session_preflight) {
        return false;
    }
    rule->match_count++;
    return rule->occurrence == TEEIO_FAULT_OCCURRENCE_ALL ||
           rule->occurrence == rule->match_count;
}

static teeio_fault_disposition_t apply_mutation(const teeio_fault_rule_t *rule,
                                                 size_t *message_size,
                                                 size_t message_capacity)
{
    size_t index;
    size_t offset;

    switch (rule->action) {
    case TEEIO_FAULT_ACTION_SET:
    case TEEIO_FAULT_ACTION_XOR:
        if (rule->offset_from_end) {
            if (rule->offset == 0 || rule->offset > *message_size) {
                return TEEIO_FAULT_DISPOSITION_ERROR;
            }
            offset = *message_size - rule->offset;
        } else {
            offset = rule->offset;
        }
        if (offset + rule->pattern_size > *message_size) {
            return TEEIO_FAULT_DISPOSITION_ERROR;
        }
        for (index = 0; index < rule->pattern_size; index++) {
            if (rule->action == TEEIO_FAULT_ACTION_SET) {
                m_primary[offset + index] = rule->pattern[index];
            } else {
                m_primary[offset + index] ^= rule->pattern[index];
            }
        }
        return TEEIO_FAULT_DISPOSITION_MUTATE;
    case TEEIO_FAULT_ACTION_TRUNCATE:
        if (rule->size >= *message_size) {
            return TEEIO_FAULT_DISPOSITION_ERROR;
        }
        *message_size -= rule->size;
        update_doe_length(*message_size);
        return TEEIO_FAULT_DISPOSITION_MUTATE;
    case TEEIO_FAULT_ACTION_TRUNCATE_TO:
        if (rule->size >= *message_size) {
            return TEEIO_FAULT_DISPOSITION_ERROR;
        }
        *message_size = rule->size;
        update_doe_length(*message_size);
        return TEEIO_FAULT_DISPOSITION_MUTATE;
    case TEEIO_FAULT_ACTION_EXTEND:
        index = rule->size == 0 ? rule->pattern_size : rule->size;
        if (*message_size + index > message_capacity ||
            *message_size + index > sizeof(m_primary)) {
            return TEEIO_FAULT_DISPOSITION_ERROR;
        }
        for (size_t pattern_index = 0; pattern_index < index; pattern_index++) {
            m_primary[*message_size + pattern_index] =
                rule->pattern[pattern_index % rule->pattern_size];
        }
        *message_size += index;
        update_doe_length(*message_size);
        if (rule->declared_length != 0) {
            if (rule->offset + sizeof(uint32_t) > *message_size) {
                return TEEIO_FAULT_DISPOSITION_ERROR;
            }
            write_u32(m_primary + rule->offset, rule->declared_length);
        }
        return TEEIO_FAULT_DISPOSITION_MUTATE;
    case TEEIO_FAULT_ACTION_SET_DECLARED_LENGTH:
        if (*message_size < TEEIO_DOE_HEADER_SIZE) {
            return TEEIO_FAULT_DISPOSITION_ERROR;
        }
        write_u32(m_primary + TEEIO_DOE_LENGTH_OFFSET,
                  rule->declared_length);
        return TEEIO_FAULT_DISPOSITION_MUTATE;
    case TEEIO_FAULT_ACTION_DROP:
        return TEEIO_FAULT_DISPOSITION_DROP;
    case TEEIO_FAULT_ACTION_DUPLICATE:
        memcpy(m_secondary, m_primary, *message_size);
        return TEEIO_FAULT_DISPOSITION_DUPLICATE;
    case TEEIO_FAULT_ACTION_REPLAY:
        return TEEIO_FAULT_DISPOSITION_REPLAY;
    case TEEIO_FAULT_ACTION_REORDER:
        return TEEIO_FAULT_DISPOSITION_REORDER;
    case TEEIO_FAULT_ACTION_ABANDON:
        return TEEIO_FAULT_DISPOSITION_ABANDON;
    default:
        return TEEIO_FAULT_DISPOSITION_ERROR;
    }
}

static teeio_fault_result_t make_result(teeio_fault_disposition_t disposition,
                                        const uint8_t *message,
                                        size_t message_size, int rule_id)
{
    teeio_fault_result_t result;

    memset(&result, 0, sizeof(result));
    result.disposition = disposition;
    result.message = message;
    result.message_size = message_size;
    result.rule_id = rule_id;
    return result;
}

teeio_fault_result_t teeio_fault_apply(const void *message,
                                       size_t message_size,
                                       size_t message_capacity)
{
    const uint8_t *input;
    teeio_fault_rule_t *rule;
    teeio_fault_result_t result;
    teeio_fault_disposition_t disposition;
    uint32_t index;
    size_t output_size;
    bool requires_dword_alignment;
    bool selector_matched;

    if (message == NULL || message_size > TEEIO_FAULT_MAX_MESSAGE_SIZE ||
        message_size > message_capacity) {
        return make_result(TEEIO_FAULT_DISPOSITION_ERROR, message,
                           message_size, -1);
    }

    input = message;
    if (m_config == NULL || m_config->rule_count == 0 || m_fault_suspended) {
        return make_result(TEEIO_FAULT_DISPOSITION_PASS, input,
                           message_size, -1);
    }

    for (index = 0; index < m_config->rule_count; index++) {
        rule = &m_config->rules[index];
        if (!rule_matches(rule, input, message_size, &selector_matched)) {
            if (selector_matched) {
                memcpy(m_previous[index], input, message_size);
                m_previous_size[index] = message_size;
            }
            continue;
        }

        memcpy(m_primary, input, message_size);
        output_size = message_size;
        requires_dword_alignment =
            input[2] != TEEIO_FAULT_DOE_TYPE_PLAIN_SPDM &&
            input[2] != TEEIO_FAULT_DOE_TYPE_PLAIN_SECURED_SPDM;
        disposition = apply_mutation(rule, &output_size, message_capacity);

        if (disposition == TEEIO_FAULT_DISPOSITION_REPLAY) {
            if (m_previous_size[index] == 0 ||
                m_previous_size[index] > message_capacity) {
                disposition = TEEIO_FAULT_DISPOSITION_ERROR;
            } else {
                output_size = m_previous_size[index];
                memcpy(m_primary, m_previous[index], output_size);
                if (rule->pattern_size != 0) {
                    size_t replay_offset = rule->offset;
                    if (rule->offset_from_end) {
                        if (rule->offset == 0 || rule->offset > output_size) {
                            disposition = TEEIO_FAULT_DISPOSITION_ERROR;
                        } else {
                            replay_offset = output_size - rule->offset;
                        }
                    }
                    if (disposition != TEEIO_FAULT_DISPOSITION_ERROR &&
                        replay_offset + rule->pattern_size <= output_size) {
                        memcpy(m_primary + replay_offset, rule->pattern,
                               rule->pattern_size);
                    } else {
                        disposition = TEEIO_FAULT_DISPOSITION_ERROR;
                    }
                }
            }
        } else if (disposition == TEEIO_FAULT_DISPOSITION_REORDER) {
            if (m_held_size == 0) {
                memcpy(m_held, input, message_size);
                m_held_size = message_size;
                disposition = TEEIO_FAULT_DISPOSITION_HOLD;
            } else {
                memcpy(m_secondary, m_held, m_held_size);
            }
        }

        if (disposition == TEEIO_FAULT_DISPOSITION_ERROR ||
            ((disposition == TEEIO_FAULT_DISPOSITION_MUTATE ||
              disposition == TEEIO_FAULT_DISPOSITION_DUPLICATE ||
              disposition == TEEIO_FAULT_DISPOSITION_REPLAY ||
              disposition == TEEIO_FAULT_DISPOSITION_REORDER) &&
             requires_dword_alignment && (output_size & 3) != 0)) {
            snprintf(m_audit, sizeof(m_audit),
                     "{\"scenario\":\"%s\",\"rule\":%d,\"action\":\"%s\",\"status\":\"rejected\"}",
                     rule->scenario, rule->id,
                     teeio_fault_action_name(rule->action));
            return make_result(TEEIO_FAULT_DISPOSITION_ERROR, input,
                               message_size, rule->id);
        }

        m_total_fire_count++;
        snprintf(m_active_scenario, sizeof(m_active_scenario), "%s",
             rule->scenario);
        snprintf(m_active_expected, sizeof(m_active_expected), "%s",
             rule->expected);
        m_active_rule_id = rule->id;
        m_active_disposition = disposition;
        m_active_match_count = rule->match_count;
        m_active_recovery = rule->recovery;
        m_same_session_recovery_checked = false;
        m_same_session_recovery_passed = false;
        m_actual[0] = '\0';
        m_waiting_for_actual = true;
        result = make_result(disposition, m_primary, output_size, rule->id);
        if (disposition == TEEIO_FAULT_DISPOSITION_DROP ||
            disposition == TEEIO_FAULT_DISPOSITION_ABANDON ||
            disposition == TEEIO_FAULT_DISPOSITION_HOLD) {
            result.message = NULL;
            result.message_size = 0;
        } else if (disposition == TEEIO_FAULT_DISPOSITION_DUPLICATE) {
            result.secondary_message = m_secondary;
            result.secondary_message_size = output_size;
        } else if (disposition == TEEIO_FAULT_DISPOSITION_REORDER) {
            result.secondary_message = m_secondary;
            result.secondary_message_size = m_held_size;
            m_held_size = 0;
        }

        snprintf(m_audit, sizeof(m_audit),
             "{\"scenario\":\"%s\",\"rule\":%d,\"direction\":\"%s\",\"action\":\"%s\",\"match\":%u,\"input_size\":%zu,\"output_size\":%zu,\"doe_length\":%u,\"expected\":\"%s\",\"status\":\"fired\"}",
                 rule->scenario, rule->id,
                 "send",
                 teeio_fault_action_name(rule->action), rule->match_count,
                 message_size, result.message_size,
                 output_size >= TEEIO_DOE_HEADER_SIZE ?
             read_u32(m_primary + TEEIO_DOE_LENGTH_OFFSET) : 0,
             rule->expected);

        memcpy(m_previous[index], input, message_size);
        m_previous_size[index] = message_size;
        return result;
    }
    return make_result(TEEIO_FAULT_DISPOSITION_PASS, input, message_size, -1);
}

bool teeio_fault_scenario_fired(void)
{
    return m_total_fire_count != 0;
}

uint32_t teeio_fault_fire_count(void)
{
    return m_total_fire_count;
}

bool teeio_fault_same_session_recovery_configured(void)
{
    uint32_t index;

    if (m_config == NULL) {
        return false;
    }
    for (index = 0; index < m_config->rule_count; index++) {
        if (m_config->rules[index].recovery ==
            TEEIO_FAULT_RECOVERY_SAME_SESSION) {
            return true;
        }
    }
    return false;
}

bool teeio_fault_begin_same_session_preflight(void)
{
    if (!teeio_fault_same_session_recovery_configured() ||
        m_total_fire_count != 0) {
        return false;
    }
    m_same_session_preflight = true;
    return true;
}

void teeio_fault_end_same_session_preflight(void)
{
    m_same_session_preflight = false;
}

bool teeio_fault_begin_same_session_recovery(void)
{
    if (m_active_recovery != TEEIO_FAULT_RECOVERY_SAME_SESSION) {
        return false;
    }
    m_waiting_for_actual = false;
    m_fault_suspended = true;
    return true;
}

void teeio_fault_record_same_session_recovery(bool succeeded)
{
    if (m_active_recovery != TEEIO_FAULT_RECOVERY_SAME_SESSION) {
        return;
    }
    m_same_session_recovery_checked = true;
    m_same_session_recovery_passed = succeeded;
    m_fault_suspended = false;
}

bool teeio_fault_same_session_recovery_succeeded(void)
{
    if (!teeio_fault_same_session_recovery_configured()) {
        return true;
    }
    return m_active_recovery == TEEIO_FAULT_RECOVERY_SAME_SESSION &&
           m_same_session_recovery_checked &&
           m_same_session_recovery_passed;
}

const char *teeio_fault_audit_record(void)
{
    return m_audit;
}

bool teeio_fault_should_record_response(void)
{
    return m_waiting_for_actual;
}

void teeio_fault_record_actual(const char *actual)
{
    if (!m_waiting_for_actual || actual == NULL || actual[0] == '\0') {
        return;
    }
    snprintf(m_actual, sizeof(m_actual), "%s", actual);
    m_waiting_for_actual = false;
}

const char *teeio_fault_result_record(void)
{
    const char *same_session_recovery;

    if (m_active_recovery != TEEIO_FAULT_RECOVERY_SAME_SESSION) {
        same_session_recovery = "not_required";
    } else if (!m_same_session_recovery_checked) {
        same_session_recovery = "not_run";
    } else {
        same_session_recovery = m_same_session_recovery_passed ? "pass" : "fail";
    }
    snprintf(m_result, sizeof(m_result),
             "{\"scenario\":\"%s\",\"rule_id\":%d,\"disposition\":\"%s\",\"match_count\":%u,\"expected\":\"%s\",\"actual\":\"%s\",\"fired\":%s,\"fire_count\":%u,\"same_session_recovery\":\"%s\"}",
             m_active_scenario,
             m_active_rule_id,
             teeio_fault_disposition_name(m_active_disposition),
             m_active_match_count,
             m_active_expected,
             m_actual[0] == '\0' ? "not_recorded" : m_actual,
             m_total_fire_count == 0 ? "false" : "true",
             m_total_fire_count,
             same_session_recovery);
    return m_result;
}