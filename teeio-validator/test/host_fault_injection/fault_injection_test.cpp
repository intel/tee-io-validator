/**
 *  Copyright Notice:
 *  Copyright 2026 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include <cstring>

#include <gtest/gtest.h>

extern "C" {
#include "teeio_fault_injection.h"
}

namespace {

constexpr size_t kTestMessageSize = 16;
constexpr uint32_t kTestDoeTypeSpdm = 1;
constexpr uint32_t kTestSpdmCodeGetVersion = 0x84;

void MakeMessage(uint8_t *message, uint8_t marker)
{
    std::memset(message, 0, kTestMessageSize);
    message[0] = 0x01;
    message[2] = kTestDoeTypeSpdm;
    message[4] = kTestMessageSize / 4;
    message[8] = 0x14;
    message[9] = kTestSpdmCodeGetVersion;
    message[12] = marker;
}

teeio_fault_rule_t *InitRule(IDE_TEST_FAULT_CONFIG *config,
                             teeio_fault_action_t action)
{
    std::memset(config, 0, sizeof(*config));
    config->rule_count = 1;

    teeio_fault_rule_t *rule = &config->rules[0];
    rule->id = 1;
    std::strcpy(rule->scenario, "test_case");
    std::strcpy(rule->expected, "controlled_result");
    rule->doe_type = kTestDoeTypeSpdm;
    rule->spdm_code = kTestSpdmCodeGetVersion;
    rule->occurrence = 1;
    rule->action = action;
    return rule;
}

TEST(FaultInjectionParseTest, ParsesActionsAndPatterns)
{
    teeio_fault_action_t action;
    uint8_t pattern[4];

    EXPECT_TRUE(teeio_fault_parse_action("reorder", &action));
    EXPECT_EQ(action, TEEIO_FAULT_ACTION_REORDER);
    EXPECT_FALSE(teeio_fault_parse_action("invalid", &action));
    ASSERT_EQ(teeio_fault_parse_pattern("0xde:ad be:ef", pattern,
                                        sizeof(pattern)), 4);
    EXPECT_EQ(pattern[0], 0xde);
    EXPECT_EQ(pattern[1], 0xad);
    EXPECT_EQ(pattern[2], 0xbe);
    EXPECT_EQ(pattern[3], 0xef);
    EXPECT_EQ(teeio_fault_parse_pattern("abc", pattern, sizeof(pattern)), -1);

    teeio_fault_recovery_t recovery;
    EXPECT_TRUE(teeio_fault_parse_recovery("same_session", &recovery));
    EXPECT_EQ(recovery, TEEIO_FAULT_RECOVERY_SAME_SESSION);
    EXPECT_TRUE(teeio_fault_parse_recovery("none", &recovery));
    EXPECT_EQ(recovery, TEEIO_FAULT_RECOVERY_NONE);
    EXPECT_FALSE(teeio_fault_parse_recovery("new_session", &recovery));
}

TEST(FaultInjectionRuleTest, RejectsSetWithoutPattern)
{
    IDE_TEST_FAULT_CONFIG config;
    teeio_fault_rule_t *rule = InitRule(&config, TEEIO_FAULT_ACTION_SET);
    char error[128];

    EXPECT_FALSE(teeio_fault_validate_rule(rule, error, sizeof(error)));
    EXPECT_NE(std::strstr(error, "requires a pattern"), nullptr);
    rule->pattern[0] = 0xaa;
    rule->pattern_size = 1;
    EXPECT_TRUE(teeio_fault_validate_rule(rule, error, sizeof(error)));
}

TEST(FaultInjectionStateTest, EnabledRequiresConfiguredRule)
{
    IDE_TEST_FAULT_CONFIG config = {};

    teeio_fault_init(&config);
    EXPECT_FALSE(teeio_fault_is_enabled());
    EXPECT_FALSE(teeio_fault_scenario_is("test_case"));
    InitRule(&config, TEEIO_FAULT_ACTION_DROP);
    teeio_fault_init(&config);
    EXPECT_TRUE(teeio_fault_is_enabled());
    EXPECT_TRUE(teeio_fault_scenario_is("test_case"));
    EXPECT_FALSE(teeio_fault_scenario_is("other_case"));
}

TEST(FaultInjectionSelectorTest, MatchesSelectorsAndOccurrence)
{
    IDE_TEST_FAULT_CONFIG config;
    teeio_fault_rule_t *rule = InitRule(&config, TEEIO_FAULT_ACTION_SET);
    uint8_t message[kTestMessageSize];

    MakeMessage(message, 0x10);
    rule->occurrence = 2;
    rule->offset = 10;
    rule->pattern[0] = 0x5a;
    rule->pattern_size = 1;
    teeio_fault_init(&config);

    message[2] = kTestDoeTypeSpdm + 1;
    teeio_fault_result_t result = teeio_fault_apply(
        message, sizeof(message), sizeof(message));
    EXPECT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_PASS);
    message[2] = kTestDoeTypeSpdm;
    result = teeio_fault_apply(message,
                               sizeof(message), sizeof(message));
    EXPECT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_PASS);
    result = teeio_fault_apply(message,
                               sizeof(message), sizeof(message));
    ASSERT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_MUTATE);
    EXPECT_EQ(result.message[10], 0x5a);
    EXPECT_EQ(message[10], 0);
    EXPECT_EQ(teeio_fault_fire_count(), 1U);
    EXPECT_NE(std::strstr(teeio_fault_audit_record(), "\"status\":\"fired\""),
              nullptr);
    EXPECT_NE(std::strstr(teeio_fault_audit_record(),
                          "\"expected\":\"controlled_result\""), nullptr);
    teeio_fault_record_actual("spdm_error_0x01");
    EXPECT_NE(std::strstr(teeio_fault_result_record(),
                          "\"actual\":\"spdm_error_0x01\""), nullptr);
    EXPECT_NE(std::strstr(teeio_fault_result_record(), "\"rule_id\":1"), nullptr);
    EXPECT_NE(std::strstr(teeio_fault_result_record(),
                          "\"disposition\":\"mutate\""), nullptr);
    EXPECT_NE(std::strstr(teeio_fault_result_record(), "\"match_count\":2"),
              nullptr);
}

TEST(FaultInjectionMutationTest, AppliesXorAndEndRelativeOffsets)
{
    IDE_TEST_FAULT_CONFIG config;
    teeio_fault_rule_t *rule = InitRule(&config, TEEIO_FAULT_ACTION_XOR);
    uint8_t message[kTestMessageSize];

    MakeMessage(message, 0x11);
    rule->offset = 12;
    rule->pattern[0] = 0xff;
    rule->pattern_size = 1;
    teeio_fault_init(&config);
    teeio_fault_result_t result = teeio_fault_apply(
        message, sizeof(message), sizeof(message));
    ASSERT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_MUTATE);
    EXPECT_TRUE(teeio_fault_should_record_response());
    EXPECT_EQ(result.message[12], static_cast<uint8_t>(0x11 ^ 0xff));

    rule->offset = 1;
    rule->offset_from_end = true;
    teeio_fault_reset();
    result = teeio_fault_apply(message,
                               sizeof(message), sizeof(message));
    ASSERT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_MUTATE);
    EXPECT_EQ(result.message[15], 0xff);
    teeio_fault_record_actual("spdm_response_0x04");
    EXPECT_FALSE(teeio_fault_should_record_response());
}

TEST(FaultInjectionResizeTest, ResizesAndRejectsNonAtomicMutation)
{
    IDE_TEST_FAULT_CONFIG config;
    teeio_fault_rule_t *rule = InitRule(&config, TEEIO_FAULT_ACTION_TRUNCATE);
    uint8_t message[kTestMessageSize];

    MakeMessage(message, 0x12);
    rule->size = 4;
    teeio_fault_init(&config);
    teeio_fault_result_t result = teeio_fault_apply(
        message, sizeof(message), sizeof(message));
    ASSERT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_MUTATE);
    EXPECT_EQ(result.message_size, 12U);
    EXPECT_EQ(result.message[4], 3);

    rule->action = TEEIO_FAULT_ACTION_TRUNCATE_TO;
    rule->size = 12;
    teeio_fault_reset();
    result = teeio_fault_apply(message,
                               sizeof(message), sizeof(message));
    EXPECT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_MUTATE);
    EXPECT_EQ(result.message_size, 12U);
    EXPECT_EQ(result.message[4], 3);

    rule->action = TEEIO_FAULT_ACTION_TRUNCATE;
    rule->size = 1;
    teeio_fault_reset();
    result = teeio_fault_apply(message,
                               sizeof(message), sizeof(message));
    EXPECT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_ERROR);
    EXPECT_EQ(result.message, message);
    EXPECT_EQ(result.message_size, sizeof(message));

    rule->action = TEEIO_FAULT_ACTION_EXTEND;
    rule->size = 0;
    rule->offset = 8;
    rule->declared_length = 0x12345678;
    rule->pattern[0] = 0xde;
    rule->pattern[1] = 0xad;
    rule->pattern[2] = 0xbe;
    rule->pattern[3] = 0xef;
    rule->pattern_size = 4;
    teeio_fault_reset();
    result = teeio_fault_apply(message,
                               sizeof(message), sizeof(message) + 4);
    ASSERT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_MUTATE);
    ASSERT_EQ(result.message_size, 20U);
    EXPECT_EQ(result.message[4], 5);
    EXPECT_EQ(result.message[8], 0x78);
    EXPECT_EQ(result.message[9], 0x56);
    EXPECT_EQ(result.message[10], 0x34);
    EXPECT_EQ(result.message[11], 0x12);
    EXPECT_EQ(result.message[16], 0xde);
    EXPECT_EQ(result.message[19], 0xef);
}

TEST(FaultInjectionLengthTest, ReplacesDoeDeclaredLength)
{
    IDE_TEST_FAULT_CONFIG config;
    teeio_fault_rule_t *rule = InitRule(
        &config, TEEIO_FAULT_ACTION_SET_DECLARED_LENGTH);
    uint8_t message[kTestMessageSize];

    MakeMessage(message, 0x13);
    rule->declared_length = 0x12345678;
    teeio_fault_init(&config);
    teeio_fault_result_t result = teeio_fault_apply(
        message, sizeof(message), sizeof(message));
    ASSERT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_MUTATE);
    EXPECT_EQ(result.message[4], 0x78);
    EXPECT_EQ(result.message[5], 0x56);
    EXPECT_EQ(result.message[6], 0x34);
    EXPECT_EQ(result.message[7], 0x12);
    EXPECT_NE(std::strstr(teeio_fault_audit_record(),
                          "\"doe_length\":305419896"), nullptr);
}

TEST(FaultInjectionDispositionTest, DropsAndAbandonsMessages)
{
    IDE_TEST_FAULT_CONFIG config;
    teeio_fault_rule_t *rule = InitRule(&config, TEEIO_FAULT_ACTION_DROP);
    uint8_t message[kTestMessageSize];

    MakeMessage(message, 0x14);
    teeio_fault_init(&config);
    teeio_fault_result_t result = teeio_fault_apply(
        message, sizeof(message), sizeof(message));
    EXPECT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_DROP);
    EXPECT_EQ(result.message, nullptr);
    EXPECT_EQ(result.message_size, 0U);

    rule->action = TEEIO_FAULT_ACTION_ABANDON;
    teeio_fault_reset();
    result = teeio_fault_apply(message,
                               sizeof(message), sizeof(message));
    EXPECT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_ABANDON);
}

TEST(FaultInjectionSequenceTest, DuplicatesMessages)
{
    IDE_TEST_FAULT_CONFIG config;
    uint8_t message[kTestMessageSize];

    MakeMessage(message, 0x15);
    InitRule(&config, TEEIO_FAULT_ACTION_DUPLICATE);
    teeio_fault_init(&config);
    teeio_fault_result_t result = teeio_fault_apply(
        message, sizeof(message), sizeof(message));
    ASSERT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_DUPLICATE);
    ASSERT_EQ(result.secondary_message_size, sizeof(message));
    EXPECT_EQ(std::memcmp(result.message, result.secondary_message,
                          sizeof(message)), 0);
}

TEST(FaultInjectionSequenceTest, ReplaysEarlierMessage)
{
    IDE_TEST_FAULT_CONFIG config;
    teeio_fault_rule_t *rule = InitRule(&config, TEEIO_FAULT_ACTION_REPLAY);
    uint8_t first[kTestMessageSize];
    uint8_t unrelated[kTestMessageSize];
    uint8_t second[kTestMessageSize];

    MakeMessage(first, 0x16);
    MakeMessage(unrelated, 0x36);
    MakeMessage(second, 0x26);
    first[2] = TEEIO_FAULT_DOE_TYPE_PLAIN_SECURED_SPDM;
    second[2] = TEEIO_FAULT_DOE_TYPE_PLAIN_SECURED_SPDM;
    rule->doe_type = TEEIO_FAULT_DOE_TYPE_PLAIN_SECURED_SPDM;
    rule->occurrence = 2;
    rule->offset = 8;
    rule->pattern[0] = 0x13;
    rule->pattern_size = 1;
    teeio_fault_init(&config);
    teeio_fault_result_t result = teeio_fault_apply(
        first, sizeof(first), sizeof(first));
    EXPECT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_PASS);
    result = teeio_fault_apply(unrelated,
                               sizeof(unrelated), sizeof(unrelated));
    EXPECT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_PASS);
    result = teeio_fault_apply(second,
                               sizeof(second), sizeof(second));
    ASSERT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_REPLAY);
    EXPECT_EQ(result.message[8], 0x13);
    EXPECT_EQ(result.message[12], 0x16);
}

TEST(FaultInjectionRecoveryTest, RecordsSameSessionRecovery)
{
    IDE_TEST_FAULT_CONFIG config;
    uint8_t message[kTestMessageSize];

    MakeMessage(message, 0x27);
    teeio_fault_rule_t *rule = InitRule(&config, TEEIO_FAULT_ACTION_DROP);
    rule->recovery = TEEIO_FAULT_RECOVERY_SAME_SESSION;
    teeio_fault_init(&config);
    EXPECT_TRUE(teeio_fault_same_session_recovery_configured());
    EXPECT_TRUE(teeio_fault_begin_same_session_preflight());
    teeio_fault_result_t result = teeio_fault_apply(
        message, sizeof(message), sizeof(message));
    EXPECT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_PASS);
    EXPECT_EQ(rule->match_count, 0U);
    teeio_fault_end_same_session_preflight();
    teeio_fault_apply(message,
                      sizeof(message), sizeof(message));
    EXPECT_TRUE(teeio_fault_begin_same_session_recovery());
    EXPECT_FALSE(teeio_fault_same_session_recovery_succeeded());
    result = teeio_fault_apply(message,
                               sizeof(message), sizeof(message));
    EXPECT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_PASS);
    EXPECT_EQ(teeio_fault_fire_count(), 1U);
    teeio_fault_record_same_session_recovery(true);
    EXPECT_TRUE(teeio_fault_same_session_recovery_succeeded());
    EXPECT_NE(std::strstr(teeio_fault_result_record(),
                          "\"same_session_recovery\":\"pass\""), nullptr);
    teeio_fault_reset();
    EXPECT_FALSE(teeio_fault_scenario_fired());
    EXPECT_FALSE(teeio_fault_same_session_recovery_succeeded());
}

TEST(FaultInjectionSelectorTest, MatchesPlainSecuredSpdm)
{
    IDE_TEST_FAULT_CONFIG config;
    teeio_fault_rule_t *rule = InitRule(&config, TEEIO_FAULT_ACTION_XOR);
    uint8_t message[kTestMessageSize];

    MakeMessage(message, 0x19);
    message[2] = TEEIO_FAULT_DOE_TYPE_PLAIN_SECURED_SPDM;
    message[9] = 0xe5;
    rule->doe_type = TEEIO_FAULT_DOE_TYPE_PLAIN_SECURED_SPDM;
    rule->spdm_code = 0xe5;
    rule->offset = 1;
    rule->offset_from_end = true;
    rule->pattern[0] = 1;
    rule->pattern_size = 1;
    teeio_fault_init(&config);
    teeio_fault_result_t result = teeio_fault_apply(
        message, sizeof(message), sizeof(message));
    ASSERT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_MUTATE);
    EXPECT_EQ(result.message[15], 1);
}

TEST(FaultInjectionSequenceTest, ReordersTwoMessages)
{
    IDE_TEST_FAULT_CONFIG config;
    teeio_fault_rule_t *rule = InitRule(&config, TEEIO_FAULT_ACTION_REORDER);
    uint8_t first[kTestMessageSize];
    uint8_t second[kTestMessageSize];

    MakeMessage(first, 0x17);
    MakeMessage(second, 0x27);
    rule->occurrence = TEEIO_FAULT_OCCURRENCE_ALL;
    teeio_fault_init(&config);
    teeio_fault_result_t result = teeio_fault_apply(
        first, sizeof(first), sizeof(first));
    EXPECT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_HOLD);
    result = teeio_fault_apply(second,
                               sizeof(second), sizeof(second));
    ASSERT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_REORDER);
    EXPECT_EQ(result.message[12], 0x27);
    ASSERT_NE(result.secondary_message, nullptr);
    EXPECT_EQ(result.secondary_message[12], 0x17);
}

TEST(FaultInjectionStateTest, ResetClearsScenarioState)
{
    IDE_TEST_FAULT_CONFIG config;
    teeio_fault_rule_t *rule = InitRule(&config, TEEIO_FAULT_ACTION_DROP);
    uint8_t message[kTestMessageSize];

    MakeMessage(message, 0x18);
    rule->occurrence = 2;
    teeio_fault_init(&config);
    teeio_fault_apply(message,
                      sizeof(message), sizeof(message));
    teeio_fault_reset();
    EXPECT_FALSE(teeio_fault_scenario_fired());
    EXPECT_EQ(teeio_fault_fire_count(), 0U);
    teeio_fault_result_t result = teeio_fault_apply(
        message, sizeof(message), sizeof(message));
    EXPECT_EQ(result.disposition, TEEIO_FAULT_DISPOSITION_PASS);
}

}  // namespace