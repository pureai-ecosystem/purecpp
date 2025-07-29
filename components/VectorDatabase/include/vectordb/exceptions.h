#pragma once
#include <stdexcept>

namespace vdb {
struct VStoreError         : std::runtime_error { using std::runtime_error::runtime_error; };
struct InvalidConfiguration: VStoreError { using VStoreError::VStoreError; };
struct BackendClosed       : VStoreError { using VStoreError::VStoreError; };
struct DimensionMismatch   : VStoreError { using VStoreError::VStoreError; };
struct QueryError          : VStoreError { using VStoreError::VStoreError; };
struct InsertionError      : VStoreError { using VStoreError::VStoreError; };
} // namespace vdb
