/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/drivers/uart.h>
#include "light_monitor_srv.h"
#include "model_handler.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

#define STANDARD_THRESHOLD_VALUE 3500
#define STANDARD_THRESHOLD_VARIANCE 50
#define DIGITAL_PIN 29

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = { DT_FOREACH_PROP_ELEM(
	DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA) };
bool test_running = false;

/******************************************************************************/
/*************************** Health server setup ******************************/
/******************************************************************************/
/* Set up a repeating delayed work to blink the DK's LEDs when attention is
 * requested.
 */
static struct k_work_delayable attention_blink_work;
static bool attention;

static void attention_blink(struct k_work *work)
{
	static int idx;
	const uint8_t pattern[] = {
		BIT(0) | BIT(1),
		BIT(1) | BIT(2),
		BIT(2) | BIT(3),
		BIT(3) | BIT(0),
	};

	if (attention) {
		dk_set_leds(pattern[idx++ % ARRAY_SIZE(pattern)]);
		k_work_reschedule(&attention_blink_work, K_MSEC(30));
	} else {
		dk_set_leds(DK_NO_LEDS_MSK);
	}
}

static void attention_on(struct bt_mesh_model *mod)
{
	attention = true;
	k_work_reschedule(&attention_blink_work, K_NO_WAIT);
}

static void attention_off(struct bt_mesh_model *mod)
{
	/* Will stop rescheduling blink timer */
	attention = false;
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.attn_on = attention_on,
	.attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);
/******************************************************************************/
/**************************** peripheral setup ********************************/
/******************************************************************************/
/*The ADC, gpio out, timers, and uart are configured and logic for the test is handled here*/

uint16_t adc_buf;
uint16_t test_duration;
uint16_t ldr_value;
uint32_t time_stamp_res;
static struct bt_mesh_light_monitor monitor;
static const struct device *gpio_dev;
struct k_timer adc_timer;
struct k_work adc_work;
struct k_timer log_timer;
struct k_work log_work;
struct adc_sequence sequence = {
	.buffer = &adc_buf,
	/* buffer size in bytes, not number of samples */
	.buffer_size = sizeof(adc_buf),
};
static void adc_sampler_helper(struct k_work *adc_work);
static void logger_helper(struct k_work *log_work);
static uint16_t adc_read_with_return(void);
K_WORK_DEFINE(adc_work, adc_sampler_helper);
K_WORK_DEFINE(log_work, logger_helper);
uint16_t test_failure_threshold = STANDARD_THRESHOLD_VALUE;

bool final_result = true;

/*Finished the test run, resets the status of the monitor to allow for another test to be started*/
static void finalize_result(struct k_timer *adc_timer)
{
	int err;

	struct test_result this_result = { .result = final_result, .time_stamp = time_stamp_res };
	monitor.res_sto.last_result_idx = (monitor.res_sto.last_result_idx + 1) % 8;
	monitor.res_sto.results[monitor.res_sto.last_result_idx] = this_result;

	err = send_test_result(&monitor, final_result);
	time_stamp_res = 0;
	final_result = true;
	test_running = false;
	test_duration = 0;
	gpio_pin_set(gpio_dev, DIGITAL_PIN, 0);

	if (err < 0) {
		printk("err is %d", err);
	}
}

/*Function to calculate resistance in the LDR based on the value read by the adc. 
  Higher resistance means less light hitting the ldr. Assumes the ldr has a max resistance of 
  10000 and that the max value of the adr is 5000. Both are estimates and for real applications
  accurate values should be used*/
static uint16_t resistance_calculation(uint16_t adc_value)
{
	float voltage = (adc_value * 3.3) / 5000;
	float ldrResistance = (10000 * voltage) / (3.3 - voltage); /*100000 is an estimation, for none-sample applications the sensor must be tested*/
	return ldrResistance;
}

static void adc_sampler_helper(struct k_work *adc_work)
{

	ldr_value = resistance_calculation(adc_read_with_return());
	printk("Value is %d\n", ldr_value);
	if (ldr_value > test_failure_threshold) {
		final_result = false;
		k_timer_stop(&adc_timer);
	}
	if (test_duration-- < 1) {
		k_timer_stop(&adc_timer);
	}

}

static uint16_t adc_read_with_return(void)
{
	int err;

	(void)adc_sequence_init_dt(&adc_channels[0], &sequence);
	err = adc_read(adc_channels[0].dev, &sequence);
	if (err < 0) {
		printk("Could not read (%d)\n", err);
	}
	return (uint16_t)adc_buf;
}

static void adc_work_handler(struct k_timer *timer)
{
	k_work_submit(&adc_work);
}

static void test_start(const uint16_t duration)
{
	test_duration = duration;
	gpio_pin_set(gpio_dev, DIGITAL_PIN, 1);
	k_timer_init(&adc_timer, adc_work_handler, finalize_result);
	k_timer_start(&adc_timer, K_SECONDS(1), K_SECONDS(1));
}

