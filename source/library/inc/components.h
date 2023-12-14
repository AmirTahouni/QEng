#include <iostream>
#include <vector>
#include <functional>
#include <queue>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <ctime>
#include <stdlib.h>
#include <unordered_map>
#include <memory>
#include <thread>
#include <future>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>

std::string convertTimestamp(const std::string& timestampString, const char* format = "%Y-%m-%d %H:%M:%S") {
    // Convert string to double
    double timestamp;
    std::istringstream iss(timestampString);
    iss >> timestamp;

    if (iss.fail()) {
        // Handle parsing error
        return "Invalid timestamp format";
    }

    // Convert milliseconds to std::chrono::milliseconds
    std::chrono::milliseconds milliseconds(static_cast<long long>(timestamp));

    // Get the duration since the epoch
    auto durationSinceEpoch = std::chrono::duration_cast<std::chrono::system_clock::duration>(milliseconds);

    // Get the time point representing the timestamp
    auto timePoint = std::chrono::time_point<std::chrono::system_clock>(durationSinceEpoch);

    // Convert the time point to std::tm
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm timeStruct = *std::localtime(&time);

    // Create a buffer to store the formatted timestamp
    char buffer[80];

    // Use std::strftime to format the timestamp
    std::strftime(buffer, sizeof(buffer), format, &timeStruct);

    // Convert the buffer to a string and return
    return std::string(buffer);
}

struct MarketData 
{ 
    MarketData(std::string ts, double o, double h, double l, double c, double v):
        timestamp(ts),open(o),high(h),low(l),close(c),volume(v){}
    MarketData():timestamp(""),open(0),high(0),low(0),close(0),volume(0){}

    std::string timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume;
};

class dataLoader
{
public:
    dataLoader(std::filesystem::path path): filePath(path)
    {
        std::ifstream file(filePath);

        if (!file.is_open()) 
        {
            std::cerr << "Error opening file: " << filePath << std::endl;
        }
        
        std::string header;
        std::getline(file, header);

        std::string line;
        while (std::getline(file, line)) 
        {
            std::istringstream ss(line);
            MarketData data;

            // Read and ignore the timestamp in original form
            std::string timestampMisc;
            std::string tstamp;
            std::getline(ss, timestampMisc, ','); // Read and discard
            std::getline(ss, tstamp, ',');
            data.timestamp=convertTimestamp(tstamp);
            std::string openStr, highStr, lowStr, closeStr, volumeStr;

            std::getline(ss, openStr,',');   // Open
            std::getline(ss, highStr,',');   // High
            std::getline(ss, lowStr,',');    // Low
            std::getline(ss, closeStr,',');  // Close
            std::getline(ss, volumeStr,',');            // Volume

            // Convert string values to doubles
            data.open = std::stod(openStr);
            data.high = std::stod(highStr);
            data.low = std::stod(lowStr);
            data.close = std::stod(closeStr);
            data.volume = std::stod(volumeStr);

            data_.push_back(data);
        }
        file.close();
    }

    std::vector<MarketData> dataGet()
    {
        return data_;
    }

    void printData()
    {
        for (const auto& data : data_) 
        {
            std::cout << "Timestamp: " << data.timestamp << std::endl;
            std::cout << "Open: " << data.open << std::endl;
            std::cout << "High: " << data.high << std::endl;
            std::cout << "Low: " << data.low << std::endl;
            std::cout << "Close: " << data.close << std::endl;
            std::cout << "Volume: " << data.volume << std::endl;
            std::cout << "-----------------------" << std::endl;
        }
    }
    
private:
    std::filesystem::path filePath;
    std::vector<MarketData> data_;
};

class dataLoader3 {
public:
    dataLoader3(std::filesystem::path path, std::size_t numThreads = std::thread::hardware_concurrency())
        : filePath(path), numThreads(numThreads) {loadDataAsync();}

    void loadDataAsync() {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Error opening file: " << filePath << std::endl;
            return;
        }

        std::string header;
        std::getline(file, header);

        // Use a thread pool for parallel loading
        std::vector<std::thread> threads;
        std::queue<std::string> lines;
        std::mutex mutex;
        std::condition_variable cv;
        bool done = false;

