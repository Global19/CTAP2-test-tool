// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef TEST_SERIES_H_
#define TEST_SERIES_H_

#include <cstdio>

#include "cbor_builders.h"
#include "device_interface.h"
#include "parameter_check.h"
#include "absl/types/variant.h"
#include "third_party/chromium_components_cbor/values.h"

namespace fido2_tests {

// This is the base class for all test series, capable of printing a summary of
// test results. You can choose to assert, if the success of the assertion is
// critical for further execution of the test suite. If a failure is tolerable,
// CheckAndReport tracks the success rate of all tests executed in this series.
// Non-mandatory tests are included for checking conditions that do not
// necessarily mean faulty behavior.
class TestSeries {
 public:
  explicit TestSeries(std::string test_series_name);
  // Uses the name and counters to print result strings.
  void PrintResults();

 protected:
  // Asserts a general condition, reporting the result and exiting on failure.
  void AssertCondition(bool condition, const std::string& test_name);
  // As above, but asserts the success of an executed command.
  void AssertResponse(
      const absl::variant<cbor::Value, Status>& returned_variant,
      const std::string& test_name);
  // Checks a general condition, reporting the result and writing statistics.
  void CheckAndReport(bool condition, const std::string& test_name);
  // As above, but checks specifically whether the variant is a CBOR value.
  void CheckAndReport(
      const absl::variant<cbor::Value, Status>& returned_variant,
      const std::string& test_name);
  // As above, but checks specifically if the expected and returned status are
  // both an error or both not an error. If both are different errors, the test
  // counts as passed, but the report contains a warning.
  void CheckAndReport(Status expected_status, Status returned_status,
                      const std::string& test_name);

 private:
  std::string test_series_name_;
  int total_tests_ = 0;
  int successful_tests_ = 0;
};

// Systematically check all input parameters, if they follow the specification.
// That includes enforcing the correct type of parameters, including members of
// maps and arrays. It is very strict at checking unexpected additional
// parameters, whenever the specification does not explicitly allow them. In
// that case, it does not fail, but just prints a red message. The same goes for
// checking optional parameters.
// Example:
//    fido2_tests::InputParameterTestSeries input_parameter_test_series =
//        fido2_tests::InputParameterTestSeries(device, key_checker);
//    input_parameter_test_series.MakeCredentialBadParameterTypesTest();
class InputParameterTestSeries : public TestSeries {
 public:
  InputParameterTestSeries(DeviceInterface* device, KeyChecker* key_checker,
                           CounterChecker* counter_checker);
  // Check if MakeCredential accepts different CBOR types for its parameters.
  void MakeCredentialBadParameterTypesTest();
  // Check if MakeCredential accepts leaving out one of the required parameters.
  void MakeCredentialMissingParameterTest();
  // Check the optional map entries of the relying party entity.
  void MakeCredentialRelyingPartyEntityTest();
  // Check the optional map entries of the user entity.
  void MakeCredentialUserEntityTest();
  // Check the inner array transport elements of the exclude list.
  void MakeCredentialExcludeListTest();
  // Check if unknown extensions are accepted.
  void MakeCredentialExtensionsTest();
  // Check if GetAssertion accepts different CBOR types for its parameters.
  void GetAssertionBadParameterTypesTest();
  // Check if GetAssertion accepts leaving out one of the required parameters.
  void GetAssertionMissingParameterTest();
  // Check the inner array transport elements of the allow list.
  void GetAssertionAllowListTest();
  // Check if unknown extensions are accepted.
  void GetAssertionExtensionsTest();
  // Check the input parameters of the client PIN subcommand getPinRetries.
  void ClientPinGetPinRetriesTest();
  // Check the input parameters of the client PIN subcommand getKeyAgreement.
  void ClientPinGetKeyAgreementTest();
  // Check the input parameters of the client PIN subcommand setPin.
  void ClientPinSetPinTest();
  // Check the input parameters of the client PIN subcommand changePin.
  void ClientPinChangePinTest();
  // Check the input parameters of the client PIN subcommand
  // getPinUvAuthTokenUsingPin.
  void ClientPinGetPinUvAuthTokenUsingPinTest();
  // Check the input parameters of the client PIN subcommand
  // getPinUvAuthTokenUsingUv.
  void ClientPinGetPinUvAuthTokenUsingUvTest();
  // Check the input parameters of the client PIN subcommand getUVRetries.
  void ClientPinGetUVRetriesTest();

