#include "array/zfparray1.h"
using namespace zfp;

extern "C" {
  #include "constants/1dDouble.h"
  #include "utils/hash64.h"
};

#include "gtest/gtest.h"
#include "utils/gtestDoubleEnv.h"
#include "utils/gtestBaseFixture.h"
#include "utils/predicates.h"

class Array1dTestEnv : public ArrayDoubleTestEnv {
public:
  virtual int getDims() { return 1; }
};

class Array1dTest : public ArrayNdTestFixture {};

TEST_F(Array1dTest, when_constructorCalled_then_rateSetWithWriteRandomAccess)
{
  double rate = ZFP_RATE_PARAM_BITS;
  array1d arr(inputDataTotalLen, rate);
  EXPECT_LT(rate, arr.rate());
}

TEST_F(Array1dTest, when_setRate_then_compressionRateChanged)
{
  double oldRate = ZFP_RATE_PARAM_BITS;
  array1d arr(inputDataTotalLen, oldRate, inputDataArr);

  double actualOldRate = arr.rate();
  size_t oldCompressedSize = arr.compressed_size();
  uint64 oldChecksum = hashBitstream((uint64*)arr.compressed_data(), oldCompressedSize);

  double newRate = oldRate - 10;
  EXPECT_LT(1, newRate);
  arr.set_rate(newRate);
  EXPECT_GT(actualOldRate, arr.rate());

  arr.set(inputDataArr);
  size_t newCompressedSize = arr.compressed_size();
  uint64 checksum = hashBitstream((uint64*)arr.compressed_data(), newCompressedSize);

  EXPECT_PRED_FORMAT2(ExpectNeqPrintHexPred, oldChecksum, checksum);

  EXPECT_GT(oldCompressedSize, newCompressedSize);
}

TEST_F(Array1dTest, when_generateRandomData_then_checksumMatches)
{
  EXPECT_PRED_FORMAT2(ExpectEqPrintHexPred, CHECKSUM_ORIGINAL_DATA_ARRAY, hashArray((uint64*)inputDataArr, inputDataTotalLen, 1));
}

// with write random access in 1d, fixed-rate params rounded up to multiples of 16
INSTANTIATE_TEST_CASE_P(TestManyCompressionRates, Array1dTest, ::testing::Values(1, 2));

TEST_P(Array1dTest, given_dataset_when_set_then_underlyingBitstreamChecksumMatches)
{
  array1d arr(inputDataTotalLen, getRate());

  uint64 expectedChecksum = getExpectedBitstreamChecksum();
  uint64 checksum = hashBitstream((uint64*)arr.compressed_data(), arr.compressed_size());
  EXPECT_PRED_FORMAT2(ExpectNeqPrintHexPred, expectedChecksum, checksum);

  arr.set(inputDataArr);

  checksum = hashBitstream((uint64*)arr.compressed_data(), arr.compressed_size());
  EXPECT_PRED_FORMAT2(ExpectEqPrintHexPred, expectedChecksum, checksum);
}

TEST_P(Array1dTest, given_setArray1d_when_get_then_decompressedValsReturned)
{
  array1d arr(inputDataTotalLen, getRate(), inputDataArr);

  double* decompressedArr = new double[inputDataTotalLen];
  arr.get(decompressedArr);

  uint64 expectedChecksum = getExpectedDecompressedChecksum();
  uint64 checksum = hashArray((uint64*)decompressedArr, inputDataTotalLen, 1);
  EXPECT_PRED_FORMAT2(ExpectEqPrintHexPred, expectedChecksum, checksum);

  delete[] decompressedArr;
}

TEST_P(Array1dTest, given_populatedCompressedArray_when_resizeWithClear_then_bitstreamZeroed)
{
  array1d arr(inputDataTotalLen, getRate());
  arr.set(inputDataArr);
  EXPECT_NE(0, hashBitstream((uint64*)arr.compressed_data(), arr.compressed_size()));

  arr.resize(inputDataTotalLen + 1, true);

  EXPECT_EQ(0, hashBitstream((uint64*)arr.compressed_data(), arr.compressed_size()));
}

TEST_P(Array1dTest, when_configureCompressedArrayFromDefaultConstructor_then_bitstreamChecksumMatches)
{
  array1d arr;
  arr.resize(inputDataTotalLen, false);
  arr.set_rate(getRate());
  arr.set(inputDataArr);

  uint64 expectedChecksum = getExpectedBitstreamChecksum();
  uint64 checksum = hashBitstream((uint64*)arr.compressed_data(), arr.compressed_size());
  EXPECT_PRED_FORMAT2(ExpectEqPrintHexPred, expectedChecksum, checksum);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::Environment* const foo_env = ::testing::AddGlobalTestEnvironment(new Array1dTestEnv);
  return RUN_ALL_TESTS();
}