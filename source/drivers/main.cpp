#include <iostream>
#include "ta_libc.h"
#include "components.h"
#include <vector>
#include <filesystem>
#include <random>
#include <memory>
#include <chrono>

class ThresholdStrategy : public strategyEngine {
public:
    // Constructor
    ThresholdStrategy(eventBus& Bus, double buyThreshold, double sellThreshold)
        : strategyEngine(Bus), buyThreshold_(buyThreshold), sellThreshold_(sellThreshold) 
    {
        // Subscribe to MarketData events
        //bus.subscribe("MarketData", std::bind(&ThresholdStrategy::onMarketData, this, std::placeholders::_1));
        //bus.subscribe("MarketData", std::function<void(const marketDataEvent&)>([this](const marketDataEvent& evnt) { this->onMarketData(evnt); }));
        bus.subscribe("MarketData", [this](event& evnt) 
        {
            if (auto marketDataEventPtr = dynamic_cast<marketDataEvent*>(&evnt)) {
                this->onMarketData(*marketDataEventPtr);
            }
        });
        std::cout << "Strategy Subscribed to MarketData events" << std::endl;
    }

    // Function to handle MarketData events
    void onMarketData(const marketDataEvent& evnt)
    {
        std::cout << "Received MarketData event for timestamp: " << evnt.timestamp << std::endl;
        // Extract MarketData and generate signals
        const MarketData& marketData = extractMarketData(evnt);
        std::unordered_map<std::string,double> signal = generateSignal(marketData);

        std::string ts = marketData.timestamp;
        signalEvent sigEvent{ts, signal};

        bus.publish(sigEvent);
    }

    // Override the generateSignal function with the threshold strategy logic
    std::unordered_map<std::string,double> generateSignal(const MarketData& marketData) override
    {
        std::random_device rd;

        // Use the random device to seed the random engine
        std::mt19937 gen(rd());

        // Define a range for the random numbers (in this case, from 1 to 100)
        std::uniform_int_distribution<int> distribution(0, 3);

        // Generate a random number within the specified range
        double randNum = distribution(gen)/2.0;

        if (marketData.close > marketData.close*randNum) {
            std::cout << "Generated Buy signal for timestamp: " << marketData.timestamp << std::endl;
            return {
                {"type",1},
                {"fraction",0.95}
            };
        } else if (marketData.close < marketData.close*randNum) {
            std::cout << "Generated Sell signal for timestamp: " << marketData.timestamp << std::endl;
            return {
                {"type",2},
                {"fraction",1.0},
                {"closePrice",marketData.close}
            };
        } else {
            std::cout << "No signal generated for timestamp: " << marketData.timestamp << std::endl;
            return {{"type",0}};
        }
    }

private:
    double buyThreshold_;
    double sellThreshold_;
};


int main() 
{
    std::filesystem::path crpth=std::filesystem::current_path();
    std::filesystem::path ORpath=crpth.parent_path().parent_path();
    std::filesystem::path HDpath=ORpath/"Backtesting/HistoricalData/1m/converted/bybit/BTCUSDT.csv";
    
    auto startOld = std::chrono::high_resolution_clock::now();
    dataLoader abbas(HDpath);
    auto endOld = std::chrono::high_resolution_clock::now();
    auto durationOld = std::chrono::duration_cast<std::chrono::milliseconds>(endOld - startOld).count();

    auto startNew = std::chrono::high_resolution_clock::now();
    dataLoaderAsync abbas2(HDpath);
    auto endNew = std::chrono::high_resolution_clock::now();
    auto durationNew = std::chrono::duration_cast<std::chrono::milliseconds>(endNew - startNew).count();

    // std::filesystem::path crpth=std::filesystem::current_path();
    // std::filesystem::path ORpath=crpth.parent_path().parent_path();
    // std::filesystem::path HDpath=ORpath/"Backtesting/HistoricalData/4h/converted/bybit/BTCUSDT.csv";
    // dataLoader abbas(HDpath);
    // dataLoaderASync abbas(HDpath);
    // std::vector<MarketData> histData=abbas.dataGet();
    // eventBus buss;
    // ThresholdStrategy myStrategy(buss, 10, 50);
    // dataHandler handler(buss,histData);
    // broker amirreza(buss);
    // std::cout<<"initiation done";
    // handler.simulateMarketData();

    
    std::cout << "Execution time Old: " << durationOld << " ms" << std::endl;
    std::cout << "Execution time New: " << durationNew << " ms" << std::endl;

    return 0;
}