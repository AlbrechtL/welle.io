/*
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

#include "fft_engine.h"

FFTEngine::FFTEngine() : fft_plan_(NULL), ifft_plan_(NULL), n_fft_(0){
}

FFTEngine::FFTEngine(int nfft) : n_fft_(nfft){
    // create fft plan using temp data arrays
    // TODO: consider using WISDOM to export plan data
    float *tmp = new float[2 * nfft];
    fft_plan_ = fftwf_plan_dft_1d(nfft, reinterpret_cast<fftwf_complex*>(tmp), reinterpret_cast<fftwf_complex*>(tmp), FFTW_FORWARD, FFTW_ESTIMATE | FFTW_UNALIGNED);
    ifft_plan_ = fftwf_plan_dft_1d(nfft, reinterpret_cast<fftwf_complex*>(tmp), reinterpret_cast<fftwf_complex*>(tmp), FFTW_BACKWARD, FFTW_ESTIMATE | FFTW_UNALIGNED);
    delete [] tmp;
}

FFTEngine::~FFTEngine() {
    if (n_fft_ != 0) {
        fftwf_destroy_plan(fft_plan_);
        fftwf_destroy_plan(ifft_plan_);
    }
}
