#include <iostream>
#include <vector>
#include <functional>
#include <queue>
#include <string>
#include "components.h"

void eventBus::subscribe(const std::string& eventType, std::function<void(event&)> callback)
{
    subscribers[eventType].push_back(callback);
}

void eventBus::publish(event& evnt)
{
    const auto& eventType=evnt.type;
    if (subscribers.find(eventType) != subscribers.end())
    {
        for (const auto& subscriber : subscribers[eventType])
        {
            subscriber(evnt);
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
        marketDataEvent mDataEvent(Data.timestamp,Data);
        bus.publish(mDataEvent);
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

    marketDataEvent mDataEvent{Data.timestamp, Data};
    bus.publish(mDataEvent);
}

MarketData strategyEngine::extractMarketData(const marketDataEvent& evnt)
{
    // Extract relevant fields from the event
    MarketData marketData(
        evnt.data_.timestamp,
        evnt.data_.open,
        evnt.data_.high,
        evnt.data_.low,
        evnt.data_.close,
        evnt.data_.volume
    );

    return marketData;
}

void strategyEngine::onMarketData(const marketDataEvent& evnt) 
{
    std::cout << "Received MarketData event" << std::endl;
    const MarketData& marketData = strategyEngine::extractMarketData(evnt);
    std::unordered_map<std::string,double> signal = strategyEngine::generateSignal(marketData);

    std::string ts=marketData.timestamp;
    signalEvent sigEvent{ts, {{"type",0}}};

    bus.publish(sigEvent);
}

std::unordered_map<std::string,double> strategyEngine::generateSignal(const MarketData& marketData)
{
    return {{"type",0}};
}

void broker::onSignal(const signalEvent& evnt) 
{

    // Check the signal and execute the corresponding order
    if (evnt.data_.at("type") == 1.0 && !inPosition) 
    {
        executeBuyOrder(evnt);
    } 
    else if (evnt.data_.at("type") == 2.0 && inPosition) 
    {
        executeSellOrder(evnt);
    }
}

//void broker::executeBuyOrder(const MarketData& marketData) 
void broker::executeBuyOrder(const signalEvent& evnt)
{
    cash -= cash*evnt.data_.at("fraction");
    inPosition = true;

    std::cout << evnt.timestamp<<" | "<< "Executed BUY order | Cash: "<< cash << std::endl;
    std::cout<<std::endl;
}

void broker::executeSellOrder(const signalEvent& evnt) 
{
    if(inPosition==true)
    {
        cash += asset*evnt.data_.at("fraction")*evnt.data_.at("closePrice");
        asset=asset-asset*evnt.data_.at("fraction");
        inPosition = false;
    }
    std::cout << evnt.timestamp<<" | "<< "Executed SELL order | Cash: "<< cash << std::endl;
    std::cout<<std::endl;
}