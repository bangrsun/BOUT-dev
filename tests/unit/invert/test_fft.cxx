#include "gtest/gtest.h"

#include "dcomplex.hxx"
#include "fft.hxx"
#include "test_extras.hxx"
#include "bout/array.hxx"
#include "bout/constants.hxx"

#include <algorithm>
#include <iostream>
#include <numeric>

class FFTTest : public ::testing::TestWithParam<int> {
public:
  FFTTest()
      : size(GetParam()), nmodes((size / 2) + 1), real_signal(size), fft_signal(nmodes) {

    // Make sure fft functions are quiet by setting fft_measure to false
    bout::fft::fft_init(false);

    // Make grid indices from [0, size - 1]
    Array<BoutReal> indices{size};
    std::iota(indices.begin(), indices.end(), 0.0);

    // Calculate sin(x) + cos(2x) on [0, 2pi]
    std::transform(indices.begin(), indices.end(), real_signal.begin(),
                   [this](BoutReal i) -> BoutReal {
                     return std::sin(i * TWOPI / size) + std::cos(2. * i * TWOPI / size);
                   });

    // Make a spectrum with two frequencies
    std::fill(fft_signal.begin(), fft_signal.end(), dcomplex{0., 0.});
    fft_signal[1] = dcomplex{0., -0.5};
    fft_signal[2] = dcomplex{0.5, 0.};
  };

  const int size;
  const int nmodes;

  Array<BoutReal> real_signal;
  Array<dcomplex> fft_signal;

  // FFTs have a slightly looser tolerance than other functions
  static constexpr BoutReal fft_tolerance{1.e-12};
};

// Test the FFT functions with both even- and odd-length real signals
INSTANTIATE_TEST_CASE_P(FFTEvenAndOddSamples, FFTTest, ::testing::Values(8, 9));

TEST_P(FFTTest, rfft) {

  Array<dcomplex> output{nmodes};

  // Compute forward real FFT
  rfft(real_signal.begin(), size, output.begin());

  EXPECT_EQ(output.size(), nmodes);

  for (int i = 0; i < nmodes; ++i) {
    EXPECT_NEAR(real(output[i]), real(fft_signal[i]), fft_tolerance);
    EXPECT_NEAR(imag(output[i]), imag(fft_signal[i]), fft_tolerance);
  }
}

TEST_P(FFTTest, irfft) {

  Array<BoutReal> output{size};

  // Compute inverse real FFT
  irfft(fft_signal.begin(), size, output.begin());

  EXPECT_EQ(output.size(), size);

  for (int i = 0; i < size; ++i) {
    EXPECT_NEAR(output[i], real_signal[i], fft_tolerance);
  }
}

TEST_P(FFTTest, rfftWithArray) {

  // Compute forward real FFT
  Array<dcomplex> output{bout::fft::rfft(real_signal)};

  EXPECT_EQ(output.size(), nmodes);

  for (int i = 0; i < nmodes; ++i) {
    EXPECT_NEAR(real(output[i]), real(fft_signal[i]), fft_tolerance);
    EXPECT_NEAR(imag(output[i]), imag(fft_signal[i]), fft_tolerance);
  }
}

TEST_P(FFTTest, irfftWithArray) {

  // Compute inverse real FFT
  Array<BoutReal> output{bout::fft::irfft(fft_signal, size)};

  EXPECT_EQ(output.size(), size);

  for (int i = 0; i < size; ++i) {
    EXPECT_NEAR(output[i], real_signal[i], fft_tolerance);
  }
}

TEST_P(FFTTest, RoundTrip) {

  // Checks normalisation is == 1
  Array<BoutReal> output{bout::fft::irfft(bout::fft::rfft(real_signal), size)};

  EXPECT_EQ(output.size(), real_signal.size());

  for (int i = 0; i < size; ++i) {
    EXPECT_NEAR(output[i], real_signal[i], fft_tolerance);
  }
}