 private:
  // Makes a credential for all tests that require one, for example assertions.
  cbor::Value MakeTestCredential(const std::string& rp_id,
                                 bool use_residential_key);
  // Tries to insert types other than the correct one into the CBOR builder.
  // Make sure to pass the appropriate CborBuilder for your command. The correct
  // types are inferred through the currently present builder entries. The tests
  // include other types than maps for the command and inner types of maps and
  // the first element of an inner array (assuming all array elements have the
  // same type). If that first element happens to be a map, its entries are also
  // checked. Even though this seems like an arbitrary choice at first, it
  // covers most of the CTAP input.
  void TestBadParameterTypes(Command command, CborBuilder* builder);
  // Tries to remove each parameter once. Make sure to pass the appropriate
  // CborBuilder for your command. The necessary parameters are inferred through
  // the currently present builder entries.
  void TestMissingParameters(Command command, CborBuilder* builder);
  // Tries to insert types other than the correct one into map entries. Those
  // maps themselves are values of the command parameter map. If
  // has_wrapping_array is true, the inner map is used as an array element
  // instead. To sum it up, the data structure tested can look like this:
  // command:outer_map_key->inner_map[key]->wrongly_typed_value or
  // command:outer_map_key->[inner_map[key]->wrongly_typed_value].
  void TestBadParametersInInnerMap(Command command, CborBuilder* builder,
                                   int outer_map_key,
                                   const cbor::Value::MapValue& inner_map,
                                   bool has_wrapping_array);
  // Tries to insert types other than the correct one into array elements. Those
  // arrays themselves are values of the command parameter map.
  void TestBadParametersInInnerArray(Command command, CborBuilder* builder,
                                     int outer_map_key,
                                     const cbor::Value& array_element);
  // Tries to insert a map or an array as a transport in an array of public key
  // credential descriptors. Both excludeList in MakeCredential and allowList in
  // GetAssertion expect this kind of value and share this test. Authenticators
  // must ignore unknown items in the transports list, so unexpected types are
  // untested. For arrays and maps though, the maximum nesting depth is reached.
  void TestCredentialDescriptorsArrayForCborDepth(Command command,
                                                  CborBuilder* builder,
                                                  int map_key,
                                                  const std::string& rp_id);
  DeviceInterface* device_;
  KeyChecker* key_checker_;
  CounterChecker* counter_checker_;
  // These are arbitrary example values for each CBOR type.
  std::map<cbor::Value::Type, cbor::Value> type_examples_;
  // This map is a subset of the type_examples_. Since CBOR implementations do
  // not need to allow all CBOR types as map keys, testing on all of them for
  // map keys might produce different error codes. Since we currently enforce
  // a specific error code, use this subset of CBOR types for all tests on map
  // keys. Allowed map keys might depend on the CBOR parser implementation.
  // The specification only states: "Note that this rule allows maps that have
  // keys of different types, even though that is probably a bad practice that
  // could lead to errors in some canonicalization implementations."
  std::map<cbor::Value::Type, cbor::Value> map_key_examples_;
  // This is an example of an EC cose key map for client PIN operations.
  cbor::Value::MapValue cose_key_example_;
};

class SpecificationProcedure : public TestSeries {
 public:
  SpecificationProcedure(DeviceInterface* device, KeyChecker* key_checker,
                         CounterChecker* counter_checker);
  // Tests if the authenticator checks the exclude list properly.
  void MakeCredentialExcludeListTest();
  // Tests correct behavior with different COSE algorithms. Tests non-existing
  // algorithm identifier and type.
  void MakeCredentialCoseAlgorithmTest();
  // Tests correct behavior when setting rk, up and uv.
  void MakeCredentialOptionsTest();
  // Tests if the PIN is correctly enforced. Resets afterwards to unset the PIN.
  void MakeCredentialPinAuthTest(bool is_fido_2_1_compliant);
  // Tests correct behavior when creating multiple keys. This test attempts to
  // create num_credentials credentials, stopping before that if the internal
  // key store is full. It resets afterwards to clear the storage.
  void MakeCredentialMultipleKeysTest(int num_credentials);
  // Tests if the key hardware actually interacts with a user. This test can not
  // be performed automatically, but requires tester feedback.
  void MakeCredentialPhysicalPresenceTest();
  // Tests if the user name is resistent to long inputs and bad UTF8.
  void MakeCredentialDisplayNameEncodingTest();
  // Tests if the HMAC-secret extension works properly.
  void MakeCredentialHmacSecretTest();
  // Tests correct behavior when setting rk, up and uv.
  void GetAssertionOptionsTest();
  // Tests correct differentiation between residential and non-residential.
  void GetAssertionResidentialKeyTest();
  // Tests if the PIN is correctly enforced. Resets afterwards to unset the PIN.
  void GetAssertionPinAuthTest(bool is_fido_2_1_compliant);
  // Tests if the key hardware actually interacts with a user. This test can not
  // be performed automatically, but requires tester feedback.
  void GetAssertionPhysicalPresenceTest();
  // Checks if the GetInfo command has valid output implicitly. Also checks for
  // support of PIN protocol version 1, because it is used throughout all tests.
  void GetInfoTest();
  // Check if FIDO version 2.1 is listed as a supported version.
  bool GetInfoIs2Point1Compliant();
  // Check if user verification is listed as a supported option.
  bool GetInfoHasUvOption();
  // Check if HMAC-secret is listed as a supported extension.
  bool GetInfoIsHmacSecretSupported();
  // Tests if the PIN minimum and maximum length are enforced correctly for the
  // SetPin and ChangePin command. Resets the device on failed tests so that the
  // following test will still find a valid state. Might end with the device
  // having a PIN set.
  void ClientPinRequirementsTest();
  // Tests if retries decrement properly and respond with correct error codes.
  // Creates a PIN if necessary. Resets the device at the end.
  void ClientPinRetriesTest();
  // Only tests the returned status code, just resets the authenticator.
  // Replugging the device before calling the function is necessary.
  void Reset();
  // Tests if the state on the device is wiped out.
  // Replugging the device before calling the function is necessary.
  void ResetDeletionTest();
  // Tests if requirements for resetting are enforced.
  void ResetPhysicalPresenceTest();
  // Tests if the state is persistent when being replugged. This includes
  // credentials and the PIN retries.
  void PersistenceTest();

