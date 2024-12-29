#include "unity.h"
#include "ring_buf.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

TEST_FILE("ring_buf.c")

void setUp(void)
{
}

void tearDown(void)
{
}

void stub_test_assert_buffer_state(ring_buffer_t *buf, size_t expected_size, bool is_empty, bool is_full)
{
  TEST_ASSERT_EQUAL_INT(expected_size, ring_buffer_size(buf));

  if (is_full)
  {
    TEST_ASSERT_TRUE(ring_buffer_full(buf));
  }
  else
  {
    TEST_ASSERT_FALSE(ring_buffer_full(buf));
  }

  if (is_empty)
  {
    TEST_ASSERT_TRUE(ring_buffer_empty(buf));
  }
  else
  {
    TEST_ASSERT_FALSE(ring_buffer_empty(buf));
  }
}

void test_ring_buffer_init_happy_path(void)
{
  uint8_t storage[5];
  ring_buffer_t buf;

  TEST_ASSERT_TRUE(INIT_RING_BUFFER(buf, storage));

  /* Verify pointers' initial values and capacity */
  TEST_ASSERT_EQUAL_INT(0, buf.write_ptr);
  TEST_ASSERT_EQUAL_INT(0, buf.read_ptr);
  TEST_ASSERT_EQUAL_INT(5, buf.capacity);

  stub_test_assert_buffer_state(&buf, 0, true, false);
}

void test_ring_buffer_init_null_ptr(void)
{
  uint8_t storage[5];
  ring_buffer_t buf;

  TEST_ASSERT_FALSE(ring_buffer_init(NULL, storage, sizeof(storage[0]), sizeof(storage)));
  TEST_ASSERT_FALSE(ring_buffer_init(&buf, NULL, sizeof(storage[0]), sizeof(storage)));
}

void test_ring_buffer_init_max_capacity_exceeded(void)
{
  uint8_t storage[5];
  ring_buffer_t buf;

  TEST_ASSERT_FALSE(ring_buffer_init(&buf, storage, sizeof(storage[0]), 0xFFFF));
  TEST_ASSERT_FALSE(ring_buffer_init(&buf, storage, sizeof(storage[0]), 0x10000));
}

void test_ring_buffer_init_invalid_args(void)
{
  uint8_t storage[5];
  ring_buffer_t buf;

  TEST_ASSERT_FALSE(ring_buffer_init(&buf, storage, sizeof(storage[0]), 0));
  TEST_ASSERT_FALSE(ring_buffer_init(&buf, storage, 0, sizeof(storage)));
}

void test_ring_buffer_write_null_ptr(void)
{
  uint8_t data;
  uint8_t storage[5];
  ring_buffer_t buf;

  TEST_ASSERT_TRUE(INIT_RING_BUFFER(buf, storage));

  TEST_ASSERT_FALSE(ring_buffer_write(NULL, &data));
  TEST_ASSERT_FALSE(ring_buffer_write(&buf, NULL));
}

void test_ring_buffer_write_happy_path_basic(void)
{
  uint8_t data = 0x55;
  uint8_t storage[5];
  ring_buffer_t buf;

  TEST_ASSERT_TRUE(INIT_RING_BUFFER(buf, storage));

  TEST_ASSERT_TRUE(ring_buffer_write(&buf, &data));

  stub_test_assert_buffer_state(&buf, 1, false, false);
}

void test_ring_buffer_write_until_full(void)
{
  uint8_t data = 0x55;
  uint8_t storage[5];
  ring_buffer_t buf;

  TEST_ASSERT_TRUE(INIT_RING_BUFFER(buf, storage));

  for (int8_t i = 0; i < sizeof(storage); i++)
  {
    TEST_ASSERT_TRUE(ring_buffer_write(&buf, &data));
    TEST_ASSERT_EQUAL_INT(i + 1, ring_buffer_size(&buf));
    data ^= 0xFF;
  }

  stub_test_assert_buffer_state(&buf, sizeof(storage), false, true);
}

void test_ring_buffer_write_while_full(void)
{
  uint8_t data = 0x55;
  uint8_t storage[5];
  ring_buffer_t buf;

  TEST_ASSERT_TRUE(INIT_RING_BUFFER(buf, storage));

  for (int8_t i = 0; i < sizeof(storage); i++)
  {
    TEST_ASSERT_TRUE(ring_buffer_write(&buf, &data));
    TEST_ASSERT_EQUAL_INT(i + 1, ring_buffer_size(&buf));
    data ^= 0xFF;
  }

  stub_test_assert_buffer_state(&buf, sizeof(storage), false, true);

  /* Attempting to write should fail now */
  TEST_ASSERT_FALSE(ring_buffer_write(&buf, &data));
}

void test_ring_buffer_read_null_ptr(void)
{
  uint8_t data;
  uint8_t storage[5];
  ring_buffer_t buf;

  TEST_ASSERT_TRUE(INIT_RING_BUFFER(buf, storage));

  TEST_ASSERT_FALSE(ring_buffer_read(NULL, &data));
  TEST_ASSERT_FALSE(ring_buffer_read(&buf, NULL));
}

void test_ring_buffer_read_happy_path_basic(void)
{
  uint8_t data = 0x55;
  uint8_t data_read = 0x00;
  uint8_t storage[5];
  ring_buffer_t buf;

  TEST_ASSERT_TRUE(INIT_RING_BUFFER(buf, storage));

  TEST_ASSERT_TRUE(ring_buffer_write(&buf, &data));

  TEST_ASSERT_TRUE(ring_buffer_read(&buf, &data_read));
  TEST_ASSERT_EQUAL_HEX8(data, data_read);

  stub_test_assert_buffer_state(&buf, 0, true, false);
}

