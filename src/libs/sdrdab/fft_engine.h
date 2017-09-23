/**
 * @class FFTEngine
 * @brief Wrapper to libfftw3
 *
 * FFT engine usually has to be initialized and here is a good place for it.
 *
 * @author Jaroslaw Bulat kwant@agh.edu.pl (FFTEngine)
 * @author Michal Rzepka (FFTEngine, FFTEngine::FFT, FFTEngine::IFFT)
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0 
 * @pre libfftw3
 * @copyright Copyright (c) 2015 Jaroslaw Bulat, Michal Rzepka
 *
 * @par License
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef FFT_ENGINE_H_
#define FFT_ENGINE_H_

#include "fftw3.h"

class FFTEngine {
    public:
        /**
         * Initialize FFT engine.
         */
        FFTEngine();

        /**
         * Initialize FFT engine and generate fftw plans
         * @param nfft - size of transforms
         */
        FFTEngine(int nfft);

        /**
         * perform in-place FFT
         * @param data - pointer to N-size FFT, result is stored in the same memory
         * fftw3f linked - float precision
         * data array size -> 2*fft_n
         * data[0] - first number's real part, data[1] - first number's imaginary part, etc...
         */
        inline void FFT(float *data) {
            fftwf_execute_dft(fft_plan_, reinterpret_cast<fftwf_complex*>(data), reinterpret_cast<fftwf_complex*>(data));
        }

        /**
         * perform in-place IFFT
         * @param data - pointer to N-size IFFT, result is stored in the same memory
         * n_fft_ - number of complex samples
         * data array size -> 2 * ifft_n
         * data[0] - first number's real part, data[1] - first number's imaginary part, etc...
         */
        inline void IFFT(float *data) {
            fftwf_execute_dft(ifft_plan_, reinterpret_cast<fftwf_complex*>(data), reinterpret_cast<fftwf_complex*>(data));
            // normalization
            for (int i = 0; i < n_fft_ * 2; i++) {
                data[i] /= n_fft_;      ///< TODO: possible optimization (remove), IFFT is used only in initialization, not in realtime
            }
        }

        virtual ~FFTEngine();

#ifndef GOOGLE_UNIT_TEST
    private:
#endif
        fftwf_plan fft_plan_; ///< pre-computed FFT plan
        fftwf_plan ifft_plan_; ///< pre-computed IFFT plan
        int n_fft_; ///< size of transform
};

#endif /* FFT_ENGINE_H_ */
