/*
 * Copyright (C) 2018 "IoT.bzh"
 * Author Jonathan Aillet <jonathan.aillet@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>

#include <filescan-utils.h>
#include <wrap-json.h>

#include <afb-definitions.h>

#include <ctl-config.h>

#include "../4a-hal-utilities/4a-hal-utilities-verbs-loader.h"

#include "4a-hal-controllers-api-loader.h"
#include "4a-hal-controllers-alsacore-link.h"
#include "4a-hal-controllers-cb.h"
#include "4a-hal-controllers-mixer-link.h"

// Default api to print log when apihandle not available
AFB_ApiT AFB_default;

/*******************************************************************************
 *		Json parsing functions using app controller		       *
 ******************************************************************************/

// Config Section definition
static CtlSectionT ctrlSections[] =
{
	{ .key = "resources",	.loadCB = PluginConfig },
	{ .key = "halmixer",	.loadCB = HalCtlsHalMixerConfig },
	{ .key = "halmap",	.loadCB = HalCtlsHalMapConfig },
	{ .key = "onload",	.loadCB = OnloadConfig },
	{ .key = "controls",	.loadCB = ControlConfig },
	{ .key = "events",	.loadCB = EventConfig },
	{ .key = NULL }
};

/*******************************************************************************
 *		Dynamic HAL verbs' functions				       *
 ******************************************************************************/

// Every HAL export the same API & Interface Mapping from SndCard to AudioLogic is done through alsaHalSndCardT
static AFB_ApiVerbs CtlHalDynApiStaticVerbs[] =
{
	/* VERB'S NAME			FUNCTION TO CALL		SHORT DESCRIPTION */
	{ .verb = "info",		.callback = HalCtlsInfo,	.info = "List available streams/playbacks/captures... for this api" },
	{ .verb = NULL }		// Marker for end of the array
};

/*******************************************************************************
 *		Dynamic API functions for app controller		       *
 *		TBD JAI : Use API-V3 instead of API-PRE-V3		       *
 ******************************************************************************/

static int HalCtlsInitOneApi(AFB_ApiT apiHandle)
{
	CtlConfigT *ctrlConfig;
	struct SpecificHalData *currentCtlHalData;

	if(! apiHandle)
		return -1;

	// Hugely hack to make all V2 AFB_DEBUG to work in fileutils
	AFB_default = apiHandle;

	// Retrieve section config from api handle
	ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(apiHandle);
	if(! ctrlConfig)
		return -2;

	currentCtlHalData = (struct SpecificHalData *) ctrlConfig->external;
	if(! currentCtlHalData)
		return -3;

	// Fill SpecificHalDatadata structure
	currentCtlHalData->internal = 1;

	currentCtlHalData->sndCardPath = (char *) ctrlConfig->uid;
	currentCtlHalData->info = (char *) ctrlConfig->info;

	currentCtlHalData->author = (char *) ctrlConfig->author;
	currentCtlHalData->version = (char *) ctrlConfig->version;
	currentCtlHalData->date = (char *) ctrlConfig->date;

	currentCtlHalData->ctlHalSpecificData->apiHandle = apiHandle;
	currentCtlHalData->ctlHalSpecificData->ctrlConfig = ctrlConfig;

	currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData.count = 0;

	currentCtlHalData->sndCardId = HalCtlsGetCardIdByCardPath(apiHandle, currentCtlHalData->sndCardPath);

	if(currentCtlHalData->sndCardId < 0)
		currentCtlHalData->status = HAL_STATUS_UNAVAILABLE;
	else
		currentCtlHalData->status = HAL_STATUS_AVAILABLE;

	// TBD JAI: handle refresh of hal status for dynamic card (/dev/by-id)

	return CtlConfigExec(apiHandle, ctrlConfig);
}

