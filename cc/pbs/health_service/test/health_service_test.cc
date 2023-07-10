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

#include "pbs/health_service/src/health_service.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <list>
#include <memory>
#include <string>

#include <gmock/gmock.h>

#include "cc/pbs/health_service/src/error_codes.h"
#include "core/interface/config_provider_interface.h"
#include "core/interface/http_server_interface.h"
#include "pbs/interface/configuration_keys.h"

using google::scp::core::AsyncContext;
using google::scp::core::ConfigKey;
using google::scp::core::ConfigProviderInterface;
using google::scp::core::ExecutionResult;
using google::scp::core::ExecutionResultOr;
using google::scp::core::FailureExecutionResult;
using google::scp::core::HttpRequest;
using google::scp::core::HttpResponse;
using google::scp::core::HttpServerInterface;
using google::scp::core::SuccessExecutionResult;
using google::scp::core::errors::
    SC_PBS_HEALTH_SERVICE_COULD_NOT_FIND_MEMORY_INFO;
using google::scp::core::errors::
    SC_PBS_HEALTH_SERVICE_COULD_NOT_OPEN_MEMINFO_FILE;
using google::scp::core::errors::
    SC_PBS_HEALTH_SERVICE_COULD_NOT_PARSE_MEMINFO_LINE;
using google::scp::core::errors::
    SC_PBS_HEALTH_SERVICE_COULD_NOT_READ_FILESYSTEM_INFO;
using google::scp::core::errors::
    SC_PBS_HEALTH_SERVICE_HEALTHY_MEMORY_USAGE_THRESHOLD_EXCEEDED;
using google::scp::core::errors::
    SC_PBS_HEALTH_SERVICE_HEALTHY_STORAGE_USAGE_THRESHOLD_EXCEEDED;
using google::scp::core::errors::
    SC_PBS_HEALTH_SERVICE_INVALID_READ_FILESYSTEM_INFO;
using google::scp::pbs::kPBSHealthServiceEnableMemoryAndStorageCheck;
using std::list;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::filesystem::space_info;
using testing::_;
using testing::ByRef;
using testing::DoAll;
using testing::NiceMock;
using testing::Return;
using testing::SetArgReferee;

namespace google::scp::pbs::test {

class ConfigProviderMock : public ConfigProviderInterface {
 public:
  ConfigProviderMock() {}

  ~ConfigProviderMock() {}

  MOCK_METHOD(ExecutionResult, Get, ((const ConfigKey& key), (bool& out)),
              (noexcept, override));

  ExecutionResult Init() noexcept override { return SuccessExecutionResult(); }

  ExecutionResult Run() noexcept override { return SuccessExecutionResult(); }

  ExecutionResult Stop() noexcept override { return SuccessExecutionResult(); }

  ExecutionResult Get(const ConfigKey& key, size_t& out) noexcept override {
    return SuccessExecutionResult();
  }

  ExecutionResult Get(const ConfigKey& key, int32_t& out) noexcept override {
    return SuccessExecutionResult();
  }

  ExecutionResult Get(const ConfigKey& key, string& out) noexcept override {
    return SuccessExecutionResult();
  }

  ExecutionResult Get(const ConfigKey& key,
                      list<string>& out) noexcept override {
    return SuccessExecutionResult();
  }

  ExecutionResult Get(const ConfigKey& key,
                      list<int32_t>& out) noexcept override {
    return SuccessExecutionResult();
  }

  ExecutionResult Get(const ConfigKey& key,
                      list<size_t>& out) noexcept override {
    return SuccessExecutionResult();
  }

  ExecutionResult Get(const ConfigKey& key, list<bool>& out) noexcept override {
    return SuccessExecutionResult();
  }
};

class HealthServiceForTests : public HealthService {
 public:
  bool mem_and_storage_health_was_checked = false;

  HealthServiceForTests() : HealthService() {}

  HealthServiceForTests(std::shared_ptr<core::HttpServerInterface>& http_server,
                        shared_ptr<ConfigProviderInterface>& config_provider)
      : HealthService(http_server, config_provider) {}