 private:
  // Prompts the user to replug the device which is required before operations
  // that need a power cycle (i.e. resetting). The Init will then handle device
  // initilalization, regardless of the current state of the device.
  void PromptReplugAndInit();
  // Makes a credential for all tests that require one, for example assertions.
  // Works with or without a PIN being set.
  cbor::Value MakeTestCredential(const std::string& rp_id,
                                 bool use_residential_key);

  // Gets and checks the PIN retry counter response from the authenticator.
  int GetPinRetries();
  // Compute the shared secret between authenticator and platform. Sets the
  // argument platform_cose_key to the EC key used during the transaction.
  void ComputeSharedSecret();
  // Sets the PIN to the value specified in new_pin_utf8. Performs key agreement
  // if not already done. Safe to call multiple times, and only talks to the
  // authenticator if there is no PIN already. Defaults to 1234 if nothing else
  // is set. Fails if the PIN requirements are not satisfied.
  void SetPin(const cbor::Value::BinaryValue& new_pin_utf8 = {0x31, 0x32, 0x33,
                                                              0x34});
  // Calls the SetPin command with the given padded PIN. Fails if the length is
  // not a multiple of the AES block size. Returns the command's status code.
  // Performs key agreement if not already done.
  Status AttemptSetPin(const cbor::Value::BinaryValue& new_padded_pin);
  // Changes the current PIN to new_pin_utf8. Fails if the PIN requirements are
  // not satisfied. Creates a PIN if not already done.
  void ChangePin(const cbor::Value::BinaryValue& new_pin_utf8);
  // Calls the ChangePin command with the given padded PIN, using the currently
  // set PIN. Fails if the length is not a multiple of the AES block size.
  // Returns the command's status code. Creates a PIN if not already done.
  Status AttemptChangePin(const cbor::Value::BinaryValue& new_padded_pin);
  // Returns a PIN Auth token valid for this power cycle from the authenticator.
  // Sets the PIN to 1234 if no PIN exists.
  void GetAuthToken();
  // Calls the GetAuthToken command with the given PIN. Creates a PIN if
  // not already done. Returns the command's status code. If redo_key_agreement
  // is true, it brings the shared_secret back to a valid state. This is
  // necessary because authenticators reset the key agreement on failed PIN
  // hash checks. Setting redo_key_agreement is only used for specific failure
  // mode tests.
  Status AttemptGetAuthToken(const cbor::Value::BinaryValue& pin_utf8,
                             bool redo_key_agreement = true);
  // Checks if the PIN we currently assume is set works for getting an auth
  // token. This way, we don't have to trust only the returned status code
  // after a SetPin or ChangePin command. It does not actually return an auth
  // token, use GetAuthToken() in that case.
  void CheckPinByGetAuthToken();
  // Checks if the PIN is not currently set by trying to make a credential.
  // The MakeCredential command should fail when the authenticator is PIN
  // protected. Even though this test could fail in case of a bad implementation
  // of Make Credential, this kind of misbehavior would be caught in another
  // test.
  void CheckPinAbsenceByMakeCredential();
  DeviceInterface* device_;
  KeyChecker* key_checker_;
  CounterChecker* counter_checker_;
  // The PIN is persistent, the other state is kept for a power cycle.
  cbor::Value::MapValue platform_cose_key_;
  cbor::Value::BinaryValue shared_secret_;
  cbor::Value::BinaryValue pin_utf8_;
  cbor::Value::BinaryValue auth_token_;
  // This is an example PIN that should be different from the real PIN.
  const cbor::Value::BinaryValue bad_pin_;
};

}  // namespace fido2_tests

#endif  // TEST_SERIES_H_
