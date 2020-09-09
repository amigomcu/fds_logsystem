
#include <stdint.h>
#include <string.h>
#include "nrf.h"
#include "nordic_common.h"
#ifdef SOFTDEVICE_PRESENT
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#else
#include "nrf_drv_clock.h"
#endif
#include "fds.h"
#include "app_timer.h"
#include "app_error.h"
#include "nrf_cli.h"
#include "fds_example.h"
#include "nrf_delay.h"

#define NRF_LOG_MODULE_NAME app
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "elog.h"


/* A tag identifying the SoftDevice BLE configuration. */
#define APP_BLE_CONN_CFG_TAG    1


/* Array to map FDS return values to strings. */
char const *fds_err_str[] =
{
    "FDS_SUCCESS",
    "FDS_ERR_OPERATION_TIMEOUT",
    "FDS_ERR_NOT_INITIALIZED",
    "FDS_ERR_UNALIGNED_ADDR",
    "FDS_ERR_INVALID_ARG",
    "FDS_ERR_NULL_ARG",
    "FDS_ERR_NO_OPEN_RECORDS",
    "FDS_ERR_NO_SPACE_IN_FLASH",
    "FDS_ERR_NO_SPACE_IN_QUEUES",
    "FDS_ERR_RECORD_TOO_LARGE",
    "FDS_ERR_NOT_FOUND",
    "FDS_ERR_NO_PAGES",
    "FDS_ERR_USER_LIMIT_REACHED",
    "FDS_ERR_CRC_CHECK_FAILED",
    "FDS_ERR_BUSY",
    "FDS_ERR_INTERNAL",
};

/* Array to map FDS events to strings. */
static char const *fds_evt_str[] =
{
    "FDS_EVT_INIT",
    "FDS_EVT_WRITE",
    "FDS_EVT_UPDATE",
    "FDS_EVT_DEL_RECORD",
    "FDS_EVT_DEL_FILE",
    "FDS_EVT_GC",
};

/* Dummy configuration data. */
static configuration_t m_dummy_cfg =
{
    .config1_on  = false,
    .config2_on  = true,
    .boot_count  = 0x0,
    .device_name = "dummy",
};

/* A record containing dummy configuration data. */
static fds_record_t const m_dummy_record =
{
    .file_id           = CONFIG_FILE,
    .key               = CONFIG_REC_KEY,
    .data.p_data       = &m_dummy_cfg,
    /* The length of a record is always expressed in 4-byte units (words). */
    .data.length_words = (sizeof(m_dummy_cfg) + 3) / sizeof(uint32_t),
};

/* Keep track of the progress of a delete_all operation. */
static struct
{
    bool delete_next;   //!< Delete next record.
    bool pending;       //!< Waiting for an fds FDS_EVT_DEL_RECORD event, to delete the next record.
} m_delete_all;

/* Flag to check fds initialization. */
static bool volatile m_fds_initialized;
/**
 * @brief 
 * description write flash state
 * true, write now ,set log_out
 * false, only when fds_event
 */
extern volatile bool isWriteing;

static void fds_evt_handler(fds_evt_t const *p_evt)
{
    NRF_LOG_GREEN("Event: %s received (%s)",
                  fds_evt_str[p_evt->id],
                  fds_err_str[p_evt->result]);

    switch (p_evt->id)
    {
    case FDS_EVT_INIT:
        if (p_evt->result == FDS_SUCCESS)
        {
            m_fds_initialized = true;
        }
        break;

    case FDS_EVT_WRITE:
    {
        if (p_evt->result == FDS_SUCCESS)
        {
            NRF_LOG_INFO("Record ID:\t0x%04x",  p_evt->write.record_id);
            NRF_LOG_INFO("File ID:\t0x%04x",    p_evt->write.file_id);
            NRF_LOG_INFO("Record key:\t0x%04x", p_evt->write.record_key);
        }
            isWriteing  = false;
    } break;

    case FDS_EVT_DEL_RECORD:
    {
        if (p_evt->result == FDS_SUCCESS)
        {
            NRF_LOG_INFO("Record ID:\t0x%04x",  p_evt->del.record_id);
            NRF_LOG_INFO("File ID:\t0x%04x",    p_evt->del.file_id);
            NRF_LOG_INFO("Record key:\t0x%04x", p_evt->del.record_key);
        }
        m_delete_all.pending = false;
    } break;

    default:
        break;
    }
}


/**@brief   Begin deleting all records, one by one. */
void delete_all_begin(void)
{
    m_delete_all.delete_next = true;
}


/**@brief   Process a delete all command.
 *
 * Delete records, one by one, until no records are left.
 */
void delete_all_process(void)
{
    if (   m_delete_all.delete_next
            & !m_delete_all.pending)
    {
        NRF_LOG_INFO("Deleting next record.");

        m_delete_all.delete_next = record_delete_next();
        if (!m_delete_all.delete_next)
        {
            NRF_LOG_CYAN("No records left to delete.");
        }
    }
}


