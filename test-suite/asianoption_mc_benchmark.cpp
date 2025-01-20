/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2023 Devin AI
 
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

#include <ql/qldefines.hpp>
#include <ql/instruments/asianoption.hpp>
#include <ql/pricingengines/asian/mc_discr_arith_av_price.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/termstructures/volatility/equityfx/blackconstantvol.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/exercise.hpp>
#include <ql/types.hpp>
#include <chrono>
#include <iostream>

using namespace QuantLib;

namespace {

    void runAsianOptionMCBenchmark() {
        std::cout << "\nRunning Asian Option Monte Carlo Benchmark..." << std::endl;
        
        // Set up market data
        Real underlying = 100.0;
        Real strike = 100.0;
        Rate riskFreeRate = 0.05;
        Volatility volatility = 0.20;
        
        Date today = Date::todaysDate();
        Date maturity = today + 360;
        
        // Option parameters
        Option::Type type = Option::Call;
        
        // Create required QuantLib objects
        auto spot = ext::make_shared<SimpleQuote>(underlying);
        auto rTS = ext::make_shared<FlatForward>(today, riskFreeRate, Actual360());
        auto volTS = ext::make_shared<BlackConstantVol>(today, NullCalendar(), volatility, Actual360());
        
        auto bsProcess = ext::make_shared<BlackScholesProcess>(
            Handle<Quote>(spot),
            Handle<YieldTermStructure>(rTS),
            Handle<BlackVolTermStructure>(volTS));

        // Create the option
        std::vector<Date> fixingDates;
        for (Size i = 1; i <= 5; ++i) {
            fixingDates.push_back(today + i * 72); // 5 equally spaced dates
        }
        
        auto payoff = ext::make_shared<PlainVanillaPayoff>(type, strike);
        auto exercise = ext::make_shared<EuropeanExercise>(maturity);
        
        DiscreteAveragingAsianOption asianOption(Average::Arithmetic,
            fixingDates, payoff, exercise);

        // Create the MC engine with specified parameters
        Size timeSteps = 5;
        Size paths = 50;
        Size dimensions = 4;
        
        auto mcEngine = MakeMCDiscreteArithmeticAPEngine<PseudoRandom>(bsProcess)
            .withSamples(paths)
            .withBrownianBridge()
            .withAntitheticVariate()
            .withControlVariate();
            
        asianOption.setPricingEngine(mcEngine);

        // Measure execution time
        auto start = std::chrono::high_resolution_clock::now();
        
        Real price = asianOption.NPV();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Asian Option MC Benchmark Results:" << std::endl
                  << "Paths: " << paths << std::endl
                  << "Time Steps: " << timeSteps << std::endl
                  << "Dimensions: " << dimensions << std::endl
                  << "Price: " << price << std::endl
                  << "Execution time: " << duration.count() << " microseconds" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    try {
        runAsianOptionMCBenchmark();
        return 0;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
        return 1;
    }
}