        // Function to load data from a line
        auto loadDataFromLine = [this, &lines, &mutex, &cv, &done]() {
            while (true) {
                std::string line;
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    cv.wait(lock, [&lines, &done] { return !lines.empty() || done; });
                    if (lines.empty() && done) {
                        break;
                    }
                    line = std::move(lines.front());
                    lines.pop();
                }

                std::istringstream ss(line);
                MarketData data;

                // Read and ignore the timestamp in the original form
                std::string timestampMisc;
                std::string tstamp;
                std::getline(ss, timestampMisc, ','); // Read and discard
                std::getline(ss, tstamp, ',');
                data.timestamp = convertTimestamp(tstamp);
                std::string openStr, highStr, lowStr, closeStr, volumeStr;

                std::getline(ss, openStr, ',');    // Open
                std::getline(ss, highStr, ',');    // High
                std::getline(ss, lowStr, ',');     // Low
                std::getline(ss, closeStr, ',');   // Close
                std::getline(ss, volumeStr, ',');  // Volume

                // Convert string values to doubles
                data.open = std::stod(openStr);
                data.high = std::stod(highStr);
                data.low = std::stod(lowStr);
                data.close = std::stod(closeStr);
                data.volume = std::stod(volumeStr);

                // Store the parsed data
                std::lock_guard<std::mutex> lock(dataMutex);
                data_.push_back(data);
            }
        };

        // Start worker threads
        for (std::size_t i = 0; i < numThreads; ++i) {
            threads.emplace_back(loadDataFromLine);
        }

        // Read lines from the file and enqueue for processing
        std::string line;
        while (std::getline(file, line)) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                lines.push(line);
            }
            cv.notify_one();
        }

        // Notify workers that no more lines are coming
        {
            std::lock_guard<std::mutex> lock(mutex);
            done = true;
        }
        cv.notify_all();

        // Join worker threads
        for (auto& thread : threads) {
            thread.join();
        }

        file.close();
    }

    std::vector<MarketData> dataGet() {
        std::lock_guard<std::mutex> lock(dataMutex);
        return data_;
    }

    void printData() {
        for (const auto& data : data_) {
            std::cout << "Timestamp: " << data.timestamp << std::endl;
            std::cout << "Open: " << data.open << std::endl;
            std::cout << "High: " << data.high << std::endl;
            std::cout << "Low: " << data.low << std::endl;
            std::cout << "Close: " << data.close << std::endl;
            std::cout << "Volume: " << data.volume << std::endl;
            std::cout << "-----------------------" << std::endl;
        }
    }

private:
    std::filesystem::path filePath;
    std::vector<MarketData> data_;
    std::mutex dataMutex;
    std::size_t numThreads;
};

class ThreadPool {
public:
    ThreadPool(std::size_t numThreads) {
        for (std::size_t i = 0; i < numThreads; ++i) {
            threads.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return !tasks.empty() || stopThreads; });
                        if (stopThreads && tasks.empty()) {
                            return;
                        }
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stopThreads = true;
        }
        condition.notify_all();
        for (auto& thread : threads) {
            thread.join();
        }
    }

    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stopThreads) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return result;
    }

private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stopThreads = false;
};

class dataLoader4 {
public:
    dataLoader4(std::filesystem::path path, std::size_t numThreads = std::thread::hardware_concurrency())
        : filePath(path), numThreads(numThreads) {loadDataAsync();}

    void loadDataAsync() {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Error opening file: " << filePath << std::endl;
            return;
        }

        std::string header;
        std::getline(file, header);

        // Use a thread pool for parallel loading
        std::vector<std::thread> threads;
        std::queue<std::string> lines;
        std::mutex mutex;
        std::condition_variable cv;
        bool done = false;

