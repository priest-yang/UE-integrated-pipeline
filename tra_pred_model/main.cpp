#include <iostream>
#include <fstream>
#include <string>
#include <deque>
#include <vector>
#include <thread>
#include <torch/script.h>
#include <torch/torch.h>
#include <argparse.hpp>

class ModelRunner {
private:
    torch::jit::script::Module model;
    std::string filename;
    int feature_dim;
    std::deque<torch::Tensor> buffer;
    size_t capacity;
    torch::Device device; // Store the device type
    int cnt = 0;

public:
    ModelRunner(const std::string& model_path, const std::string& filename, int feature_dim, size_t capacity)
        : filename(filename), feature_dim(feature_dim), capacity(capacity), 
         device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU) {
        try {
            model = torch::jit::load(model_path);
            // Set device based on CUDA availability
            model.to(device);
            std::cout << (device.type() == torch::kCUDA ? "Using GPU." : "Using CPU.") << std::endl;

            // Verify model input dimension with a random input
            auto test_input = torch::rand({1, feature_dim}, device);
            std::vector<torch::jit::IValue> inputs = {test_input};
            model.forward(inputs); // This will throw if the dimension is incorrect
        } catch (const c10::Error& err) {
            std::cerr << "Error loading the model: " << err.what() << std::endl;
            exit(-1);
        }
    }

    torch::Tensor convertLineToTensor(const std::string& line) {
        torch::Tensor tensor = torch::empty(feature_dim, torch::kFloat32);
        std::stringstream ss(line);
        std::string item;
        int index = 0;
        while (std::getline(ss, item, ',') && index < feature_dim) {
            try {
                tensor[index++] = std::stof(item);
            } catch (const std::exception& e) {
                std::cerr << "Parsing error: " << e.what() << " in line: " << line << std::endl;
                tensor[index++] = 0.0f; // Handle error, e.g., by setting to zero
            }
        }
        return tensor;
    }

    void feedModel(){
        if (buffer.empty()) return;

        // Stack tensors to create a batched input
        auto input = torch::stack(std::vector<torch::Tensor>(buffer.begin(), buffer.end()));

        // Forward pass through the model
        // auto outputs = model.forward({input}).toTuple();
        // auto predictions = outputs->elements()[0].toTensor();

        auto predictions = model.forward({input}).toTensor();
        std::cout << "Model output " << this->cnt + 1 << ": " 
                  << std::setw(3) << std::setfill(' ');
        this->cnt++;
        for (auto s : predictions.sizes()) std::cout << s << " ";
        std::cout << std::endl;

        std::cout << predictions << std::endl;
    }

    void updateBuffer(torch::Tensor newTensor) {
        if (buffer.size() >= capacity) {
            buffer.pop_front();  // Remove the oldest tensor if we are at capacity
        }
        buffer.push_back(newTensor);  // Add the new tensor to the buffer
    }

    void processFile(const std::string& spec_filename) {
        auto start = std::chrono::high_resolution_clock::now();

        std::string effectiveFilename = spec_filename.empty() ? this->filename : spec_filename;
        std::cout << "Processing file: " << effectiveFilename << std::endl;
        std::ifstream file(effectiveFilename);
        std::string line;
        if (std::getline(file, line)) {} // Optionally handle header

        while (std::getline(file, line)) {
            auto tensor = convertLineToTensor(line);
            updateBuffer(tensor);  // Update the buffer with each new line
            feedModel();           // Run the model on every new line
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout <<"\n\n";
        std::cout << "Elapsed time: " << elapsed.count() << " seconds." << std::endl;
        std::cout << "Processed " << this->cnt << " lines." << std::endl;
        std::cout << "Speed: " << this->cnt / elapsed.count() << " lines per second.\n\n" << std::endl;
    }


    void start(const std::string& filename = "") {
        this->cnt = 0;
        std::thread worker(&ModelRunner::processFile, this, filename);
        worker.join();
    }
};

int main(int argc, char** argv) {
    argparse::ArgumentParser program("FAM Benchmarking Program");

    // Add arguments
    program.add_argument("-f", "--file_path")
        .help("Path to the CSV file containing feature records")
        .default_value(std::string("data/demo/feature_model/0.csv"));

    program.add_argument("-b", "--batch_size")
        .help("Size of bach for processing")
        .default_value(40)
        .scan<'i', size_t>(); // Scanning as size_t; 

    program.add_argument("-m", "--model_path")
        .help("Path to the model file")
        .default_value(std::string("tra_pred_model/model/model_tft_vqvae_cpu_preproc.pt"));

    program.add_argument("--feature_dim")
        .help("Feature dimension")
        .default_value(32)
        .scan<'i', int>();

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        std::cout << program;
        exit(0);
    }

    // Get the file path and buffer size from the arguments
    std::string file_path = program.get<std::string>("-f");
    const size_t batch_size = program.get<size_t>("-b");
    std::string model_path = program.get<std::string>("-m");
    int feature_dim = program.get<int>("--feature_dim");


    ModelRunner runner(model_path,
                       file_path,
                       feature_dim, // Feature dimension
                       batch_size); // Capacity or batch size

    runner.start(file_path);
    return 0;
}