  void SetMemInfoFilePath(const string& meminfo_file_path) {
    meminfo_file_path_ = meminfo_file_path;
  }

  string GetMemInfoFilePath() noexcept override { return meminfo_file_path_; }

  void SetFileSystemSpaceInfo(
      const ExecutionResultOr<space_info>& fs_space_info) {
    fs_space_info_ = fs_space_info;
  }

  ExecutionResultOr<space_info> GetFileSystemSpaceInfo(
      string directory) noexcept override {
    return fs_space_info_;
  }

  ExecutionResult CheckHealth(
      AsyncContext<HttpRequest, HttpResponse>& http_context) {
    return HealthService::CheckHealth(http_context);
  }

  ExecutionResult CheckMemoryAndStorageUsage() noexcept override {
    mem_and_storage_health_was_checked = true;
    return HealthService::CheckMemoryAndStorageUsage();
  }

  ExecutionResultOr<int> GetMemoryUsagePercentage() {
    return HealthService::GetMemoryUsagePercentage();
  }

  ExecutionResultOr<int> GetFileSystemStorageUsagePercentage(
      const string& directory) {
    return HealthService::GetFileSystemStorageUsagePercentage(directory);
  }

 private:
  string meminfo_file_path_;
  ExecutionResultOr<space_info> fs_space_info_;
};

class HealthServiceTest : public ::testing::Test {
 public:
  HealthServiceForTests health_service;
  shared_ptr<ConfigProviderInterface> config_provider_mock;