void test_ring_buffer_read_until_empty(void)
{
  uint8_t data;
  uint8_t data_read = 0x00;
  uint8_t storage[5];
  ring_buffer_t buf;

  TEST_ASSERT_TRUE(INIT_RING_BUFFER(buf, storage));

  for (int8_t i = 0; i < sizeof(storage); i++)
  {
    data = 0xA0 | i;
    TEST_ASSERT_TRUE(ring_buffer_write(&buf, &data));
  }

  stub_test_assert_buffer_state(&buf, sizeof(storage), false, true);

  for (int8_t i = 0; i < sizeof(storage); i++)
  {
    data = 0xA0 | i;
    TEST_ASSERT_TRUE(ring_buffer_read(&buf, &data_read));
    TEST_ASSERT_EQUAL_INT(sizeof(storage) - i - 1, ring_buffer_size(&buf));
    TEST_ASSERT_EQUAL_HEX8(data, data_read);
  }

  stub_test_assert_buffer_state(&buf, 0, true, false);
}

void test_ring_buffer_write_while_empty(void)
{
  uint8_t data;
  uint8_t storage[5];
  ring_buffer_t buf;

  TEST_ASSERT_TRUE(INIT_RING_BUFFER(buf, storage));
  TEST_ASSERT_FALSE(ring_buffer_read(&buf, &data));
}

void test_ring_buffer_write_multiple_bytes(void)
{
  uint32_t data = 0xDEADBEEF;
  uint32_t storage[3];
  ring_buffer_t buf;

  TEST_ASSERT_TRUE(INIT_RING_BUFFER(buf, storage));

  for (int8_t i = 0; i < ARRAY_SIZE(storage); i++)
  {
    TEST_ASSERT_TRUE(ring_buffer_write(&buf, &data));
    stub_test_assert_buffer_state(&buf, i + 1, false, (i == 2));
  }
}

void test_ring_buffer_read_multiple_bytes(void)
{
  uint32_t write_data[] = {0xDEADBEEF, 0xBADDF00D, 0x7357D47A};
  uint32_t read_data;
  uint32_t storage[3];
  ring_buffer_t buf;

  TEST_ASSERT_TRUE(INIT_RING_BUFFER(buf, storage));

  for (int8_t i = 0; i < ARRAY_SIZE(storage); i++)
  {
    TEST_ASSERT_TRUE(ring_buffer_write(&buf, &write_data[i]));
  }
  stub_test_assert_buffer_state(&buf, 3, 0, 1);

  for (int8_t i = 0; i < ARRAY_SIZE(storage); i++)
  {
    TEST_ASSERT_TRUE(ring_buffer_read(&buf, &read_data));
    TEST_ASSERT_EQUAL_HEX32(write_data[i], read_data);
    stub_test_assert_buffer_state(&buf, 2 - i, (i == 2), false);
  }
}

void test_ring_buffer_read_write_wrap_around_single_byte(void)
{
  uint8_t data[256];
  uint8_t read_data;
  uint8_t storage[73];
  uint16_t w_index = 0;
  uint16_t r_index = 0;
  ring_buffer_t buf;

  /* Initialize the data */
  for (uint16_t i = 0; i < ARRAY_SIZE(data); i++)
  {
    data[i] = (i & 0xFF);
  }

  TEST_ASSERT_TRUE(INIT_RING_BUFFER(buf, storage));

  while (w_index < ARRAY_SIZE(storage))
  {
    TEST_ASSERT_TRUE(ring_buffer_write(&buf, &data[w_index++]));
  }

  stub_test_assert_buffer_state(&buf, ARRAY_SIZE(storage), false, true);

  while (r_index < ARRAY_SIZE(data))
  {
    TEST_ASSERT_TRUE(ring_buffer_read(&buf, &read_data));
    TEST_ASSERT_EQUAL_HEX8(data[r_index++], read_data);

    if (w_index < ARRAY_SIZE(data))
    {
      TEST_ASSERT_TRUE(ring_buffer_write(&buf, &data[w_index++]));
    }
  }

  stub_test_assert_buffer_state(&buf, 0, true, false);
}

void test_ring_buffer_read_write_wrap_around_multiple_bytes(void)
{
  uint32_t data[256];
  uint32_t read_data;
  uint32_t storage[73];
  uint16_t w_index = 0;
  uint16_t r_index = 0;
  ring_buffer_t buf;

  /* Initialize the data */
  for (uint16_t i = 0; i < ARRAY_SIZE(data); i++)
  {
    data[i] = (0xBEEF << 16) | (((i & 0xFF) ^ 0xFF) << 0x8) | (i & 0xFF);
  }

  TEST_ASSERT_TRUE(INIT_RING_BUFFER(buf, storage));

  while (w_index < ARRAY_SIZE(storage))
  {
    TEST_ASSERT_TRUE(ring_buffer_write(&buf, &data[w_index++]));
  }

  stub_test_assert_buffer_state(&buf, ARRAY_SIZE(storage), false, true);

  while (r_index < ARRAY_SIZE(data))
  {
    TEST_ASSERT_TRUE(ring_buffer_read(&buf, &read_data));
    TEST_ASSERT_EQUAL_HEX8(data[r_index++], read_data);

    if (w_index < ARRAY_SIZE(data))
    {
      TEST_ASSERT_TRUE(ring_buffer_write(&buf, &data[w_index++]));
    }
  }

  stub_test_assert_buffer_state(&buf, 0, true, false);
}