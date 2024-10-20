#include <iostream>
#include <fstream>
#include <string>
#include <deque>
#include <vector>
#include <thread>
#include "argparse.hpp"
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>


class ModelRunner {
private:
    Ort::Env env;
    Ort::SessionOptions session_options;
    Ort::Session session;
    std::vector<std::string> input_node_names;
    std::vector<std::string> output_node_names;
    std::string filename;
    int feature_dim;
    std::deque<std::vector<float>> buffer;
    size_t capacity;
    int cnt = 0;

public:
    ModelRunner(const std::string& model_path, const std::string& filename, int feature_dim, size_t capacity)
        : filename(filename), feature_dim(feature_dim), capacity(capacity),
          env(ORT_LOGGING_LEVEL_WARNING, "ModelRunner"),
          session_options(), session(nullptr) {
        // Set session options if needed
        session_options.SetIntraOpNumThreads(1);

        // Load the model
        try {
            session = Ort::Session(env, model_path.c_str(), session_options);

            // Get input and output node names
            size_t num_input_nodes = session.GetInputCount();
            size_t num_output_nodes = session.GetOutputCount();
            Ort::AllocatorWithDefaultOptions allocator;

            // Get input node names
            for (size_t i = 0; i < num_input_nodes; i++) {
                Ort::AllocatedStringPtr input_name = session.GetInputNameAllocated(i, allocator);
                input_node_names.push_back(input_name.get());
            }

            // Get output node names
            for (size_t i = 0; i < num_output_nodes; i++) {
                Ort::AllocatedStringPtr output_name = session.GetOutputNameAllocated(i, allocator);
                output_node_names.push_back(output_name.get());
            }

            std::cout << "Model loaded successfully." << std::endl;
        } catch (const Ort::Exception& exception) {
            std::cerr << "Error loading the model: " << exception.what() << std::endl;
            exit(-1);
        }
    }

    std::vector<float> convertLineToVector(const std::string& line) {
        std::vector<float> vec(feature_dim, 0.0f);
        std::stringstream ss(line);
        std::string item;
        int index = 0;
        while (std::getline(ss, item, ',') && index < feature_dim) {
            try {
                vec[index++] = std::stof(item);
            } catch (const std::exception& e) {
                std::cerr << "Parsing error: " << e.what() << " in line: " << line << std::endl;
                vec[index++] = 0.0f; // Handle error by setting to zero
            }
        }
        return vec;
    }

    void feedModel() {
        if (buffer.size() < capacity) return;

        size_t batch_size = buffer.size();
        size_t input_tensor_size = batch_size * feature_dim;
        std::vector<float> input_tensor_values;
        input_tensor_values.reserve(input_tensor_size);

        for (const auto& vec : buffer) {
            input_tensor_values.insert(input_tensor_values.end(), vec.begin(), vec.end());
        }

        std::vector<int64_t> input_shape = {static_cast<int64_t>(batch_size), feature_dim};

        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, input_tensor_values.data(), input_tensor_size, input_shape.data(), input_shape.size());

        // Prepare input and output node names
        const char* input_names[] = {input_node_names[0].c_str()};
        const char* output_names[] = {output_node_names[0].c_str()};

        // Run the model
        auto output_tensors = session.Run(
            Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);

        // Get output tensor
        float* output_data = output_tensors[0].GetTensorMutableData<float>();

        // Get output shape
        Ort::TensorTypeAndShapeInfo output_info = output_tensors[0].GetTensorTypeAndShapeInfo();
        std::vector<int64_t> output_shape = output_info.GetShape();

        // Output the shape and data
        std::cout << "Model output " << this->cnt + 1 << ": ";
        this->cnt++;
        for (auto s : output_shape) std::cout << s << " ";
        std::cout << std::endl;

        size_t output_size = output_info.GetElementCount();
        std::cout << "Output data: ";
        for (size_t i = 0; i < output_size; i++) {
            std::cout << output_data[i] << " ";
        }
        std::cout << std::endl;
    }

    void updateBuffer(const std::vector<float>& newVector) {
        if (buffer.size() >= capacity) {
            buffer.pop_front();  // Remove the oldest vector if we are at capacity
        }
        buffer.push_back(newVector);  // Add the new vector to the buffer
    }

    void processFile(const std::string& spec_filename) {
        auto start = std::chrono::high_resolution_clock::now();

        std::string effectiveFilename = spec_filename.empty() ? this->filename : spec_filename;
        std::cout << "Processing file: " << effectiveFilename << std::endl;
        std::ifstream file(effectiveFilename);
        std::string line;
        if (std::getline(file, line)) {} // Optionally handle header

        while (std::getline(file, line)) {
            auto vec = convertLineToVector(line);
            updateBuffer(vec);    // Update the buffer with each new line
            feedModel();          // Run the model on every new line
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "\n\n";
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