static void adc_init(void)
{
	int err;
	gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	gpio_pin_configure(gpio_dev, DIGITAL_PIN, GPIO_OUTPUT);
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!device_is_ready(adc_channels[i].dev)) {
			printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
			return;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			printk("Could not setup channel #%d (%d)\n", i, err);
			return;
		}
	}
}

static void button_handler_cb(uint32_t pressed, uint32_t changed)
{

	if (!bt_mesh_is_provisioned()) {
		return;
	}
	if (pressed == BIT(0)) {
		printk("button 1 was pressed\n");
	}
	if (pressed == BIT(1)) {
		printk("button 2 was pressed\n");
	}
	if (pressed == BIT(2)) {
		gpio_pin_set(gpio_dev, DIGITAL_PIN, 0);
		printk("button 3 was pressed\n");
	}
	if (pressed == BIT(3)) {
		gpio_pin_set(gpio_dev, DIGITAL_PIN, 1);
		printk("button 4 was pressed\n");
	}

}

static void calibrate_sensor(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx)
{	
	uint16_t calibration_value;
	calibration_value = resistance_calculation(adc_read_with_return());
	test_failure_threshold = calibration_value + STANDARD_THRESHOLD_VARIANCE;
	printk("New calibrated value is %d", calibration_value);
	send_calibrated_ok(monitor);
}

/******************************************************************************/
/*************************** monitor model setup ******************************/
/******************************************************************************/
/*All handling of monitor model cb and calls are done here*/

static void handle_start(struct bt_mesh_light_monitor *monitor)
{
	printk("Started \n");
}

static void handle_test_start(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx,
			      uint16_t duration, uint32_t time_stamp)
{
	if (test_running) {
		printk("Wait for existing test to finish before starting a new test \n");
	} else {
		test_running = true;
		time_stamp_res = time_stamp;

		test_start(duration);
		send_test_ack(monitor);
	}
}
int logger_index;
struct bt_mesh_light_monitor *logging_monitor;
static void logger_helper(struct k_work *log_work)
{
	if (logger_index == logging_monitor->res_sto.last_result_idx) {
		k_timer_stop(&log_timer);
	} else if (logging_monitor->res_sto.results[logger_index].time_stamp == 0) {
		printk("Empty log entry \n");
		logger_index--;
		if (logger_index < 0) {
			logger_index = 7;
		}
	}

	else {
		send_logged_result(logging_monitor,
				   logging_monitor->res_sto.results[logger_index].time_stamp,
				   logging_monitor->res_sto.results[logger_index].result);
		logger_index--;
		if (logger_index < 0) {
			logger_index = 7;
		}
	}
}

static void log_work_handler(struct k_timer *timer)
{
	k_work_submit(&log_work);
}

static int handle_get_log(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx)
{
	printk("Time stamp in handler is %d \n",
	       monitor->res_sto.results[monitor->res_sto.last_result_idx].time_stamp);
	send_logged_result(monitor,
			   monitor->res_sto.results[monitor->res_sto.last_result_idx].time_stamp,
			   monitor->res_sto.results[monitor->res_sto.last_result_idx].result);
	logger_index = monitor->res_sto.last_result_idx - 1;
	k_timer_init(&log_timer, log_work_handler, NULL);
	k_timer_start(&log_timer, K_SECONDS(0.05), K_SECONDS(0.05));
	logging_monitor = monitor;
	return 0;
}

static int handle_get_ack(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx)
{
	if (test_running) {
		send_test_ack(monitor);
		return 0;
	} else {
		get_test_start(monitor);
		return -1;
	}
}

static int handle_get_sensor(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx)
{
	send_sensor_update(monitor, ldr_value);

	return 0;
}

static int handle_get_result(struct bt_mesh_light_monitor *monitor, struct bt_mesh_msg_ctx *ctx)
{
	send_test_result(monitor,
			 monitor->res_sto.results[monitor->res_sto.last_result_idx].result);
	return 0;
}

static const struct bt_light_monitor_handlers monitor_handlers = {
	.start = handle_start,
	.test = handle_test_start,
	.get = handle_get_sensor,
	.get_log = handle_get_log,
	.get_ack = handle_get_ack,
	.get_result = handle_get_result,
};

static const struct bt_light_monitor_setup_handlers setup_handlers = {
	.calibrate = calibrate_sensor,
};

static struct bt_mesh_light_monitor monitor = {
	.handlers = &monitor_handlers,
	.setup_handlers = &setup_handlers,
};


static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(1,
		     BT_MESH_MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
					BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub)),
		     BT_MESH_MODEL_LIST(BT_MESH_MODEL_LIGHT_MONITOR(&monitor))),
};

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

const struct bt_mesh_comp *model_handler_init(void)
{
	k_work_init_delayable(&attention_blink_work, attention_blink);
	adc_init();
	static struct button_handler button_handler = {
		.cb = button_handler_cb,
	};
	dk_button_handler_add(&button_handler);
	return &comp;
}
