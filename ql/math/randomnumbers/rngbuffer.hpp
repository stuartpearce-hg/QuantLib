/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2024 Devin AI

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*! \file rngbuffer.hpp
    \brief Random number generator buffer for efficient pre-generation and reuse
*/

#ifndef quantlib_rng_buffer_hpp
#define quantlib_rng_buffer_hpp

#include <ql/methods/montecarlo/sample.hpp>
#include <vector>

namespace QuantLib {

    //! Random number generator buffer
    /*! Pre-generates and stores random numbers for efficient reuse.
        Particularly effective for small path counts where RNG overhead
        is significant relative to path generation time.
        
        The buffer size is fixed at construction time and numbers are
        pre-generated in blocks for efficiency. When the buffer is
        exhausted, it automatically refills using the underlying RNG.
    */
    template <class RNG>
    class RngBuffer {
      public:
        typedef typename RNG::sample_type sample_type;
        
        RngBuffer(RNG rng)  // Take RNG by value
        : rng_(std::move(rng)),
          dimension_(rng_.dimension()),
          bufferSize_(50),
          currentIndex_(bufferSize_),
          buffer_(bufferSize_ * dimension_),
          sequence_(std::vector<Real>(dimension_), 1.0) {
            refillBuffer();
        }
        
        //! Get next random sequence from buffer
        sample_type nextSequence() const {
            if (currentIndex_ >= bufferSize_) {
                refillBuffer();
                currentIndex_ = 0;
            }
            
            // Copy values from buffer to sequence
            for (Size i = 0; i < dimension_; ++i) {
                sequence_.value[i] = buffer_[currentIndex_ * dimension_ + i];
            }
            sequence_.weight = 1.0;
            
            currentIndex_++;
            return sequence_;
        }
        
        //! Get last generated sequence
        sample_type lastSequence() const {
            return sequence_;
        }
        
        Size dimension() const { return dimension_; }
        
      private:
        void refillBuffer() const {
            for (Size i = 0; i < bufferSize_; ++i) {
                sample_type seq = rng_.nextSequence();
                std::copy(seq.value.begin(),
                         seq.value.end(),
                         buffer_.begin() + i * dimension_);
            }
        }
        
        mutable RNG rng_;
        Size dimension_;
        Size bufferSize_;
        mutable Size currentIndex_;
        mutable std::vector<Real> buffer_;
        mutable sample_type sequence_;
    };

} // namespace QuantLib

#endif
