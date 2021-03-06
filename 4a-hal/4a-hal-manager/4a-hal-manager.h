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

#ifndef _HALMGR_BINDING_INCLUDE_
#define _HALMGR_BINDING_INCLUDE_

#include <stdio.h>

#define HAL_MANAGER_API_NAME "4a-hal-manager"
#define HAL_MANAGER_API_INFO "Manager for 4A HAL APIs"

// HAL Manager get first 'SpecificHalData' structure from HAL list function
struct SpecificHalData **HalMngGetFirstHalData(void);

#endif /* _HALMGR_BINDING_INCLUDE_ */