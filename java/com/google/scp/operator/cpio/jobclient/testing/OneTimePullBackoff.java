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

package com.google.scp.operator.cpio.jobclient.testing;

import com.google.scp.operator.cpio.jobclient.JobPullBackoff;

/** Pull backoff strategy that lets only one pull, and no subsequent pulls. */
public final class OneTimePullBackoff implements JobPullBackoff {

  /** Only allows one pull, there is no blocking wait before it signals to not perform a retry. */
  @Override
  public Boolean get() {
    return false;
  }
}
