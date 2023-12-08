#include <iostream>
#include "ta_libc.h"
#include "components.h"
#include <vector>
#include <filesystem>
#include <random>

class ThresholdStrategy : public strategyEngine {
public:
    // Constructor
    ThresholdStrategy(eventBus& Bus, double buyThreshold, double sellThreshold)
        : strategyEngine(Bus), buyThreshold_(buyThreshold), sellThreshold_(sellThreshold) 
    {
        // Subscribe to MarketData events
        bus.subscribe("MarketData", std::bind(&ThresholdStrategy::onMarketData, this, std::placeholders::_1));
        std::cout << "Strategy Subscribed to MarketData events" << std::endl;
    }

    // Function to handle MarketData events
    void onMarketData(const event& evnt)
    {
        std::cout << "Received MarketData event for timestamp: " << evnt.timestamp << std::endl;
        // Extract MarketData and generate signals
        const MarketData& marketData = extractMarketData(evnt);
        bool signal = generateSignal(marketData);

        std::string ts = marketData.timestamp;
        event signalEvent{"Signal", ts, signal ? "Buy" : "Sell"};

        bus.publish(signalEvent);
    }

    // Override the generateSignal function with the threshold strategy logic
    bool generateSignal(const MarketData& marketData) override
    {
        std::random_device rd;

        // Use the random device to seed the random engine
        std::mt19937 gen(rd());

        // Define a range for the random numbers (in this case, from 1 to 100)
        std::uniform_int_distribution<int> distribution(0.5, 1.5);

        // Generate a random number within the specified range
        double randNum = distribution(gen);

        if (marketData.close > marketData.close*randNum) {
            std::cout << "Generated Buy signal for timestamp: " << marketData.timestamp << std::endl;
            return true;
        } else if (marketData.close < marketData.close*randNum) {
            std::cout << "Generated Sell signal for timestamp: " << marketData.timestamp << std::endl;
            return false;
        } else {
            std::cout << "No signal generated for timestamp: " << marketData.timestamp << std::endl;
            return false;
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
    std::filesystem::path HDpath=ORpath/"Backtesting/HistoricalData/4h/converted/bybit/BTCUSDT.csv";
    dataLoader abbas(HDpath);
    std::vector<MarketData> histData=abbas.dataGet();
    std::cout<<"gotten"<<std::endl;
    eventBus buss;
    ThresholdStrategy myStrategy(buss, 10, 50);
    dataHandler handler(buss,histData);
    broker amirreza(buss);

    handler.simulateMarketData();


    return 0;
}