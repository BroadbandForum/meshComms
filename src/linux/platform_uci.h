/*
 *  prplMesh Wi-Fi Multi-AP
 *
 *  Copyright (c) 2018, prpl Foundation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/** @file
 * @brief Driver interface for UCI
 *
 * This file provides driver functionality using UCI. It uses UCI calls to create access points.
 */

#ifndef PLATFORM_UCI_H
#define PLATFORM_UCI_H

/** @brief Register the UCI callbacks for all radios.
 *
 * This function must be called after the radios have already been discovered (e.g. with nl80211).
 *
 * @todo Add a non-UCI dummy implementation.
 */
void uci_register_handlers(void);

#endif // PLATFORM_UCI_H
