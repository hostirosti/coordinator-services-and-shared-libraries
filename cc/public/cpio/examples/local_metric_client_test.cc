// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include <aws/core/Aws.h>

#include "core/test/utils/conditional_wait.h"
#include "public/core/interface/errors.h"
#include "public/core/interface/execution_result.h"
#include "public/cpio/interface/metric_client/metric_client_interface.h"
#include "public/cpio/interface/metric_client/type_def.h"
#include "public/cpio/interface/type_def.h"
#include "public/cpio/test/global_cpio/test_lib_cpio.h"

using Aws::InitAPI;
using Aws::SDKOptions;
using Aws::ShutdownAPI;
using google::scp::core::AsyncContext;
using google::scp::core::ExecutionResult;
using google::scp::core::GetErrorMessage;
using google::scp::core::SuccessExecutionResult;
using google::scp::core::test::WaitUntil;
using google::scp::cpio::LogOption;
using google::scp::cpio::Metric;
using google::scp::cpio::MetricClientFactory;
using google::scp::cpio::MetricClientInterface;
using google::scp::cpio::MetricClientOptions;
using google::scp::cpio::MetricUnit;
using google::scp::cpio::PutMetricsRequest;
using google::scp::cpio::PutMetricsResponse;
using google::scp::cpio::TestCpioOptions;
using google::scp::cpio::TestLibCpio;
using std::atomic;
using std::make_shared;
using std::make_unique;
using std::move;
using std::shared_ptr;
using std::stod;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::chrono::milliseconds;

static constexpr char kRegion[] = "us-east-1";

int main(int argc, char* argv[]) {
  SDKOptions options;
  InitAPI(options);
  TestCpioOptions cpio_options;
  cpio_options.log_option = LogOption::kConsoleLog;
  cpio_options.region = kRegion;
  auto result = TestLibCpio::InitCpio(cpio_options);
  if (!result.Successful()) {
    std::cout << "Failed to initialize CPIO: "
              << GetErrorMessage(result.status_code) << std::endl;
  }

  MetricClientOptions metric_client_options;
  metric_client_options.metric_namespace = "MetricClientTest";
  metric_client_options.enable_batch_recording = true;
  metric_client_options.batch_recording_time_duration = milliseconds(10);
  auto metric_client = MetricClientFactory::Create(move(metric_client_options));
  result = metric_client->Init();
  if (!result.Successful()) {
    std::cout << "Cannot init metric client!"
              << GetErrorMessage(result.status_code) << std::endl;
    return 0;
  }
  result = metric_client->Run();
  if (!result.Successful()) {
    std::cout << "Cannot run metric client!"
              << GetErrorMessage(result.status_code) << std::endl;
    return 0;
  }

  Metric metric;
  metric.name = "test_metric";
  metric.value = "12";
  metric.unit = MetricUnit::kCount;
  metric.labels = {{"lable_key", "label_value"}};
  PutMetricsRequest request;
  request.metrics.push_back(metric);

  atomic<bool> finished = false;
  result = metric_client->PutMetrics(
      move(request),
      [&](const ExecutionResult result, PutMetricsResponse response) {
        if (!result.Successful()) {
          std::cout << "PutMetrics failed: "
                    << GetErrorMessage(result.status_code) << std::endl;
        } else {
          std::cout << "PutMetrics succeeded." << std::endl;
        }
        finished = true;
      });
  if (!result.Successful()) {
    std::cout << "PutMetrics failed immediately: "
              << GetErrorMessage(result.status_code) << std::endl;
  }
  WaitUntil([&finished]() { return finished.load(); }, milliseconds(100000));

  result = metric_client->Stop();
  if (!result.Successful()) {
    std::cout << "Cannot stop metric client!"
              << GetErrorMessage(result.status_code) << std::endl;
  }

  result = TestLibCpio::ShutdownCpio(cpio_options);
  if (!result.Successful()) {
    std::cout << "Failed to shutdown CPIO: "
              << GetErrorMessage(result.status_code) << std::endl;
  }
  ShutdownAPI(options);
}