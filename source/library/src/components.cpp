#include <iostream>
#include <vector>
#include <functional>
#include <queue>
#include <string>
#include "components.h"

void eventBus::subscribe(const std::string& eventType, std::function<void(const event&)> callback)
{
    subscribers[eventType].push_back(callback);
    std::cout << "Subscribed to event type: " << eventType << std::endl;
}

void eventBus::publish(const event& event)
{
    const auto& eventType=event.type;
    if (subscribers.find(eventType) != subscribers.end())
    {
        for (const auto& subscriber : subscribers[eventType])
        {
            subscriber(event);
        }
    }
    else
    {
        std::cout << "No subscribers for event type: " << eventType << std::endl;
    }
}

void dataHandler::simulateMarketData() 
{
    for (const auto& Data : historicalMarketData) 
    {
        event marketDataEvent=event("MarketData",Data.timestamp,Data);
        bus.publish(marketDataEvent);
    }
}

MarketData dataHandler::getNextMarketData()
{
    if (currentDataIndex < historicalMarketData.size()) 
    {
        std::cout<<"data with this ts has been passed: "<<historicalMarketData[currentDataIndex].timestamp<<std::endl;
        return historicalMarketData[currentDataIndex++];
    } 
    else 
    {
        // Return an empty MarketData structure to signal the end of data
        return {"",0,0,0,0,0};
    }
}

void dataHandler::simulateNextMarketDataEvent() 
{
    MarketData Data = dataHandler::getNextMarketData();
    if (Data.timestamp.empty()) 
    {
        std::cout << "No more data to simulate." << std::endl;
        return;
    }

    event marketDataEvent{"MarketData", Data.timestamp, Data};
    bus.publish(marketDataEvent);
}

MarketData strategyEngine::extractMarketData(const event& evnt)
{
    // Extract relevant fields from the event
    MarketData marketData(
        evnt.dataMarket.timestamp,
        evnt.dataMarket.open,
        evnt.dataMarket.high,
        evnt.dataMarket.low,
        evnt.dataMarket.close,
        evnt.dataMarket.volume
    );

    return marketData;
}

void strategyEngine::onMarketData(const event& evnt) 
{
    std::cout << "Received MarketData event" << std::endl;
    const MarketData& marketData = strategyEngine::extractMarketData(evnt);
    bool signal = strategyEngine::generateSignal(marketData);

    std::string ts=marketData.timestamp;
    event signalEvent{"Signal",ts,signal ? "Buy" : "Sell"};

    bus.publish(signalEvent);
}

bool strategyEngine::generateSignal(const MarketData& marketData)
{
    return false;
}

void broker::onSignal(const event& evnt) {

    // Check the signal and execute the corresponding order
    if (evnt.dataSignal == "Buy" && !inPosition) {
        executeBuyOrder(evnt.dataMarket);
    } else if (evnt.dataSignal == "Sell" && inPosition) 
    {
        executeSellOrder(evnt.dataMarket);
    }
}

void broker::executeBuyOrder(const MarketData& marketData) 
{
    double buyAmount = 0.01*cash;
    cash -= buyAmount;
    inPosition = true;

    std::cout << marketData.timestamp<<" | "<< "Executed BUY order | Cash: "<< cash << std::endl;
    std::cout<<std::endl;
}

void broker::executeSellOrder(const MarketData& marketData) {
    if(inPosition==true)
    {
        double sellAmount = cash;
        cash += sellAmount;
        inPosition = false;
    }
    std::cout << marketData.timestamp<<" | "<< "Executed SELL order | Cash: "<< cash << std::endl;
    std::cout<<std::endl;
}