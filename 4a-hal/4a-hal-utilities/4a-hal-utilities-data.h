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

#ifndef _HAL_UTILITIES_DATA_INCLUDE_
#define _HAL_UTILITIES_DATA_INCLUDE_

#include <stdio.h>

#include <wrap-json.h>

#include <afb-definitions.h>

#include <ctl-config.h>

#include "../4a-hal-controllers/4a-hal-controllers-alsacore-link.h"

// Enum for sharing hal (controller or external) status
enum HalStatus {
	HAL_STATUS_UNAVAILABLE=0,
	HAL_STATUS_AVAILABLE=1,
	HAL_STATUS_READY=2
};

// Structure to store stream data
struct CtlHalMixerData {
	char *verb;
	char *verbToCall;
	char *streamCardId;
};

// Structure to store stream data table
struct CtlHalMixerDataT {
	struct CtlHalMixerData *data;
	unsigned int count;
};

// Structure to store specific controller hal data
struct CtlHalSpecificData {
	char *mixerApiName;
	char *prefix;
	json_object *halMixerJ;

	struct CtlHalMixerDataT ctlHalStreamsData;
	struct CtlHalMixerDataT ctlHalPlaybacksData;
	struct CtlHalMixerDataT ctlHalCapturesData;
	struct CtlHalAlsaMapT *ctlHalAlsaMapT;

	AFB_ApiT apiHandle;
	CtlConfigT *ctrlConfig;
};

// Structure to store specific hal (controller or external) data
struct SpecificHalData {
	char *apiName;
	enum HalStatus status;
	char *sndCardPath;
	int sndCardId;
	char *info;
	unsigned int internal;

	char *author;
	char *version;
	char *date;
	// Can be beefed up if needed

	struct CtlHalSpecificData *ctlHalSpecificData;		// Can be NULL if external api

	struct SpecificHalData *next;
};

// Structure to store hal manager data
struct HalMgrData {
	char *apiName;
	char *info;

	AFB_ApiT apiHandle;

	struct SpecificHalData *first;
};

// Exported verbs for 'struct SpecificHalData' handling
struct SpecificHalData *HalUtlAddHalApiToHalList(struct SpecificHalData **firstHalData);
int8_t HalUtlRemoveSelectedHalFromList(struct SpecificHalData **firstHalData, struct SpecificHalData *ApiToRemove);
int64_t HalUtlRemoveAllHalFromList(struct SpecificHalData **firstHalData);
int64_t HalUtlGetNumberOfHalInList(struct SpecificHalData **firstHalData);
struct SpecificHalData *HalUtlSearchHalDataByApiName(struct SpecificHalData **firstHalData, char *apiName);
struct SpecificHalData *HalUtlSearchReadyHalDataByCarId(struct SpecificHalData **firstHalData, int cardId);

// Exported verbs for 'struct HalMgrData' handling
uint8_t HalUtlInitializeHalMgrData(AFB_ApiT apiHandle, struct HalMgrData *HalMgrGlobalData, char *apiName, char *info);
void HalUtlRemoveHalMgrData(struct HalMgrData *HalMgrGlobalData);

#endif /* _HAL_UTILITIES_DATA_INCLUDE_ */