static int HalCtlsLoadOneApi(void *cbdata, AFB_ApiT apiHandle)
{
	int err;
	CtlConfigT *ctrlConfig;

	if(! cbdata || ! apiHandle)
		return -1;

	ctrlConfig = (CtlConfigT*) cbdata;

	// Save closure as api's data context
	afb_dynapi_set_userdata(apiHandle, ctrlConfig);

	// Add static controls verbs
	if(HalUtlLoadVerbs(apiHandle, CtlHalDynApiStaticVerbs)) {
		AFB_ApiError(apiHandle, "Load Section : fail to register static V2 verbs");
		return 1;
	}

	// Load section for corresponding Api
	if((err = CtlLoadSections(apiHandle, ctrlConfig, ctrlSections)))
		return err;

	// Declare an event manager for this Api
	afb_dynapi_on_event(apiHandle, HalCtlsDispatchApiEvent);

	// Init Api function (does not receive user closure ???)
	afb_dynapi_on_init(apiHandle, HalCtlsInitOneApi);

	return 0;
}

int HalCtlsCreateApi(AFB_ApiT apiHandle, char *path, struct HalMgrData *HalMgrGlobalData)
{
	CtlConfigT *ctrlConfig;
	struct SpecificHalData *currentCtlHalData;

	if(! apiHandle || ! path || ! HalMgrGlobalData)
		return -1;

	// Create one Api per file
	ctrlConfig = CtlLoadMetaData(apiHandle, path);
	if(! ctrlConfig) {
		AFB_ApiError(apiHandle, "No valid control config file in:\n-- %s", path);
		return -2;
	}

	if(! ctrlConfig->api) {
		AFB_ApiError(apiHandle, "API Missing from metadata in:\n-- %s", path);
		return -3;
	}

	// Allocation of current hal controller data
	currentCtlHalData = HalUtlAddHalApiToHalList(&HalMgrGlobalData->first);
	if(! currentCtlHalData)
		return -4;

	currentCtlHalData->apiName = (char *) ctrlConfig->api;

	// Stores current hal controller data in controller config
	ctrlConfig->external = (void *) currentCtlHalData;

	// Allocation of the structure that will be used to store specific hal controller data
	currentCtlHalData->ctlHalSpecificData = calloc(1, sizeof(struct CtlHalSpecificData));

	// Create one API (Pre-V3 return code ToBeChanged)
	if(afb_dynapi_new_api(apiHandle, ctrlConfig->api, ctrlConfig->info, 1, HalCtlsLoadOneApi, ctrlConfig))
		return -5;

	return 0;
}

int HalCtlsCreateAllApi(AFB_ApiT apiHandle, struct HalMgrData *HalMgrGlobalData)
{
	int index, status = 0;
	char *dirList, *fileName, *fullPath;
	char filePath[CONTROL_MAXPATH_LEN];

	filePath[CONTROL_MAXPATH_LEN - 1] = '\0';

	json_object *configJ, *entryJ;

	if(! apiHandle || ! HalMgrGlobalData)
		return -1;

	// Hugely hack to make all V2 AFB_DEBUG to work in fileutils
	AFB_default = apiHandle;

	AFB_ApiNotice(apiHandle, "Begining to create all APIs");

	dirList = getenv("CONTROL_CONFIG_PATH");
	if(! dirList)
		dirList = CONTROL_CONFIG_PATH;

	configJ = CtlConfigScan(dirList, "hal");
	if(! configJ) {
		AFB_ApiWarning(apiHandle, "No hal-(binder-middle-name)*.json config file(s) found in %s, 4a-hal-manager will only works with external hal", dirList);
		return 0;
	}

	// We load 1st file others are just warnings
	for(index = 0; index < (int) json_object_array_length(configJ); index++) {
		entryJ = json_object_array_get_idx(configJ, index);

		if(wrap_json_unpack(entryJ, "{s:s, s:s !}", "fullpath", &fullPath, "filename", &fileName)) {
			AFB_ApiError(apiHandle, "HOOPs invalid JSON entry = %s", json_object_get_string(entryJ));
			return -2;
		}

		strncpy(filePath, fullPath, sizeof(filePath) - 1);
		strncat(filePath, "/", sizeof(filePath) - 1);
		strncat(filePath, fileName, sizeof(filePath) - 1);

		if(HalCtlsCreateApi(apiHandle, filePath, HalMgrGlobalData) < 0)
			status--;
	}

	return status;
}