#ifdef SOFTDEVICE_PRESENT
/**@brief   Function for initializing the SoftDevice and enabling the BLE stack. */
static void ble_stack_init(void)
{
    ret_code_t rc;
    uint32_t   ram_start;

    /* Enable the SoftDevice. */
    rc = nrf_sdh_enable_request();
    APP_ERROR_CHECK(rc);

    rc = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(rc);

    rc = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(rc);
}
#else
static void clock_init(void)
{
    /* Initialize the clock. */
    ret_code_t rc = nrf_drv_clock_init();
    APP_ERROR_CHECK(rc);

    nrf_drv_clock_lfclk_request(NULL);

    /* Wait for the clock to be ready. */
    while (!nrf_clock_lf_is_running()) {;}
}
#endif


/**@brief   Initialize the timer. */
static void timer_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);


}


/**@brief   Initialize logging. */
static void log_init(void)
{
    ret_code_t rc = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(rc);
}


/**@brief   Sleep until an event is received. */
static void power_manage(void)
{
#ifdef SOFTDEVICE_PRESENT
    (void) sd_app_evt_wait();
#else
    __WFE();
#endif
}


/**@brief   Wait for fds to initialize. */
static void wait_for_fds_ready(void)
{
    while (!m_fds_initialized)
    {
        power_manage();
    }
}


void assert_hook(const char *expr, const char *func, size_t line)
{
    NRF_LOG_INFO(expr);
    //elog_a("E/a", "Er");
    log_a("A");
    return;
}

#define SYMBOL_NAME MAIN

int main(void)
{
     ret_code_t rc;

#ifdef SOFTDEVICE_PRESENT
    ble_stack_init();
#else
    clock_init();
#endif

    timer_init();
    log_init();
    cli_init();

    NRF_LOG_INFO("FDS example started.")

    /* Register first to receive an event when initialization is complete. */
    (void) fds_register(fds_evt_handler);

    NRF_LOG_INFO("Initializing fds...");

    rc = fds_init();
    APP_ERROR_CHECK(rc);

    /* Wait for fds to initialize. */
    wait_for_fds_ready();

    NRF_LOG_INFO("Available commands:");
    NRF_LOG_INFO("- print all\t\tprint records");
    NRF_LOG_INFO("- print config\tprint configuration");
    NRF_LOG_INFO("- update\t\tupdate configuration");
    NRF_LOG_INFO("- stat\t\tshow statistics");
    NRF_LOG_INFO("- write\t\twrite a new record");
    NRF_LOG_INFO("- delete\t\tdelete a record");
    NRF_LOG_INFO("- delete_all\tdelete all records");
    NRF_LOG_INFO("- gc\t\trun garbage collection");

    NRF_LOG_INFO("Reading flash usage statistics...");

    fds_stat_t stat = {0};

    rc = fds_stat(&stat);
    APP_ERROR_CHECK(rc);
    elog_init();
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    /* start EasyLogger */
    elog_start();
    elog_assert_set_hook(assert_hook);

    NRF_LOG_INFO("Found %d valid records.", stat.valid_records);
    NRF_LOG_INFO("Found %d dirty records (ready to be garbage collected).", stat.dirty_records);
/*
    fds_record_desc_t desc = {0};
    fds_find_token_t  tok  = {0};

    rc = fds_record_find(CONFIG_FILE, CONFIG_REC_KEY, &desc, &tok);

    if (rc == FDS_SUCCESS)
    {
        fds_flash_record_t config = {0};

        rc = fds_record_open(&desc, &config);
        APP_ERROR_CHECK(rc);

        memcpy(&m_dummy_cfg, config.p_data, sizeof(configuration_t));

        NRF_LOG_INFO("Config file found, updating boot count to %d.", m_dummy_cfg.boot_count);

        m_dummy_cfg.boot_count++;

        rc = fds_record_close(&desc);
        APP_ERROR_CHECK(rc);

        rc = fds_record_update(&desc, &m_dummy_record);
        APP_ERROR_CHECK(rc);
    }
    else
    {
        NRF_LOG_INFO("Writing config file...");

        rc = fds_record_write(&desc, &m_dummy_record);
        APP_ERROR_CHECK(rc);
    }*/

    cli_start();
    ELOG_ASSERT(1==0)
    ELOG_ASSERT(2==0)
//    ELOG_INFO(SYMBOL_NAME,1 == 0);
    //ELOG_INFO(SYMBOL_NAME, 0 == 1);
//    elog_set_output_enabled(false);
    nrf_delay_ms(1000);
    //log_i("Hello!");

    /* Enter main loop. */
    for (;;)
    {
        if (!NRF_LOG_PROCESS())
        {
            power_manage();
        }
        cli_process();
        delete_all_process();
    }
}


/**
 * @}
 */
