/*
 * Copyright 2022 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.scp.operator.cpio.lifecycleclient.local;

import com.google.scp.operator.cpio.lifecycleclient.LifecycleClient;
import com.google.scp.operator.cpio.lifecycleclient.LifecycleModule;

/**
 * Guice Module for the Local implementation of {@link
 * com.google.scp.operator.cpio.lifecycleclient.LifecycleClient}
 */
public final class LocalLifecycleModule extends LifecycleModule {

  /**
   * Gets the local implementation of lifecycleClient
   *
   * @return
   */
  @Override
  public Class<? extends LifecycleClient> getLifecycleClientImpl() {
    return LocalLifecycleClient.class;
  }
}
