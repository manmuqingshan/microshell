#include <stdio.h>
#include <string.h>

#include <unity.h>

#include "ush_internal.h"

struct ush_io_interface ush_io_iface;
struct ush_descriptor ush_desc;
struct ush_object ush;
int write_callback_call_count;
int write_callback_pattern_index;
int write_callback_return_val;
int read_callback_return_val;

int write_callback(struct ush_object *self, char ch);

void setUp(void)
{
        ush_io_iface.write = write_callback;
        ush_desc.io = &ush_io_iface;

        memset((uint8_t*)&ush, 0, sizeof(ush));

        ush.desc = &ush_desc;
        ush.write_buf = "test_message";

        write_callback_call_count = 0;
        write_callback_pattern_index = 0;
        write_callback_return_val = 0;
        read_callback_return_val = 0;
}

void tearDown(void)
{

}

int write_callback(struct ush_object *self, char ch)
{
        char *pattern = "test_message";

        TEST_ASSERT_EQUAL(&ush, self);
        TEST_ASSERT_EQUAL(pattern[write_callback_pattern_index], ch);

        write_callback_call_count++;
        write_callback_pattern_index += write_callback_return_val;

        return write_callback_return_val;
}

void test_ush_write_pointer(void)
{
        char *data = (char*)"test_string";
        
        ush.write_pos = -1;
        ush_write_pointer(&ush, data, USH_STATE__TOTAL_NUM);
        TEST_ASSERT_EQUAL(0, ush.write_pos);
        TEST_ASSERT_EQUAL(11, ush.write_size);
        TEST_ASSERT_EQUAL(data, ush.write_buf);
        TEST_ASSERT_EQUAL(USH_STATE_WRITE_CHAR, ush.state);
        TEST_ASSERT_EQUAL(USH_STATE__TOTAL_NUM, ush.write_next_state);
}

void test_ush_write_pointer_bin(void)
{
        uint8_t *data = (uint8_t*)1234;
        
        ush.write_pos = -1;
        ush_write_pointer_bin(&ush, data, 4, USH_STATE__TOTAL_NUM);
        TEST_ASSERT_EQUAL(0, ush.write_pos);
        TEST_ASSERT_EQUAL(4, ush.write_size);
        TEST_ASSERT_EQUAL((uint8_t*)1234, ush.write_buf);
        TEST_ASSERT_EQUAL(USH_STATE_WRITE_CHAR, ush.state);
        TEST_ASSERT_EQUAL(USH_STATE__TOTAL_NUM, ush.write_next_state);
}

void test_ush_write_char(void)
{
        for (int i = 0; i < USH_STATE__TOTAL_NUM; i++) {
                ush_state_t state = (ush_state_t)i;
                
                setUp();

                ush.state = USH_STATE__TOTAL_NUM;
                ush.write_pos = 1;
                ush.write_size = 1;
                ush.write_next_state = state;
                ush_write_char(&ush);

                TEST_ASSERT_EQUAL(state, ush.state);
                TEST_ASSERT_EQUAL(1, ush.write_pos);
                TEST_ASSERT_EQUAL(1, ush.write_size);

                TEST_ASSERT_EQUAL_INT(0, write_callback_call_count);
        }

        setUp();

        ush.write_pos = 0;
        ush.write_size = 4;
        write_callback_return_val = 1;
        for (int i = 0; i < 4; i++)
                ush_write_char(&ush);

        TEST_ASSERT_EQUAL(0, ush.state);
        TEST_ASSERT_EQUAL(4, ush.write_pos);
        TEST_ASSERT_EQUAL(4, ush.write_size);
        TEST_ASSERT_EQUAL_INT(4, write_callback_call_count);

        setUp();

        ush.write_pos = 0;
        ush.write_size = 4;
        write_callback_return_val = 0;
        for (int i = 0; i < 4; i++)
                ush_write_char(&ush);

        TEST_ASSERT_EQUAL(0, ush.state);
        TEST_ASSERT_EQUAL(0, ush.write_pos);
        TEST_ASSERT_EQUAL(4, ush.write_size);
        TEST_ASSERT_EQUAL_INT(4, write_callback_call_count);        
}

void test_ush_write_service(void)
{
        for (int i = 0; i < USH_STATE__TOTAL_NUM; i++) {
                setUp();
                ush.state = (ush_state_t)i;
                ush.write_pos = 0;
                ush.write_size = 4;
                write_callback_return_val = 1;
                switch (ush.state) {
                case USH_STATE_WRITE_CHAR:
                        TEST_ASSERT_TRUE(ush_write_service(&ush));
                        TEST_ASSERT_EQUAL_INT(1, write_callback_call_count);
                        TEST_ASSERT_EQUAL(1, ush.write_pos);
                        TEST_ASSERT_EQUAL(4, ush.write_size);
                        break;
                default:
                        TEST_ASSERT_FALSE(ush_write_service(&ush));
                        TEST_ASSERT_EQUAL_INT(0, write_callback_call_count);
                        TEST_ASSERT_EQUAL(0, ush.write_pos);
                        TEST_ASSERT_EQUAL(4, ush.write_size);
                        break;
                }
        }
}

int main(int argc, char *argv[])
{
        (void)argc;
        (void)argv;

        UNITY_BEGIN();

        RUN_TEST(test_ush_write_pointer);
        RUN_TEST(test_ush_write_pointer_bin);
        RUN_TEST(test_ush_write_char);
        RUN_TEST(test_ush_write_service);

        return UNITY_END();
}