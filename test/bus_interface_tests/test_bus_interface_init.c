#include "unity.h"
#include "bus_interface.h"
#include "status_code.h"

#include "bus_interface_test_helper.h"

TEST_FILE("bus_interface.c")

void setUp(void)
{
}

void tearDown(void)
{
}

void test_bus_interface_init(void)
{
  test_bus_data_ctx_t ctx = {0};
  bus_interface_t bus_interface = {0};
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_init(&bus_interface, stub_bus_read, stub_bus_write, &ctx));
}

void test_bus_interface_init__null_ptr_bus_interface(void)
{
  test_bus_data_ctx_t ctx = {0};
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, bus_interface_init(NULL, stub_bus_read, stub_bus_write, &ctx));
}

void test_bus_interface_init__null_ptr_bus_read(void)
{
  test_bus_data_ctx_t ctx = {0};
  bus_interface_t bus_interface = {0};
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, bus_interface_init(&bus_interface, NULL, stub_bus_write, &ctx));
}

void test_bus_interface_init__null_ptr_bus_write(void)
{
  test_bus_data_ctx_t ctx = {0};
  bus_interface_t bus_interface = {0};
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, bus_interface_init(&bus_interface, stub_bus_read, NULL, &ctx));
}

void test_bus_interface_init__null_ptr_resource(void)
{
  bus_interface_t bus_interface = {0};
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_init(&bus_interface, stub_bus_read, stub_bus_write, NULL));
}