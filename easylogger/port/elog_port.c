#include "elog.h"
#include "fds.h"
//#include "fds_example.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "app_timer.h"

#ifndef ELOG_ASYNC_POLL_GET_LOG_BUF_SIZE
#define ELOG_ASYNC_POLL_GET_LOG_BUF_SIZE         (ELOG_LINE_BUF_SIZE)
#endif

APP_TIMER_DEF(m_FdsWrite_tmr);
/*uint8_t gu8LogString[ELOG_LINE_BUF_SIZE];
static fds_record_t const m_assert_record =
{
    .file_id           = LOG_FILE,
    .key               = ASSET_REC_KEY,
    .data.p_data       = gu8LogString,
    .data.length_words = (sizeof(gu8LogString) + 3) / sizeof(uint32_t),
};*/


volatile bool isWriteing = false;
static bool isSynTimeRun = false;
/**
 * @brief
 * 1. check the state(write ack,semi)
 * 2.
 * @param p_context
 */

void fdsFifo_timer_handler(void *p_context)
{
    ret_code_t ret;
    size_t get_log_size = 0;
    static char poll_get_buf[ELOG_ASYNC_POLL_GET_LOG_BUF_SIZE];

    if (false == isWriteing)
    {

#ifdef ELOG_ASYNC_LINE_OUTPUT
        get_log_size = elog_async_get_line_log(poll_get_buf, ELOG_ASYNC_POLL_GET_LOG_BUF_SIZE);
#else
        get_log_size = elog_async_get_log(poll_get_buf, ELOG_ASYNC_POLL_GET_LOG_BUF_SIZE);
#endif

        if (get_log_size)
        {
            isWriteing  = true;
            elog_port_output(poll_get_buf, get_log_size);
        }
        else
        {
            isSynTimeRun = false;
            ret = app_timer_stop(m_FdsWrite_tmr);
        }
    }
    return;
}
/**
 * EasyLogger port initialize
 *
 * @return result
 */
ElogErrCode elog_port_init(void)
{
    ret_code_t rc;
    ElogErrCode result = ELOG_NO_ERR;
    /* add your code here */

    rc = app_timer_create(&m_FdsWrite_tmr, APP_TIMER_MODE_REPEATED, fdsFifo_timer_handler);
    APP_ERROR_CHECK(rc);
    return result;
}

static int16_t getKey(const char *log)
{
    switch (log[0])
    {
    case 'A': return ELOG_LVL_ASSERT;
    case 'E': return ELOG_LVL_ERROR;
    case 'W': return ELOG_LVL_WARN;
    case 'I': return ELOG_LVL_INFO;
    case 'D': return ELOG_LVL_DEBUG;
    case 'V': return ELOG_LVL_VERBOSE;
    default: return -1;
    }
}

//ASSET_REC_KEY
/**
 * output log port interface
 *
 * @param log output of log
 * @param size log size
 */
void elog_port_output(const char *log, size_t size)
{
    /* add your code here */

    int16_t key_start = getKey(log);

    if (key_start == -1)
        return;

    fds_record_t const rec =
    {
        .file_id           = ((key_start << 8) | 0x1000),
        .key               = ((key_start << 8) | 0x1000),
        .data.p_data       = log,
        .data.length_words = (size + 3) / sizeof(uint32_t)
    };

    ret_code_t rc = fds_record_write(NULL, &rec);
    if (rc != FDS_SUCCESS)
    {
        NRF_LOG_INFO("elog failed");
    }
    else
    {
        NRF_LOG_INFO("elog write %s,len %d", log, size);
    }
}

/**
 * output lock
 */
void elog_port_output_lock(void)
{

    /* add your code here */

}

/**
 * output unlock
 */
void elog_port_output_unlock(void)
{

    /* add your code here */

}

/**
 * get current time interface
 *
 * @return current time
 */
const char *elog_port_get_time(void)
{
    /* add your code here */
    return "100812";
}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char *elog_port_get_p_info(void)
{

    /* add your code here */

    return "pid:1008";
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char *elog_port_get_t_info(void)
{

    /* add your code here */
    return "tid:24";
}

/**
 * @brief
 * check the log syn timer is runing now  
 * see the isSynTimeRun
 */
void elog_async_output_notice(void)
{
    ret_code_t ret;
    if(false == isSynTimeRun)
    {
            isSynTimeRun = true;
            ret = app_timer_start(m_FdsWrite_tmr, APP_TIMER_TICKS(1000), NULL);
    }
    return;
}