  void SetUp() override {
    shared_ptr<HttpServerInterface> http_server;
    config_provider_mock = make_shared<NiceMock<ConfigProviderMock>>();
    // Make memory and storage checking enabled by default
    ON_CALL(*dynamic_cast<ConfigProviderMock*>(config_provider_mock.get()),
            Get(kPBSHealthServiceEnableMemoryAndStorageCheck, _))
        .WillByDefault(
            DoAll(SetArgReferee<1>(true), Return(SuccessExecutionResult())));
    health_service = HealthServiceForTests(http_server, config_provider_mock);

    // Always be good on memory and drive usage
    health_service.SetMemInfoFilePath(
        "cc/pbs/health_service/test/files/five_percent_meminfo_file.txt");
    space_info fs_space_info;
    fs_space_info.capacity = 100;
    fs_space_info.available = 80;
    health_service.SetFileSystemSpaceInfo(fs_space_info);
  }
};

TEST_F(HealthServiceTest,
       ShouldReturnHealthyWhenMemoryAndStorageUsageAreBelowThreshold) {
  AsyncContext<HttpRequest, HttpResponse> context;
  auto result = health_service.CheckHealth(context);
  EXPECT_TRUE(health_service.mem_and_storage_health_was_checked);
  EXPECT_TRUE(result.Successful());
  EXPECT_TRUE(context.result.Successful());
}

TEST_F(HealthServiceTest, ShouldNotCheckMemOrStorageIfCheckingDisabled) {
  // Return false for mem and storage checking
  EXPECT_CALL(*dynamic_cast<ConfigProviderMock*>(config_provider_mock.get()),
              Get(kPBSHealthServiceEnableMemoryAndStorageCheck, _))
      .WillOnce(
          DoAll(SetArgReferee<1>(false), Return(SuccessExecutionResult())));

  AsyncContext<HttpRequest, HttpResponse> context;
  auto result = health_service.CheckHealth(context);

  EXPECT_FALSE(health_service.mem_and_storage_health_was_checked);
  EXPECT_TRUE(result.Successful());
  EXPECT_TRUE(context.result.Successful());
}

TEST_F(HealthServiceTest, ShouldNotCheckMemOrStorageIfConfigDoesNotExist) {
  // Failure execution result when reading the config key
  EXPECT_CALL(*dynamic_cast<ConfigProviderMock*>(config_provider_mock.get()),
              Get(kPBSHealthServiceEnableMemoryAndStorageCheck, _))
      .WillOnce(Return(FailureExecutionResult(SC_UNKNOWN)));

  AsyncContext<HttpRequest, HttpResponse> context;
  auto result = health_service.CheckHealth(context);

  EXPECT_FALSE(health_service.mem_and_storage_health_was_checked);
  EXPECT_TRUE(result.Successful());
  EXPECT_TRUE(context.result.Successful());
}

TEST_F(HealthServiceTest, ShouldParseMemInfoFileWhenInfoIsAvailable) {
  // Set the path to the file to read meminfo from
  health_service.SetMemInfoFilePath(
      "cc/pbs/health_service/test/files/five_percent_meminfo_file.txt");

  auto mem_usage_percentage = health_service.GetMemoryUsagePercentage();
  EXPECT_TRUE(mem_usage_percentage.Successful());
  EXPECT_EQ(*mem_usage_percentage, 5);

  // Set the path to the file to read meminfo from
  health_service.SetMemInfoFilePath(
      "cc/pbs/health_service/test/files/ninety_six_percent_meminfo_file.txt");

  mem_usage_percentage = health_service.GetMemoryUsagePercentage();
  EXPECT_TRUE(mem_usage_percentage.Successful());
  EXPECT_EQ(*mem_usage_percentage, 96);
}

TEST_F(HealthServiceTest, ShouldIfMemInfoFileIsNotFound) {
  // Set the path to the file to read meminfo from
  health_service.SetMemInfoFilePath("file/that/does/not/exist.txt");

  auto mem_usage_percentage = health_service.GetMemoryUsagePercentage();

  EXPECT_EQ(mem_usage_percentage.result(),
            FailureExecutionResult(
                SC_PBS_HEALTH_SERVICE_COULD_NOT_OPEN_MEMINFO_FILE));
}

TEST_F(HealthServiceTest,
       ShouldFailIfAnExpectedFieldIsMissingFromTheMemInfoFile) {
  // Set the path to the file to read meminfo from
  health_service.SetMemInfoFilePath(
      "cc/pbs/health_service/test/files/missing_total_meminfo_file.txt");

  auto mem_usage_percentage = health_service.GetMemoryUsagePercentage();
  EXPECT_EQ(
      mem_usage_percentage.result(),
      FailureExecutionResult(SC_PBS_HEALTH_SERVICE_COULD_NOT_FIND_MEMORY_INFO));

  // Set the path to the file to read meminfo from
  health_service.SetMemInfoFilePath(
      "cc/pbs/health_service/test/files/missing_available_meminfo_file.txt");

  mem_usage_percentage = health_service.GetMemoryUsagePercentage();
  EXPECT_EQ(
      mem_usage_percentage.result(),
      FailureExecutionResult(SC_PBS_HEALTH_SERVICE_COULD_NOT_FIND_MEMORY_INFO));
}

TEST_F(HealthServiceTest, ShouldFailIfMemInfoFileLineIsNotInTheExpectedFormat) {
  // Set the path to the file to read meminfo from
  health_service.SetMemInfoFilePath(
      "cc/pbs/health_service/test/files/invalid_format_meminfo_file.txt");

  auto mem_usage_percentage = health_service.GetMemoryUsagePercentage();
  EXPECT_EQ(mem_usage_percentage.result(),
            FailureExecutionResult(
                SC_PBS_HEALTH_SERVICE_COULD_NOT_PARSE_MEMINFO_LINE));
}

TEST_F(HealthServiceTest, ShouldFailHealthCheckIfReadingFromMemInfoFileFails) {
  // Set the path to the file to read meminfo from
  health_service.SetMemInfoFilePath("file/that/does/not/exist.txt");

  AsyncContext<HttpRequest, HttpResponse> context;
  auto result = health_service.CheckHealth(context);
  EXPECT_TRUE(result.Successful());
  // Request response fails
  EXPECT_FALSE(context.result.Successful());
}

TEST_F(HealthServiceTest,
       ShouldFailHealthCheckIfHealthyMemThresholdIsExceeded) {
  // Set the path to the file to read meminfo from
  health_service.SetMemInfoFilePath(
      "cc/pbs/health_service/test/files/ninety_six_percent_meminfo_file.txt");

  AsyncContext<HttpRequest, HttpResponse> context;
  auto result = health_service.CheckHealth(context);
  EXPECT_TRUE(result.Successful());
  // Request response fails
  EXPECT_EQ(context.result,
            FailureExecutionResult(
                SC_PBS_HEALTH_SERVICE_HEALTHY_MEMORY_USAGE_THRESHOLD_EXCEEDED));
}

TEST_F(HealthServiceTest, ShouldFailFsStoragePercentageIfReadingInfoFails) {
  health_service.SetFileSystemSpaceInfo(FailureExecutionResult(
      SC_PBS_HEALTH_SERVICE_COULD_NOT_READ_FILESYSTEM_INFO));

  auto info = health_service.GetFileSystemStorageUsagePercentage("dir");
  EXPECT_EQ(info.result(),
            FailureExecutionResult(
                SC_PBS_HEALTH_SERVICE_COULD_NOT_READ_FILESYSTEM_INFO));
}

TEST_F(HealthServiceTest, ShouldFailIfFsStorageInfoReadingIsInvalid) {
  space_info fs_space_info;
  fs_space_info.capacity = 0;
  fs_space_info.available = 50;
  health_service.SetFileSystemSpaceInfo(fs_space_info);

  auto info = health_service.GetFileSystemStorageUsagePercentage("dir");
  EXPECT_EQ(info.result(),
            FailureExecutionResult(
                SC_PBS_HEALTH_SERVICE_INVALID_READ_FILESYSTEM_INFO));

  fs_space_info.capacity = 50;
  fs_space_info.available = 0;
  health_service.SetFileSystemSpaceInfo(fs_space_info);

  info = health_service.GetFileSystemStorageUsagePercentage("dir");
  EXPECT_EQ(info.result(),
            FailureExecutionResult(
                SC_PBS_HEALTH_SERVICE_INVALID_READ_FILESYSTEM_INFO));
}

TEST_F(HealthServiceTest, ShouldGetFsStoragePercentage) {
  space_info fs_space_info;
  fs_space_info.capacity = 100;
  fs_space_info.available = 50;
  health_service.SetFileSystemSpaceInfo(fs_space_info);

  auto percent = health_service.GetFileSystemStorageUsagePercentage("dir");
  EXPECT_TRUE(percent.result().Successful());
  EXPECT_EQ(50, *percent);

  fs_space_info.capacity = 100;
  fs_space_info.available = 95;
  health_service.SetFileSystemSpaceInfo(fs_space_info);

  percent = health_service.GetFileSystemStorageUsagePercentage("dir");
  EXPECT_TRUE(percent.result().Successful());
  EXPECT_EQ(5, *percent);

  fs_space_info.capacity = 100;
  fs_space_info.available = 5;
  health_service.SetFileSystemSpaceInfo(fs_space_info);

  percent = health_service.GetFileSystemStorageUsagePercentage("dir");
  EXPECT_TRUE(percent.result().Successful());
  EXPECT_EQ(95, *percent);
}

TEST_F(HealthServiceTest,
       ShouldFailHealthCheckIfHealthyStorageThresholdIsExceeded) {
  space_info fs_space_info;
  // Results in 96% utilization
  fs_space_info.capacity = 100;
  fs_space_info.available = 4;
  health_service.SetFileSystemSpaceInfo(fs_space_info);

  AsyncContext<HttpRequest, HttpResponse> context;
  auto result = health_service.CheckHealth(context);
  EXPECT_TRUE(result.Successful());
  // Request response fails
  EXPECT_EQ(
      context.result,
      FailureExecutionResult(
          SC_PBS_HEALTH_SERVICE_HEALTHY_STORAGE_USAGE_THRESHOLD_EXCEEDED));
}

TEST_F(HealthServiceTest, ShouldFailHealthCheckIfFilesystemInfoCantBeRead) {
  health_service.SetFileSystemSpaceInfo(FailureExecutionResult(SC_UNKNOWN));

  AsyncContext<HttpRequest, HttpResponse> context;
  auto result = health_service.CheckHealth(context);
  EXPECT_TRUE(result.Successful());
  // Request response fails
  EXPECT_EQ(context.result, FailureExecutionResult(SC_UNKNOWN));
}
}  // namespace google::scp::pbs::test