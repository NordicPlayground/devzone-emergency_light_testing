/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Model handler
 */

#ifndef MODEL_HANDLER_H__
#define MODEL_HANDLER_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

/*A standard test length in seconds 7200*/
#define TEST_DURATION 7200
#define TEST_DURATION_TESTING 60
#define TEST_TIME_STAMP 1111111111

struct NodesList {
	uint16_t nodes[512];
	uint16_t len;
};

struct UartInput {
	uint8_t code;
	uint32_t value;
	uint16_t duration;
};

const struct bt_mesh_comp *model_handler_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MODEL_HANDLER_H__ */