        // Function to load data from a line
        auto loadDataFromLine = [this, &lines, &mutex, &cv, &done]() {
            while (true) {
                std::string line;
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    cv.wait(lock, [&lines, &done] { return !lines.empty() || done; });
                    if (lines.empty() && done) {
                        break;
                    }
                    line = std::move(lines.front());
                    lines.pop();
                }

                std::istringstream ss(line);
                MarketData data;

                // Read and ignore the timestamp in the original form
                std::string timestampMisc;
                std::string tstamp;
                std::getline(ss, timestampMisc, ','); // Read and discard
                std::getline(ss, tstamp, ',');
                data.timestamp = convertTimestamp(tstamp);
                std::string openStr, highStr, lowStr, closeStr, volumeStr;

                std::getline(ss, openStr, ',');    // Open
                std::getline(ss, highStr, ',');    // High
                std::getline(ss, lowStr, ',');     // Low
                std::getline(ss, closeStr, ',');   // Close
                std::getline(ss, volumeStr, ',');  // Volume

                // Convert string values to doubles
                data.open = std::stod(openStr);
                data.high = std::stod(highStr);
                data.low = std::stod(lowStr);
                data.close = std::stod(closeStr);
                data.volume = std::stod(volumeStr);

                // Store the parsed data
                std::lock_guard<std::mutex> lock(dataMutex);
                data_.push_back(data);
            }
        };

        // Start worker threads
        for (std::size_t i = 0; i < numThreads; ++i) {
            threads.emplace_back(loadDataFromLine);
        }

        // Read lines from the file and enqueue for processing
        std::string line;
        while (std::getline(file, line)) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                lines.push(line);
            }
            cv.notify_one();
        }

        // Notify workers that no more lines are coming
        {
            std::lock_guard<std::mutex> lock(mutex);
            done = true;
        }
        cv.notify_all();

        // Join worker threads
        for (auto& thread : threads) {
            thread.join();
        }

        file.close();
    }

    std::vector<MarketData> dataGet() {
        std::lock_guard<std::mutex> lock(dataMutex);
        return data_;
    }

    void printData() {
        for (const auto& data : data_) {
            std::cout << "Timestamp: " << data.timestamp << std::endl;
            std::cout << "Open: " << data.open << std::endl;
            std::cout << "High: " << data.high << std::endl;
            std::cout << "Low: " << data.low << std::endl;
            std::cout << "Close: " << data.close << std::endl;
            std::cout << "Volume: " << data.volume << std::endl;
            std::cout << "-----------------------" << std::endl;
        }
    }

private:
    std::filesystem::path filePath;
    std::vector<MarketData> data_;
    std::mutex dataMutex;
    std::size_t numThreads;
};

class dataLoader2 {
public:
    dataLoader2(std::filesystem::path path) : filePath(path) {loadDataAsync();}

    // Optimized function for batch reading and parallel loading
    void loadDataAsync() {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error opening file: " << filePath << std::endl;
            return;
        }

        constexpr std::size_t BUFFER_SIZE = 8192; // Adjust the buffer size as needed
        char buffer[BUFFER_SIZE];

        // Get the file size
        file.seekg(0, std::ios::end);
        std::size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        ThreadPool threadPool(std::thread::hardware_concurrency()); // Use the number of available cores

        std::size_t bytesRead = 0;

        while (bytesRead < fileSize) {
            std::size_t readSize = std::min(BUFFER_SIZE, fileSize - bytesRead);
            // Process the buffer in a thread from the pool
            threadPool.enqueue(&dataLoader2::processBuffer, this, std::string(buffer, readSize));
            bytesRead += readSize;
            file.read(buffer, readSize);
        }

        file.close();
    }

    std::vector<MarketData> dataGet() {
        std::shared_lock<std::shared_timed_mutex> lock(dataMutex);
        return data_;
    }

private:
    std::filesystem::path filePath;
    std::vector<MarketData> data_;
    mutable std::shared_timed_mutex dataMutex; // Shared mutex for concurrent reading

    void processBuffer(const std::string& buffer) {
        std::istringstream ss(buffer);

        while (ss) {
            MarketData data;

            // Read and ignore the timestamp in the original form
            std::string timestampMisc;
            std::string tstamp;
            std::getline(ss, timestampMisc, ','); // Read and discard
            std::getline(ss, tstamp, ',');
            data.timestamp = convertTimestamp(tstamp);

            std::string openStr, highStr, lowStr, closeStr, volumeStr;

            std::getline(ss, openStr, ',');    // Open
            std::getline(ss, highStr, ',');    // High
            std::getline(ss, lowStr, ',');     // Low
            std::getline(ss, closeStr, ',');   // Close
            std::getline(ss, volumeStr, ',');  // Volume

            // Convert string values to doubles

            data.open = std::stod(openStr);
            data.high = std::stod(highStr);
            data.low = std::stod(lowStr);
            data.close = std::stod(closeStr);
            data.volume = std::stod(volumeStr);

            // Store the parsed data within the critical section
            std::unique_lock<std::shared_timed_mutex> lock(dataMutex);
            data_.push_back(data);
        }
    }
};

class event 
{
public:
    std::string type;  // Event type (e.g., "MarketData", "Signal")
    std::string timestamp="";
    std::vector<double> data_;

