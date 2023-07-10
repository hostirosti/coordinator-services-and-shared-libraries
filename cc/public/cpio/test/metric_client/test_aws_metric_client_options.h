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

#ifndef SCP_CPIO_TEST_AWS_METRIC_CLIENT_OPTIONS_H_
#define SCP_CPIO_TEST_AWS_METRIC_CLIENT_OPTIONS_H_

#include <memory>
#include <string>

#include "public/cpio/interface/metric_client/type_def.h"

namespace google::scp::cpio {
/// MetricClientOptions for testing on AWS.
struct TestAwsMetricClientOptions : public MetricClientOptions {
  std::shared_ptr<std::string> cloud_watch_endpoint_override;
};
}  // namespace google::scp::cpio

#endif  // SCP_CPIO_TEST_AWS_METRIC_CLIENT_OPTIONS_H_
