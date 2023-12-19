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

/*A standard test length in seconds is 7200(Two hours) 
but is set shorter for testing purposes*/
#define TEST_DURATION 600
#define TEST_DURATION_TESTING 60

struct nodes_list {
	/** Name of the device instance */
	uint8_t list[512];
	/** Address of device instance config information */
	const void *config;
	/** Address of the API structure exposed by the device instance */
};

const struct bt_mesh_comp *model_handler_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MODEL_HANDLER_H__ */
