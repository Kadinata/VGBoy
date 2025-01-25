#include "unity.h"
#include "dma.h"
#include "status_code.h"

#include "bus_interface_test_helper.h"
#include "mock_bus_interface.h"

TEST_FILE("dma.c")

void stub_init_bus_interface(bus_interface_t *bus_interface, test_bus_data_ctx_t *ctx)
{
  bus_interface->read = stub_bus_read;
  bus_interface->write = stub_bus_write;
  bus_interface->resource = ctx;

  if (ctx)
  {
    ctx->return_status = STATUS_OK;
  }
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_dma_init_null_ptr(void)
{
  dma_handle_t dma_handle = {0};
  bus_interface_t bus_interface = {0};

  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, dma_init(NULL, bus_interface));

  stub_init_bus_interface(&bus_interface, NULL);
  bus_interface.read = NULL;
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, dma_init(&dma_handle, bus_interface));

  stub_init_bus_interface(&bus_interface, NULL);
  bus_interface.write = NULL;
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, dma_init(&dma_handle, bus_interface));
}

void test_dma_init(void)
{
  dma_handle_t dma_handle = {0};
  bus_interface_t bus_interface = {0};
  stub_init_bus_interface(&bus_interface, NULL);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_init(&dma_handle, bus_interface));
  TEST_ASSERT_EQUAL_INT(DMA_IDLE, dma_handle.state);
}

void test_dma_start_null_ptr(void)
{
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, dma_start(NULL, 0));
}

void test_dma_start(void)
{
  dma_handle_t dma_handle = {0};
  TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_start(&dma_handle, 0x23));

  TEST_ASSERT_EQUAL_INT(DMA_PREPARING, dma_handle.state);
  TEST_ASSERT_EQUAL_INT(0x23 * 0x100, dma_handle.starting_addr);
  TEST_ASSERT_EQUAL_INT(0, dma_handle.current_offset);
  TEST_ASSERT_EQUAL_INT(2, dma_handle.prep_delay);
}

void test_dma_tick_null_ptr(void)
{
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, dma_tick(NULL));
}

void test_dma_tick_while_idle(void)
{
  /* Setup */
  dma_handle_t dma_handle = {0};
  bus_interface_t bus_interface = {0};
  test_bus_data_ctx_t bus_data_ctx = {0};
  stub_init_bus_interface(&bus_interface, &bus_data_ctx);

  /* Initialize DMA */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_init(&dma_handle, bus_interface));

  TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_tick(&dma_handle));
  TEST_ASSERT_EQUAL_INT(DMA_IDLE, dma_handle.state);
}

void test_dma_tick(void)
{
  /* Setup */
  uint8_t dma_data = 0x55;
  dma_handle_t dma_handle = {0};
  bus_interface_t bus_interface = {0};
  stub_init_bus_interface(&bus_interface, NULL);

  /* Initialize DMA */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_init(&dma_handle, bus_interface));

  /* start DMA */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_start(&dma_handle, 0x23));

  /* No transfer should have occurred in the first two ticks */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_tick(&dma_handle));
  TEST_ASSERT_EQUAL_INT(DMA_PREPARING, dma_handle.state);
  TEST_ASSERT_EQUAL_INT(0x23 * 0x100, dma_handle.starting_addr);
  TEST_ASSERT_EQUAL_INT(0, dma_handle.current_offset);
  TEST_ASSERT_EQUAL_INT(1, dma_handle.prep_delay);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_tick(&dma_handle));
  TEST_ASSERT_EQUAL_INT(DMA_XFER_ACTIVE, dma_handle.state);
  TEST_ASSERT_EQUAL_INT(0x23 * 0x100, dma_handle.starting_addr);
  TEST_ASSERT_EQUAL_INT(0, dma_handle.current_offset);
  TEST_ASSERT_EQUAL_INT(0, dma_handle.prep_delay);

  for (int16_t i = 0; i < 0x9F; i++)
  {
    bus_interface_read_ExpectAndReturn(&bus_interface, (0x23 * 0x100) + i, NULL, STATUS_OK);
    bus_interface_read_IgnoreArg_data();
    bus_interface_read_ReturnThruPtr_data(&dma_data);

    bus_interface_write_ExpectAndReturn(&bus_interface, 0xFE00 + i, dma_data, STATUS_OK);

    TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_tick(&dma_handle));
    TEST_ASSERT_EQUAL_INT(DMA_XFER_ACTIVE, dma_handle.state);
  }

  /* Last byte to transfer (to address 0xFE9F) should reset the DMA state to IDLE */
  bus_interface_read_ExpectAndReturn(&bus_interface, (0x23 * 0x100) + 0x9F, NULL, STATUS_OK);
  bus_interface_read_IgnoreArg_data();
  bus_interface_read_ReturnThruPtr_data(&dma_data);

  bus_interface_write_ExpectAndReturn(&bus_interface, 0xFE9F, dma_data, STATUS_OK);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_tick(&dma_handle));

  TEST_ASSERT_EQUAL_INT(DMA_IDLE, dma_handle.state);
}

void test_dma_tick_bus_read_failure(void)
{

  /* Setup */
  uint8_t dma_data = 0x55;
  dma_handle_t dma_handle = {0};
  bus_interface_t bus_interface = {0};
  stub_init_bus_interface(&bus_interface, NULL);

  /* Initialize DMA */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_init(&dma_handle, bus_interface));

  /* start DMA */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_start(&dma_handle, 0x23));

  /* Flush out the 2 cycle prep */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_tick(&dma_handle));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_tick(&dma_handle));

  bus_interface_read_ExpectAndReturn(&bus_interface, (0x23 * 0x100), NULL, STATUS_ERR_GENERIC);
  bus_interface_read_IgnoreArg_data();
  bus_interface_read_ReturnThruPtr_data(&dma_data);

  TEST_ASSERT_EQUAL_INT(STATUS_ERR_GENERIC, dma_tick(&dma_handle));
}

void test_dma_tick_bus_write_failure(void)
{

  /* Setup */
  uint8_t dma_data = 0x55;
  dma_handle_t dma_handle = {0};
  bus_interface_t bus_interface = {0};
  stub_init_bus_interface(&bus_interface, NULL);

  /* Initialize DMA */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_init(&dma_handle, bus_interface));

  /* start DMA */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_start(&dma_handle, 0x23));

  /* Flush out the 2 cycle prep */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_tick(&dma_handle));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, dma_tick(&dma_handle));

  bus_interface_read_ExpectAndReturn(&bus_interface, (0x23 * 0x100), NULL, STATUS_OK);
  bus_interface_read_IgnoreArg_data();
  bus_interface_read_ReturnThruPtr_data(&dma_data);

  bus_interface_write_ExpectAndReturn(&bus_interface, 0xFE00, dma_data, STATUS_ERR_GENERIC);

  TEST_ASSERT_EQUAL_INT(STATUS_ERR_GENERIC, dma_tick(&dma_handle));
}