    event(const std::string& ty, const std::string& ts): type(ty), timestamp(ts) {}
    virtual ~event() = default;
    // MarketData dataMarket={"MarketData",0,0,0,0,0};
    // std::unordered_map<std::string,double> signalData {{"type",0}};
    
    // event(std::string ty, std::string ts, MarketData mD): type(ty), timestamp(ts),dataMarket(mD){}
    
    // // signal types:
    // // 0. hold
    // // 1. buy
    // // 2. sell
    // event(std::string ty, std::string ts, std::unordered_map<std::string,double> sigData): type(ty), timestamp(ts), signalData(sigData) {}
};

struct marketDataEvent : public event {
    marketDataEvent(const std::string& ts, MarketData data)
        : event("MarketData", ts), data_(data) {}

    MarketData data_;
};

struct signalEvent : public event {
    signalEvent(const std::string& ts, std::unordered_map<std::string, double> data)
        : event("Signal", ts), data_(data) {}

    std::unordered_map<std::string, double> data_;
};

class eventBus
{
public:
    void subscribe(const std::string& eventType, std::function<void(event&)> callback);

    void publish(event& evnt);
private:
    std::unordered_map<std::string, std::vector<std::function<void(event&)>>> subscribers;
};

class dataHandler 
{
public:
    // Constructor
    dataHandler(eventBus& Bus,const std::vector<MarketData>& historicalData) : bus(Bus), historicalMarketData(historicalData) {}

    // Function to simulate market data generation
    void simulateMarketData();

    void simulateMarketDataAsync() 
    {
        constexpr std::size_t NUM_THREADS = 4; // Adjust the number of threads as needed
        std::vector<std::future<void>> simulationFutures;

        for (std::size_t i = 0; i < NUM_THREADS; ++i) {
            simulationFutures.push_back(std::async(std::launch::async, &dataHandler::simulateMarketDataWorker, this));
        }

        // Wait for all threads to complete
        for (auto& future : simulationFutures) {
            future.wait();
        }
    }

    // Function to manually iterate through historical market data
    MarketData getNextMarketData();

    // Function to manually reset the iteration
    void resetIteration() 
    {
        currentDataIndex = 0;
    };

    // Function to manually trigger the next data point event
    void simulateNextMarketDataEvent();

    std::vector<MarketData> historicalMarketData;
    eventBus bus;
    size_t currentDataIndex = 0;
private:
    void simulateMarketDataWorker() 
    {
        while (true) {
            // Get the next market data
            MarketData data = getNextMarketData();
            if (data.timestamp.empty()) {
                // No more data to simulate
                break;
            }

            // Simulate market data event
            marketDataEvent mDataEvent{data.timestamp, data};
            bus.publish(mDataEvent);
        }
    }
};

class strategyEngine 
{
public:
    // Constructor
    explicit strategyEngine(eventBus& Bus) : bus(Bus) 
    {
        // Subscribe to MarketData events
        //bus.subscribe("MarketData", std::bind(&strategyEngine::onMarketData, this, std::placeholders::_1));
        bus.subscribe("MarketData", [this](event& evnt) 
        {
            if (auto marketDataEventPtr = dynamic_cast<marketDataEvent*>(&evnt)) {
                this->onMarketData(*marketDataEventPtr);
            }
        });
    }

    // Function to handle MarketData events
    void onMarketData(const marketDataEvent& evnt);

    // Function to be overridden by derived classes to implement strategy logic
    virtual std::unordered_map<std::string,double> generateSignal(const MarketData& marketData);

protected:
    // Helper function to extract MarketData from the event
    MarketData extractMarketData(const marketDataEvent& evnt);
    eventBus& bus;
};

class broker 
{
public:
    explicit broker(eventBus& Bus) : bus(Bus) {
        // Subscribe to Signal events
        //bus.subscribe("Signal", std::bind(&broker::onSignal, this, std::placeholders::_1));
        bus.subscribe("Signal", [this](event& evnt) {
            if (auto signalEventPtr = dynamic_cast<signalEvent*>(&evnt)) {
                this->onSignal(*signalEventPtr);
            }
        });

    }

    // Function to handle Signal events
    void onSignal(const signalEvent& evnt);

    // Function to execute a Buy order
    void executeBuyOrder(const signalEvent& evnt);

    // Function to execute a Sell order
    void executeSellOrder(const signalEvent& evnt);

private:
    eventBus& bus;
    double cash = 1000.0;  // Initial cash amount for the portfolio
    double asset = 0;
    bool inPosition = false; // Indicates whether the broker is in position
};



