/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2002, 2003 Ferdinando Ametrano
 Copyright (C) 2000, 2001, 2002, 2003 RiskMap srl
 Copyright (C) 2003, 2004, 2005, 2006 StatPro Italia srl

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

/*! \file pathgenerator.hpp
    \brief Generates random paths using a sequence generator
*/

#ifndef quantlib_montecarlo_path_generator_hpp
#define quantlib_montecarlo_path_generator_hpp

#include <ql/methods/montecarlo/brownianbridge.hpp>
#include <ql/stochasticprocess.hpp>
#include <ql/math/randomnumbers/rngbuffer.hpp>
#include <utility>

namespace QuantLib {
    class StochasticProcess;
    class StochasticProcess1D;
    //! Generates random paths using a sequence generator
    /*! Generates random paths with drift(S,t) and variance(S,t)
        using a gaussian sequence generator

        \ingroup mcarlo

        \test the generated paths are checked against cached results
    */
    template <class GSG>
    class PathGenerator {
      public:
        typedef Sample<Path> sample_type;
        // constructors
        PathGenerator(const ext::shared_ptr<StochasticProcess>&,
                      Time length,
                      Size timeSteps,
                      GSG generator,
                      bool brownianBridge);
        PathGenerator(const ext::shared_ptr<StochasticProcess>&,
                      TimeGrid timeGrid,
                      GSG generator,
                      bool brownianBridge);
        //! \name inspectors
        //@{
        sample_type next() const;
        sample_type antithetic() const;
        Size size() const { return dimension_; }
        const TimeGrid& timeGrid() const { return timeGrid_; }
        //@}
      private:
        sample_type next(bool antithetic) const;
        bool brownianBridge_;
        Size dimension_;
        TimeGrid timeGrid_;
        ext::shared_ptr<StochasticProcess1D> process_;
        mutable RngBuffer<GSG> generator_;
        BrownianBridge bb_;
    };


    // template definitions

    template <class GSG>
    PathGenerator<GSG>::PathGenerator(const ext::shared_ptr<StochasticProcess>& process,
                                      Time length,
                                      Size timeSteps,
                                      GSG generator,
                                      bool brownianBridge)
    : brownianBridge_(brownianBridge), 
      dimension_(timeSteps),
      timeGrid_(length, timeSteps),
      process_(ext::dynamic_pointer_cast<StochasticProcess1D>(process)),
      generator_(std::move(generator)),
      bb_(timeGrid_) {
        QL_REQUIRE(process_, "process cannot be null");
        QL_REQUIRE(timeSteps > 0, "timeSteps must be positive");
        QL_REQUIRE(dimension_ == timeSteps,
                   "sequence generator dimensionality (" << dimension_
                   << ") != timeSteps (" << timeSteps << ")");
    }

    template <class GSG>
    PathGenerator<GSG>::PathGenerator(const ext::shared_ptr<StochasticProcess>& process,
                                      TimeGrid timeGrid,
                                      GSG generator,
                                      bool brownianBridge)
    : brownianBridge_(brownianBridge),
      dimension_(timeGrid.size()-1), 
      timeGrid_(std::move(timeGrid)),
      process_(ext::dynamic_pointer_cast<StochasticProcess1D>(process)),
      generator_(generator),
      bb_(timeGrid_) {
        QL_REQUIRE(dimension_==timeGrid_.size()-1,
                   "sequence generator dimensionality (" << dimension_
                   << ") != timeSteps (" << timeGrid_.size()-1 << ")");
    }

    template <class GSG>
    typename PathGenerator<GSG>::sample_type
    PathGenerator<GSG>::next() const {
        return next(false);
    }

    template <class GSG>
    typename PathGenerator<GSG>::sample_type
    PathGenerator<GSG>::antithetic() const {
        return next(true);
    }

    template <class GSG>
    typename PathGenerator<GSG>::sample_type
    PathGenerator<GSG>::next(bool antithetic) const {
        // Create local storage for this path generation
        sample_type result(Path(timeGrid_), 1.0);
        std::vector<Real> temp(dimension_);
        
        auto rng_sequence = antithetic ? 
            generator_.lastSequence() : generator_.nextSequence();

        if (brownianBridge_) {
            bb_.transform(rng_sequence.value.begin(),
                        rng_sequence.value.end(),
                        temp.begin());
        } else {
            std::copy(rng_sequence.value.begin(),
                     rng_sequence.value.end(),
                     temp.begin());
        }

        result.weight = rng_sequence.weight;
        result.value.front() = process_->x0();

        #pragma omp parallel for if(result.value.length() > 4)
        for (Size i=1; i<result.value.length(); i++) {
            Time t = timeGrid_[i-1];
            Time dt = timeGrid_.dt(i-1);
            result.value[i] = process_->evolve(t, result.value[i-1], dt,
                                           antithetic ? -temp[i-1] : temp[i-1]);
        }

        return result;
    }

}


#endif
