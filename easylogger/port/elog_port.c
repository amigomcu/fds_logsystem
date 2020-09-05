#include "elog.h"
#include "fds.h"
//#include "fds_example.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#define LOG_FILE     (0x1010)
#define ASSET_REC_KEY  (0x1010)
#define INFO_REC_KEY  (0x2010)

/*uint8_t gu8LogString[ELOG_LINE_BUF_SIZE];
static fds_record_t const m_assert_record =
{
    .file_id           = LOG_FILE,
    .key               = ASSET_REC_KEY,
    .data.p_data       = gu8LogString,
    .data.length_words = (sizeof(gu8LogString) + 3) / sizeof(uint32_t),
};*/

/**
 * EasyLogger port initialize
 *
 * @return result
 */
ElogErrCode elog_port_init(void) {
    ret_code_t rc;
    ElogErrCode result = ELOG_NO_ERR;

    /* add your code here */
    
    fds_record_desc_t desc = {0};
    fds_find_token_t  tok  = {0};

    rc = fds_record_find(LOG_FILE, ASSET_REC_KEY, &desc, &tok);

    if (rc == FDS_SUCCESS)
    {
        /* A config file is in flash. Let's update it. */
        fds_flash_record_t config = {0};

        /* Open the record and read its contents. */
        rc = fds_record_open(&desc, &config);
        APP_ERROR_CHECK(rc);

        /* Copy the configuration from flash into m_dummy_cfg. */
        //memcpy(&m_dummy_cfg, config.p_data, sizeof(configuration_t));
        rc = fds_record_close(&desc);
        APP_ERROR_CHECK(rc);
        NRF_LOG_INFO("find log record");
    } else {
        NRF_LOG_INFO("not find log record");
    }
    return result;
}

//ASSET_REC_KEY
uint16_t key_start =ASSET_REC_KEY;
/**
 * output log port interface
 *
 * @param log output of log
 * @param size log size
 */
void elog_port_output(const char *log, size_t size) {
    /* add your code here */
    fds_record_t const rec =
    {
        .file_id           = LOG_FILE,
        .key               = key_start,
        .data.p_data       = log,
        .data.length_words = (size + 3) / sizeof(uint32_t)
    };

    ret_code_t rc = fds_record_write(NULL, &rec);
    if (rc != FDS_SUCCESS)
    {
        NRF_LOG_INFO("elog failed");
    }
key_start++;
}

/**
 * output lock
 */
void elog_port_output_lock(void) {
    
    /* add your code here */
    
}

/**
 * output unlock
 */
void elog_port_output_unlock(void) {
    
    /* add your code here */
    
}

/**
 * get current time interface
 *
 * @return current time
 */
const char *elog_port_get_time(void) {
    
    /* add your code here */
    return "10:08:12";

}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char *elog_port_get_p_info(void) {
    
    /* add your code here */
    
    return "pid:1008";
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char *elog_port_get_t_info(void) {
    
    /* add your code here */
    return "tid:24";
}